.section .data

.section .bss

.section	.text.SVC_Handler
.thumb
.syntax unified
.type	SVC_Handler, %function
.global SVC_Handler

SVC_Handler:
    TST LR,#4
    ITE NE
    MRSNE R0,PSP
    MRSEQ R0,MSP
    B   SVC_Handler_server