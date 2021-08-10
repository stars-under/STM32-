#include "os.h"

//系统当前信息
volatile os_system system_information = {0, 0, 0};

//线程信息
volatile THREAD thread_information[MAX_thread];

//栈区域
volatile uint32_t stack[(THREAD_stack_size)*MAX_thread];

//配置滴答定时器和时间片中断
void sys_time_init(uint32_t usCount)
{
    //在AHB进行8分频（9M）后把时钟提供到SysTick，所以每9K记录下1ms
    SysTick->CTRL &= 0xFFFFFFFB;
    //定时系统产生中断
    SysTick->CTRL |= (0x1 << 1);
    //清除计时器值
    SysTick->VAL = 0x0;
    //设置装载值
    SysTick->LOAD = 9 * usCount;

    return;
}

//栈信息变量
volatile uint32_t actual;

#if defined(__VFP_FP__) && !defined(__SOFTFP__)
//中断返回值暂存变量
volatile uint32_t EXC_RETURN_BUFF;
#endif

//更新pid服务
void os_update_service()
{
    //写入信息_栈地址
    thread_information[system_information.pid].stack_position = actual;
#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    //写入信息_中断返回值
    thread_information[system_information.pid].EXC_RETURN = EXC_RETURN_BUFF;
#endif
    //时间++
    system_information.SYS_TIME++;
    //获取下一个id
    system_information.pid = lookup_pid(system_information.pid);
    //写入新栈地址
    actual = thread_information[system_information.pid].stack_position;
#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    //写入信息_中断返回值
    EXC_RETURN_BUFF = thread_information[system_information.pid].EXC_RETURN;
#endif
}

/*
    时间片核心函数

    基本思想:
        首先硬件在进入中断时会自动入栈{R0-R3,LR,PC,xPSR,R12}共8个寄存器 

        该函数需要做的就是,在硬件自动入栈8个寄存器后.入栈剩下的8个寄存器
        并找出下一个线程的栈地址.随后写入PSP并软件出栈(8个).
        return后随着中断出栈将PC带出栈
*/
#if (os_version <= 100U)
void SysTick_Handler()
{
    //保存栈
    __asm__ __volatile__(
        "push   {LR}\n"
        "mrs    R0,PSP\n"
        "stmdb  R0!,{r4-r11}\n"
        "LDR    R1, =actual\n"
        "STR    R0, [R1]\n"
        "BL     os_update_service\n"
        "LDR    R1, =actual\n"
        "LDR    R0,[R1]\n"
        "ldmia  R0!,{r4-r11}\n"
        "msr    PSP,R0\n"
        "pop    {PC}\n");
}
#endif

void SysTick_Handler()
{
    system_information.next_pid = lookup_pid(system_information.pid);
    system_information.SYS_TIME++;
    __asm__ __volatile__(
        ".EQU NVIC_INT_CTRL , 0xE000ED04 @ 中断控制寄存器\n"
        ".EQU NVIC_SYSPRI14 , 0xE000ED22 @ 系统优先级寄存器(优先级14). \n"
        ".EQU NVIC_PENDSV_PRI , 0xFF @ PendSV优先级(最低). NVIC_PENDSVSET EQU 0x10000000 ; PendSV触发值\n"
        ".EQU NVIC_PENDSVSET , 0x10000000 @ PendSV触发值\n"
        "@ 触发PendSV异常\n"
        "LDR R0, =NVIC_INT_CTRL \n"
        "LDR R1, =NVIC_PENDSVSET \n"
        "STR R1, [R0]\n");
}

//线程退出时的处理函数
void thread_handle_return()
{
    thread_information[system_information.pid].thread_state = 4;
    while (1)
        ;
}

uint32_t reboot_thread(unsigned int id)
{
    if (id >= MAX_thread)
    {
        return 0;
    }
    //重新计算栈栈指针
    thread_information[id].stack_position = thread_information[id].stack_stars - 8 * 4;
    //写入指针
    HEADER_visit_STACK_PSP(thread_information[id].stack_position, STACK_PC) = thread_information[id].reboot_pc;
    //写xPSR
    HEADER_visit_STACK_PSP(thread_information[id].stack_position, STACK_xPSR) = 0x1000000;
    //传递参数
    HEADER_visit_STACK_PSP(thread_information[id].stack_position, STACK_R0) = thread_information[id].parameter;
    //线程处于未启动(死亡状态)
    thread_information[id].thread_state = 2;
    //重置优先级
    thread_information[id].actual_priority = thread_information[id].priority;
    thread_stars(id);
    return 1;
}

uint32_t hang_thread(unsigned int id)
{
    if (id >= MAX_thread)
    {
        return 0;
    }
    thread_information[id].thread_state = 0b100;
    return 1;
}

uint32_t cancel_hang_thread(unsigned int id)
{
    if (id >= MAX_thread)
    {
        return 0;
    }
    thread_information[id].thread_state = 0b001;
    return 1;
}

//新建一个线程
uint32_t newlyBuild_thread(new_thread_information_pointer new_thread)
{
    //数量增加
    system_information.thread_num++;
    //给栈指针
    thread_information[system_information.thread_num].stack_stars = (uint32_t)&stack[
        //没有越栈正常分配
        ((system_information.thread_num + 1) * THREAD_stack_size)];
    //计算实际栈指针
    thread_information[system_information.thread_num].stack_position = thread_information[system_information.thread_num].stack_stars - 8 * 4;
    //写入指针
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_PC) = new_thread->PC;
    //写入退出函数
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_LR) = (uint32_t)&thread_handle_return;
    //写xPSR
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_xPSR) = 0x1000000;
    //传递参数
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_R0) = new_thread->parameter;
    //写入优先级
    thread_information[system_information.thread_num].actual_priority = new_thread->priority;
    //写入优先级
    thread_information[system_information.thread_num].priority = new_thread->priority;
    //线程处于未启动(死亡状态)
    thread_information[system_information.thread_num].thread_state = 2;
    //重启操作需要PC
    thread_information[system_information.thread_num].reboot_pc = new_thread->PC;
    //重启需要参数
    thread_information[system_information.thread_num].parameter = new_thread->parameter;
#if defined(__VFP_FP__) && !defined(__SOFTFP__)

    //M4需要EXC_RETURN(中断返回值判断是否启用了浮点运算单元)
    thread_information[system_information.thread_num].EXC_RETURN = 0xFFFFFFFD;
#endif
    //返回id
    return system_information.thread_num;
}

//新建一个线程
uint32_t newlyBuild_thread_stack(new_thread_information_pointer new_thread, void *stack)
{
    //数量增加
    system_information.thread_num++;
    //给栈指针
    thread_information[system_information.thread_num].stack_stars = (uint32_t)stack;
    //计算实际栈指针
    thread_information[system_information.thread_num].stack_position = thread_information[system_information.thread_num].stack_stars - 8 * 4;
    //写入指针
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_PC) = new_thread->PC;
    //写入退出函数
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_LR) = (uint32_t)&thread_handle_return;
    //写xPSR
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_xPSR) = 0x1000000;
    //传递参数
    HEADER_visit_STACK_PSP(thread_information[system_information.thread_num].stack_position, STACK_R0) = new_thread->parameter;
    //写入优先级
    thread_information[system_information.thread_num].actual_priority = new_thread->priority;
    //写入优先级
    thread_information[system_information.thread_num].priority = new_thread->priority;
    //线程处于未启动(死亡状态)
    thread_information[system_information.thread_num].thread_state = 2;
#if defined(__VFP_FP__) && !defined(__SOFTFP__)

    //M4需要EXC_RETURN(中断返回值判断是否启用了浮点运算单元)
    thread_information[system_information.thread_num].EXC_RETURN = 0xFFFFFFFD;
#endif
    //返回id
    return system_information.thread_num;
}

//启动一个线程
void thread_stars(uint32_t pid)
{
    //标记线程为正常
    thread_information[pid].thread_state = 1;
    //为空的R4-R11余位
    thread_information[pid].stack_position = thread_information[pid].stack_position - 8 * 4;
}

//扩容栈
void expansion_stack(uint32_t pid)
{
}

//休眠一个线程 单位ms
void thread_sleep(uint32_t sleep)
{

    //计算休眠时间
    thread_information[system_information.pid].sleep_time = system_information.SYS_TIME + sleep;
    //标记休眠状态0
    thread_information[system_information.pid].thread_state = 2;
    //将空闲时间片指向空闲线程
    system_information.next_pid = 1;
    __asm__ __volatile__(
        ".EQU NVIC_INT_CTRL , 0xE000ED04 @ 中断控制寄存器\n"
        ".EQU NVIC_PENDSVSET , 0x10000000 @ PendSV触发值\n"
        "@ 触发PendSV异常\n"
        "LDR R0, =NVIC_INT_CTRL \n"
        "LDR R1, =NVIC_PENDSVSET \n"
        "STR R1, [R0]\n");
}

#ifdef EVENT_STAND_BY
//事件信息结构体
EVENT_SYS event_information = {
    0, {0}, {0}};

uint16_t event_id;

//添加事件服务
uint32_t event_Add_service(EVENT_SERVICE information)
{
    //当服务函数为空或线程id为0时失败
    if ((uint32_t)information.service == 0 ||
        (uint32_t)information.thread_id == 0)
    {
        return 0;
    }
    //写入PC
    event_information.service[event_information.THREAD_NUM] = information.service;
    //写入被唤醒的id
    event_information.thread_id[event_information.THREAD_NUM] = information.thread_id;
    if (++event_information.THREAD_NUM >= EVENT_THREAD_NUM_MAX)
    {
        return 0;
    }

    return 1;
}

//事件处理线程
void event_thread()
{
    uint16_t i = 0;

    while (1)
    {
        if ((uint32_t)event_information.service[i] != 0)
        {
            if ((THREAD_OPERATING(event_information.thread_id[i]).EVENT_parameter = event_information.service[i]()) != 0)
            {
                THREAD_OPERATING(
                    event_information.thread_id[i])
                    .thread_state = 1;
            }
        }
        if (i++ >= event_information.THREAD_NUM)
        {
            i = 0;
        }
    }
}

//休眠一个线程,并等待唤醒
uint32_t thread_Dormant()
{
    /*
    该代码片用于防止在以下情况导致死循环
        1.在用户程序关闭时间片后休眠会导致死循环
    */

    //保存定时器状态
    int32_t ctrl = SysTick->CTRL;
    //打开OS时间片内核
    start_sys();
    //标记休眠状态
    thread_information[system_information.pid].thread_state = 4;
    //等待状态被恢复
    while (thread_information[system_information.pid].thread_state != 1)
        ;

    //恢复定时器状态
    SysTick->CTRL = ctrl;

    //返回事件函数的返回值
    return thread_information[system_information.pid].EVENT_parameter;
}

//在休眠状态中等待挂起的唤醒
uint32_t thread_sleep_and_Dormant(uint32_t sleep)
{
    /*
    该代码片用于防止在以下情况导致死循环
        1.在用户程序关闭时间片后休眠会导致死循环
    */

    //保存定时器状态
    int32_t ctrl = SysTick->CTRL;
    //打开OS时间片内核
    start_sys();

    //计算休眠时间
    thread_information[system_information.pid].sleep_time = system_information.SYS_TIME + sleep;
    //挂起加延时 使用延时的 状态
    thread_information[system_information.pid].thread_state = 3;
    //等待状态被恢复
    while (thread_information[system_information.pid].thread_state != 1)
        ;

    //恢复定时器状态
    SysTick->CTRL = ctrl;

    //判断唤醒是由什么引起的
    if (thread_information[system_information.pid].sleep_time > system_information.SYS_TIME)
    {
        //是由事件唤醒的

        //返回事件函数的返回值
        return thread_information[system_information.pid].EVENT_parameter;
    }
    else
    {
        //是时间到期唤醒的

        //恢复休眠时间
        thread_information[system_information.pid].sleep_time = system_information.SYS_TIME;
        return 0;
    }
}
#endif