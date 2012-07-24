#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>

//VM library functions
void vm_reset();
void vm_step();
uint8_t vm_running(); //once 'halt' is called the CPU stops and this returns 0
void vm_interrupt(uint8_t interrupt);

#endif //_VM_H__
