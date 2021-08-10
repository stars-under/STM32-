/*
 * @Author: your name
 * @Date: 2021-08-06 20:25:28
 * @LastEditTime: 2021-08-10 20:59:36
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \IAR_OS\USER\PendSV.c
 */

#include "os.h"

void __set_pid_stack(uint32_t stack, uint32_t id)
{
    thread_information[id].stack_position = stack;
}

uint32_t __get_pid_stack(uint32_t id)
{
    return thread_information[id].stack_position;
}