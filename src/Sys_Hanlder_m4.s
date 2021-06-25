
/**
  ******************************************************************************
  * @file      Sys_Handler_m4.s
  * @author    MCD Application Team
  * @version   V0.1.0
  * @date      11-May-2021
  * @brief     用于时间片的切换工作
  ******************************************************************************
  * @attention
  *
  * 请在确定芯片型号后选择开启该文件或"Sys_Handler_m3.s"的编译
  * 本文件专为具有FPU单元的Cotrex_M4内核服务
  ******************************************************************************
  */
  
.section .data

.section .bss

.section	.text.SysTick_Handler
.thumb
.syntax unified
.type	SysTick_Handler, %function
.global SysTick_Handler

SysTick_Handler:
    @入栈LR(MSP)
    //push   {LR}
    
    @读取EXC_RETURN
    MOV    R3,LR
    @读取EXC_RETURN_BUFF的地址到R1
    LDR    R2,=EXC_RETURN_BUFF
    @把EXC_RETURN写入到EXC_RETURN_BUFF中
    STR    R3,[R2]
    
    @把PSP读取到R1
    mrs    R0,PSP


#if defined (__VFP_FP__) && !defined(__SOFTFP__)

    @判断第4位()是否为1
    @if((R1&0x10)!=0)
    TST    R3,#0x00000010
    @跳转到入短栈帧
    BNE      enter_short_stack
    @跳转到入长栈帧
    BEQ      enter_long_stack
enter_short_stack:
    @入短栈帧
    stmdb  R0!,{r4-r11}
    B      enter_stack_end
enter_long_stack:
    @入长栈帧
    stmdb  R0!,{r4-r11}
    vstmdb R0!, {d8 - d15}
    B      enter_stack_end
enter_stack_end:
#else
    @入短栈帧
    stmdb  R0!,{r4-r11}
#endif

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

    LDR    R2,=EXC_RETURN_BUFF

    LDR    R3,[R2]

#if defined (__VFP_FP__) && !defined(__SOFTFP__)

    @判断第4位()是否为1
    @if((R1&0x10)!=0)
    TST    R3,#0x00000010
    @跳转到出短栈帧
    BNE      out_short_stack
    @跳转到出长栈帧
    BEQ      out_long_stack
out_short_stack:
    @出短栈帧{r4-r11}
    ldmia  R0!,{r4-r11}
    @结束出栈
    B      out_stack_end
out_long_stack:
    @出长栈帧{s16-s31}浮点寄存器
    vldmia  R0!,{d8 - d15}
    @出长栈帧{r4-r11}
    ldmia  R0!,{r4-r11}
    @结束出栈
    B      out_stack_end
out_stack_end:

#else

    @出短栈帧{r4-r11}
    stmdb  R0!,{r4-r11}

#endif

    @把R0写入到PSP
    msr    PSP,R0

    BX      R3
    //push   {R3}

    @弹出上次的LR
    //mov    PC,R3
    //pop    {PC}
