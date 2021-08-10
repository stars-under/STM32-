   
.section .data

/**
  ******************************************************************************
  * @file      Sys_Handler_m3.s
  * @author    MCD Application Team
  * @version   V0.1.0
  * @date      11-May-2021
  * @brief     用于时间片的切换工作
  ******************************************************************************
  * @attention
  *
  * 请在确定芯片型号后选择开启该文件或"Sys_Handler_m4.s"的编译
  * 本文件专为Cotrex_M3内核服务
  ******************************************************************************
  */


.section	.text.PendSV_Handler
.thumb
.syntax unified
.type	PendSV_Handler, %function
.global PendSV_Handler
.extern system_information

PendSV_Handler:
    push   {LR}
    @把PSP读取到R1
    mrs    R0,PSP
    @入短栈帧
    stmdb  R0!,{r4-r11}
    @读取上一个线程的id
    @system_information.pid
    ldr    R1,=(system_information + 0x00)
    ldr    R1,[R1]
    @设置上一个线程的栈信息
    BL     __set_pid_stack
    @读取下一个线程的id
    @system_information.next_pid
    LDR    R0,=(system_information + 0x0C)
    LDR    R0,[R0]
    @system_information.pid = system_information.next_pid
    LDR    R1,=(system_information + 0x00)
    STR    R0,[R1]
    @获得下一个线程的栈信息
    BL     __get_pid_stack
    @出短栈帧{r4-r11}
    ldmia  R0!,{r4-r11}
    @把R0写入到PSP
    msr    PSP,R0
    @return
    pop    {PC}

.EQU NVIC_INT_CTRL , 0xE000ED04 @ 中断控制寄存器
.EQU NVIC_SYSPRI14 , 0xE000ED22 @ 系统优先级寄存器(优先级14). 
.EQU NVIC_PENDSV_PRI , 0xFF @ PendSV优先级(最低). NVIC_PENDSVSET EQU 0x10000000 ; PendSV触发值
.EQU NVIC_PENDSVSET , 0x10000000 @ PendSV触发值

.section	.text.PendSV_Init
.thumb
.syntax unified
.type	PendSV_Init, %function
.global PendSV_Init

PendSV_Init:
    @ 设置PendSV的异常中断优先级
    LDR R0, =NVIC_SYSPRI14 
    LDR R1, =NVIC_PENDSV_PRI
    STRB R1, [R0] 
    @return
    BX  LR
