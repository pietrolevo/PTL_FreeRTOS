/*
 * PTL V1.0.0
 * Copyright (C) 2026 Group3.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include <stdint.h>

/* Symbols defined by the linker script */
extern uint32_t _sidata; /* Start address of initialized data in FLASH */
extern uint32_t _sdata;  /* Start address of data section in RAM */
extern uint32_t _edata;  /* End address of data section in RAM */
extern uint32_t _sbss;   /* Start address of BSS section in RAM */
extern uint32_t _ebss;   /* End address of BSS section in RAM */
extern uint32_t _estack; /* End of stack */

/* Main function prototype */
extern int main(void);

/* Standard system handlers */
void Reset_Handler(void);
void Default_Handler(void) { while(1); }

/* FreeRTOS specific Handlers */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

/* Weak aliases for unused system handlers. */
void NMI_Handler(void)        __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__ ((weak, alias("Default_Handler")));

/**
 * @brief Interrupt Vector Table for Cortex-M3
 * Located in the .isr_vector section as required by the linker script.
 */
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))&_estack,    /* 0x00: Initial Stack Pointer */
    Reset_Handler,               /* 0x04: Reset Handler */
    NMI_Handler,                 /* 0x08: Non-Maskable Interrupt */
    HardFault_Handler,           /* 0x0C: All-class Fault */
    MemManage_Handler,           /* 0x10: Memory Management Fault */
    BusFault_Handler,            /* 0x14: Pre-fetch or Memory Access Fault */
    UsageFault_Handler,          /* 0x18: Instruction Execution Fault */
    0, 0, 0, 0,                  /* 0x1C-0x28: Reserved */
    vPortSVCHandler,             /* 0x2C: SVC Handler */
    0, 0,                        /* 0x30-0x34: Reserved */
    xPortPendSVHandler,          /* 0x38: PendSV Handler */
    xPortSysTickHandler,         /* 0x3C: SysTick Handler */
};

/**
 * @brief Implementation of the Reset Handler.
 * This function runs immediately after the processor resets.
 */
void Reset_Handler(void) {
    /* 1. Copy initialized data from FLASH to RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* 2. Initialize the BSS section to zero in RAM */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* 3. Execute the application main function */
    main();

    /* Safety catch: If main() returns, loop forever */
    while (1);
}
