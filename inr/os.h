#ifndef __OS_H_
#define __OS_H_

#include "os_include.h"


#define EXPERIMENT

#define os_version (102U)

#define MAX_thread 16

#define THREAD_stack_size (8 * 16) //8是栈大小,8是可容纳8层嵌套

#define THREAD_warning 15 * 2 * sizeof(uint32_t) * 8 //当栈只剩下2层嵌套时警告

#define THREAD_extend 15 * 2 * sizeof(uint32_t) * 8 //每次扩容时扩容两个嵌套

//#define EVENT_STAND_BY  //事件支持

#define EVENT_THREAD_NUM_MAX 16

//对栈操作宏的支持
#define STACK_xPSR 1
#define STACK_PC 2
#define STACK_LR 3
#define STACK_R12 4
#define STACK_R3 5
#define STACK_R2 6
#define STACK_R1 7
#define STACK_R0 8

#define HEADER_visit_STACK_MSP(stack, rx) (*(uint32_t *)(stack + ((2 + (8 - rx)) * 4))) //对MSP操作的宏的正常,2*4是因为在c中自动将R3和LR入栈了,所以访问要上访2个字节
#define HEADER_visit_STACK_PSP(stack, rx) (*(uint32_t *)(stack + ((8 - rx) * 4)))       //对PSP栈操作的宏

//系统信息结构体
typedef struct os_system
{
    //当前运行pid
    uint32_t pid;
    //当前系统运行时间
    uint32_t SYS_TIME;
    //线程数量
    uint32_t thread_num;
    #ifdef EXPERIMENT
    uint32_t next_pid;
    #endif // DEBUG

} os_system;

//线程信息结构体
typedef struct THREAD
{
    //线程当前栈指针
    uint32_t stack_position;
    //线程栈指针的开始处
    uint32_t stack_stars;
    //栈大小(一个单位代表8bit)
    uint16_t stack_size;
    //线程状态
    /*
      最高位                                                         最低位
        0       0        0        0        0        0        0        0
                                                  |         状态位      |
       状态位:
            000(0)     死亡(或没有该线程)
            001(1)     正常
            010(2)     休眠
            011(3)     休眠并挂起(等待其他程序唤醒或超时唤醒)
            100(4)     挂起(等待其他程序唤醒)
            101(5)     同(0)
    */
    unsigned char thread_state;
    //休眠时间
    uint32_t sleep_time;
    //优先级
    unsigned char priority;
    //事实优先级
    unsigned char actual_priority;
    //reboot PC
    unsigned int reboot_pc;
    //parameter
    unsigned int parameter;

#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    //M4需要EXC_RETURN(中断返回值判断是否启用了浮点运算单元)
    uint32_t EXC_RETURN;
#endif

#ifdef EVENT_STAND_BY
    //事件 返回值
    uint32_t EVENT_parameter;
#endif

} THREAD;

#ifdef EVENT_STAND_BY

typedef struct EVENT_SYS
{
    //事件线程数量
    uint16_t THREAD_NUM;
    //事件的函数指针
    int (*service[EVENT_THREAD_NUM_MAX])(void);
    //事件对应的线程pid
    int thread_id[EVENT_THREAD_NUM_MAX];
} EVENT_SYS;

//添加服务信息的结构体
typedef struct EVENT_SERVICE
{
    uint32_t thread_id;
    int (*service)(void);
} EVENT_SERVICE;

#endif

//线程信息
volatile extern THREAD thread_information[MAX_thread];

#define THREAD_OPERATING(thread) thread_information[thread]

//系统当前信息
volatile extern os_system system_information;

//栈区域
volatile extern uint32_t stack[(THREAD_stack_size)*MAX_thread];

void PendSV_Init();

/*
*   功能:延时固定的时间(单位:ms)
*
*   @param sleep 延时时间(单位:ms)
*
*   @return 无输出
*/
void thread_sleep(uint32_t sleep);

//新建线程的信息结构体
typedef struct new_thread_information
{
    //函数开始地址
    uint32_t PC;
    //参数
    uint32_t parameter;
    //优先级
    unsigned char priority;
} new_thread_information, *new_thread_information_pointer;

/*
*   功能:配置滴答定时器和时间片中断
*
*   @param usCount 中断分辨率
*
*   @return 无输出
*/
void sys_time_init(uint32_t usCount);

/*
*   功能:新建线程
*
*   @param new_thread 指向线程信息的结构体指针
*
*   @return 返回线程id
*/
uint32_t newlyBuild_thread(new_thread_information_pointer new_thread);

/*
*   功能:新建线程(指定栈的位置)
*
*   @param new_thread 指向线程信息的结构体指针
*
*   @param stack 栈地址
*
*   @return 返回线程id
*/
uint32_t newlyBuild_thread_stack(new_thread_information_pointer new_thread, void *stack);

/*
*   功能:重启线程
*
*   @param id 线程id
*
*   @return ok return 1 and on return 0
*/
uint32_t reboot_thread(unsigned int id);

/*
*   功能:挂起线程
*
*   @param id 线程id
*
*   @return ok return 1 and on return 0
*/
uint32_t hang_thread(unsigned int id);

/*
*   功能:将挂起线程恢复
*
*   @param id 线程id
*
*   @return ok return 1 and on return 0
*/
uint32_t cancel_hang_thread(unsigned int id);

/*
*   功能:启动一个线程
*
*   @param 线程pid
*
*   @return 无返回
*/
void thread_stars(uint32_t pid);

/*
*   功能:暂时关闭时间片切换
*
*   @param 无输入
*
*   @return 无输出
*/
static inline void keil_sys()
{
    SysTick->CTRL &= ~(0x01 << 0);
}

/*
*   功能:再次开启时间片切换
*
*   @param 无输入
*
*   @return 无输出
*/
static inline void start_sys()
{
    SysTick->CTRL |= 0x01 << 0;
}

/*
*   功能:恢复一个线程到正常状态
*
*   @param thread_id 线程id
*
*   @return 无输出
*/
static inline void wake_thread(uint8_t thread_id)
{
    THREAD_OPERATING(thread_id).thread_state = 1;
}

/*
*   功能:休眠一个线程,由时间到期或事件唤醒
*
*   @param sleep 到期事件(ms)
*
*   @return 返回事件的返回值
*/
uint32_t thread_sleep_and_Dormant(uint32_t sleep);

#ifdef EVENT_STAND_BY

/*
*   功能:新建一个事件
*
*   @param information 事件的信息结构体指针
*
*   @return 返回1为成功 0为失败(数据异常)
*/
uint32_t event_Add_service(EVENT_SERVICE information);

/*
*   功能:事件处理线程 注:不允许用户调用
*
*   @param 无输入
*
*   @return 无输出
*/
void event_thread();

/*
*   功能:休眠当前线程并等待唤醒
*   
*   @param 无输入
*
*   @return 返回事件的返回值
*/
uint32_t thread_Dormant();

#endif

/*
*   功能:查找下一个可切换的pid
*
*   @param pid 当前pid
*
*   @return 无输出
*/
static inline uint32_t lookup_pid(uint32_t pid)
{
    unsigned char i = 0;
    unsigned pid_actual = pid;
    //为了让优先级更均匀 循环查找MAX_thread-1个线程
    for (i = 0; i < MAX_thread - 1; i++)
    {
        //下一个pid
        if (++pid >= MAX_thread)
        {
            pid = 0;
        }
        switch (thread_information[pid].thread_state)
        {
        case 1:
            if (thread_information[pid].actual_priority >= thread_information[pid_actual].actual_priority)
            {
                pid_actual = pid;
            }
            break;
        case 2: //休眠
            if (system_information.SYS_TIME > thread_information[pid].sleep_time)
            {
                thread_information[pid].sleep_time = 0;
                thread_information[pid].thread_state = 1;

                pid_actual = pid;
            }
            break; //顺延到下一个选项
        case 3:    //休眠并挂起
            if (system_information.SYS_TIME > thread_information[pid].sleep_time)
            {
                thread_information[pid].sleep_time = 0;
                thread_information[pid].thread_state = 1;

                pid_actual = pid;
            }
            break;
        default:
            break;
        }
    }
    if (--thread_information[pid_actual].actual_priority == 1)
    {
        thread_information[pid_actual].actual_priority = thread_information[pid_actual].priority;
    }
    return pid_actual;
}

/*
*   功能:开始时间片切换进程
*
*   @param 无输入
*
*   @return 无输出
*/

static inline void os_stars()
{
#ifdef EVENT_STAND_BY
    //初始化事件线程
    new_thread_information eve_thread;
    eve_thread.PC = (uint32_t)&event_thread;
    eve_thread.parameter = 0;
    eve_thread.priority = 4;
    uint32_t id = newlyBuild_thread(&eve_thread);
    thread_stars(id);
#endif
    PendSV_Init();
    //启动定时器
    sys_time_init(1000);
    //切换模式
    __set_CONTROL(0x02);
    //PSP = MSP
    __set_PSP(__get_MSP());
    //MSP = &stack[THREAD_stack_size]
    __set_MSP((uint32_t)&stack[THREAD_stack_size]);
    //线程处于启动状态(正常)
    thread_information[0].thread_state = 1;
    //优先级设置
    thread_information[0].priority = 4;
    thread_information[0].actual_priority = 4;
    thread_information[0].name = "main thread";
    //重置运行时间
    system_information.SYS_TIME = 0;
    //开启中断
    SysTick->CTRL |= 0x01 << 0;
}

#ifdef EVENT_STAND_BY
static inline void awaken_thread(uint32_t id, uint32_t data)
{
    if (thread_information[id].thread_state != 3 && thread_information[id].thread_state != 4)
    {
        return;
    }
    thread_information[id].EVENT_parameter = data;
    thread_information[id].thread_state = 1;
}
#endif
#endif
