   
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
.section .bss

.section	.text.SysTick_Handler
.thumb
.syntax unified
.type	SysTick_Handler, %function
.global SysTick_Handler

SysTick_Handler:
    @入栈LR(MSP)
    push   {LR}
    @把PSP读取到R0
    mrs    R0,PSP
    @入栈R4-R11(R0)
    stmdb  R0!,{r4-r11}
    @读取actual的地址到R1
    LDR    R1, =actual
    @把R0写入R1指向的地址
    STR    R0, [R1]
    @跳转到查找下一个pid的函数
    BL     os_update_service
    @读取actual的地址到R1
    LDR    R1, =actual
    @把R1指向的地址的数据写入到R0
    LDR    R0,[R1]
    @使用R0出栈R4-R11
    ldmia  R0!,{r4-r11}
    @把R0写入到PSP
    msr    PSP,R0
    @弹出LR到PC
    pop    {PC}
