/*
 * @Author: your name
 * @Date: 2021-07-15 10:59:16
 * @LastEditTime: 2021-08-07 21:56:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \IAR_OS\USER\svc.c
 */
#include "svc.h"

unsigned int u_printf_svc(const char *str, ...);

#define SVC_SERVER_FUNCTION_MAX 30

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

uint32_t (*svc_server_function[SVC_SERVER_FUNCTION_MAX])(unsigned int r0, unsigned int r1, unsigned int r2, unsigned int r3)=
{
    thread_sleep,           //0
    newlyBuild_thread,      //1
    newlyBuild_thread_stack,//2
    thread_stars,           //3
    0,                      //4
    //usart_re,               //5
    0,
    //u_printf_svc,
    0,
};

#pragma GCC diagnostic pop

#define SVC_SERVER_ID(stack) (((char *)((unsigned int *)stack)[6])[-2])
#define SVC_SERVER_R0(stack) (((unsigned int *)stack)[0])
#define SVC_SERVER_R1(stack) (((unsigned int *)stack)[1])
#define SVC_SERVER_R2(stack) (((unsigned int *)stack)[2])
#define SVC_SERVER_R3(stack) (((unsigned int *)stack)[3])
#define SVC_SERVER_R12(stack) (((unsigned int *)stack)[4])

void SVC_Handler_server(unsigned int *stack)
{
    unsigned int svc_number = SVC_SERVER_ID(stack);
    if (svc_number >= SVC_SERVER_FUNCTION_MAX)
    {
        SVC_SERVER_R0(stack) = 0;
        return;
    }
    SVC_SERVER_R0(stack) = svc_server_function[svc_number](SVC_SERVER_R0(stack), SVC_SERVER_R1(stack), SVC_SERVER_R2(stack), SVC_SERVER_R3(stack));
    return;
}
