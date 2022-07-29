/*
 * @Author: your name
 * @Date: 2021-07-15 10:59:22
 * @LastEditTime: 2021-07-15 14:46:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \IAR_OS\USER\svc.h
 */

#ifndef _SVC_H_
#define _SVC_H_
#include <stdio.h>
//#include "usart.h"
#include "os.h"

void SVC_Handler_server(unsigned int *stack);

#endif
