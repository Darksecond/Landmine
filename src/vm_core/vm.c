#include <stdint.h>
#ifdef DBG_VM
#include <stdio.h> //for printf
#endif //DBG_VM

#include "vm.h"
#include "vm_ext.h"

//maximum amount of interrupts
#define MAX_INT 32

//amount of registers
//each register is 2 bytes long!
#define MAX_REG 256


uint16_t intr[MAX_INT]; //interrupts
int16_t reg[MAX_REG]; //registers

//these registers hold important values
#define PC reg[0]
#define SP reg[1]
#define FLAGS reg[2]

#define F_ZERO 1
#define F_CARRY 2
#define F_IE 4 //interrupt enabled
#define F_HLT 8 //halted


uint8_t vm_running()
{
	if((FLAGS & F_HLT))
	{
		return false;
	}
	else
	{
		return true;
	}
}

void vm_reset()
{
	PC = 0;
	//reserve the top few bytes for interrupt-counting?
	//or for something else?
	SP = 252; 
	FLAGS = 0;

	//reset interrupts
	for (uint8_t i = 0; i <= MAX_INT; i++)
	{
		intr[i] = 0;
	}
}

void vm_step()
{
#ifdef DBG_VM
	printf("PC=0x%04X ", PC);
#endif

	uint8_t num_args;
	uint8_t a1, a2, a3, a4;
	uint8_t opcode = vmext_getbyte(PC++);
	//determine amount of arguments
	if (opcode < 0x50) num_args = 3;
	if (opcode < 0x40) num_args = 2;
	if (opcode < 0x20) num_args = 1;
	if (opcode < 0x10) num_args = 0;
	
	if (num_args >= 1) a1 = vmext_getbyte(PC++); 
	if (num_args >= 2) a2 = vmext_getbyte(PC++); 
	if (num_args >= 3) a3 = vmext_getbyte(PC++); 
	if (num_args >= 4) a4 = vmext_getbyte(PC++); 

#ifdef DBG_VM
	printf("opcode=0x%02hhX", opcode);
	if (num_args >= 1) printf(" a1=0x%02hhi", a1);
	if (num_args >= 2) printf(" a2=0x%02hhi", a2);
	if (num_args >= 3) printf(" a3=0x%02hhi", a3);
	if (num_args >= 4) printf(" a4=0x%02hhi", a4);
	printf("\n");
#endif

	//prepare pp (long), a location in firmware, for eventual use
	if (num_args == 2) pp = (a1 << 8) + a2;
	if (num_args == 3) pp = (a2 << 8) + a3;
	if (num_args == 4) pp = (a3 << 8) + a4;

	switch(opcode)
	{
		case 0x01: //NOP
			break;
		case 0x02: //RET
			pop(PC);
			break;
		case 0x03: //RETI
			pop(PC);
			pop(FLAGS);
			break;
		case 0x04: //CLI
			FLAGS = FLAGS & (~F_IE);
			break;
		case 0x05: //STI
			FLAGS = FLAGS|F_IE;
			break;
		case 0x06: //HLT
			FLAGS = FLAGS|F_HLT;
			break;
		//*****************************************************
		case 0x10: //INC r1
			int32_t result = reg[a1] + 1;
			reg[a1] = result & 0xffff;
			chkzero(result); //TODO will this ever be zero?
			chkcarry(result & 0xf0000);
			break;
		case 0x11: //DEC r1
			int32_t result = reg[a1] - 1;
			reg[a1] = result & 0xffff;
			chkzero(result);
			chkcarry(result & 0xf0000);
			break;
		case 0x12: //PUSH r1
			push(reg[a1]);
			break;
		case 0x13: //POP r1
			reg[a1] = pop();
			break;
		//*****************************************************
		case 0x20: //MOV r1, r2
			reg[a1] = reg[a2];
			break;
		case 0x21: //MOV r1, [r2]
			reg[a1] = reg[(uint8_t)(reg[a2] & 255)];
			break;
		case 0x22: //MOV [r1], r2
			reg[(uint8_t)(reg[a1] & 255)] = reg[a2];
			break;
		case 0x23: //MOV r1, L
			reg[a1] = (int8_t)a2;
			break;
		case 0x24: //AND r1, r2
			reg[a1] = reg[a1] & reg[a2];
			chkzero(reg[a1]);
			break;
		case 0x25: //OR r1, r2
			reg[a1] = reg[a1] | reg[a2];
			chkzero(reg[a1]);
			break;
		case 0x26: //XOR r1, r2
			reg[a1] = reg[a1] ^ reg[a2];
			chkzero(reg[a1]);
			break;
		case 0x27: //MUL r1, r2
			int32_t result = reg[a1] * reg[a2];
			reg[a1] = result;
			chkcarry(result & 0xf0000);
			chkzero(result);
			break;
		case 0x28: //DIV r1, r2
			reg[a1] = reg[a1] / reg[a2];
			chkzero(reg[a1]);
			break;
		case 0x29: //ADD r1, r2
			int32_t result = reg[a1] + reg[a2];
			reg[a1] = result & 0xffff;
			chkcarry(result & 0xf0000);
			break;
		case 0x2A: //SUB r1, r2
			int32_t result = reg[a1] - reg[a2];
			reg[a1] = result & 0xffff;
			chkcarry(result & 0xf0000);
			chkzero(reg[a1]);
			break;
		case 0x2B: //JMP PP
			PC = pp;
			break;
		case 0x2C: //CALL PP
			push(PC);
			PC = pp;
			break;
		case 0x2D: //JE PP
			if ((FLAGS & F_ZERO))
			{
				PC = pp;
			}
			break;
		case 0x2E: //JNE PP
			if (!(FLAGS & F_ZERO))
			{
				PC = pp;;
			}
			break;
		case 0x2F: //JO PP
			if((FLAGS & F_CARRY))
			{
				PC = pp;
			}
			break;
		case 0x30: //JNO PP
			if(!(FLAGS & F_CARRY))
			{
				PC = pp;
			}
			break;
		case 0x31: //CMP r1, r2
			int32_t result = reg[a1] - reg[a2];
			chkzero(result);
			chkcarry(result & 0xf0000);
			break;
		case 0x32: //CMP r1, L
			int32_t result = reg[a1] - (int16_t)a2;
			chkzero(result);
			chkcarry(result & 0xf0000);
			break;
				case 0x17: //MOV r1, LL
			reg[a1] = (int16_t)pp;
			break;
		//*****************************************************
		case 0x40: //LPM r1, PP
			//LPM stands for: Load Program Memory
			reg[a1] = (vmext_getbyte(pp) << 8) + vmext_getbyte(pp+1);
			break;
		case 0x41: //AND r1, LL
			reg[a1] = reg[a1] & (int16_t)pp;
			chkzero(reg[a1]);
			break;
		case 0x42: //OR r1, LL
			reg[a1] = reg[a1] | (int16_t)pp;
			chkzero(reg[a1]);
			break;
		case 0x43: //XOR r1, LL
			reg[a1] = reg[a1] ^ (int16_t)pp;
			chkzero(reg[a1]);
			break;
		case 0x44: //ADD r1, LL
			int32_t result = reg[a1] + (int16_t)pp;
			reg[a1] = result & 0xffff;
			chkzero(result);
			chkcarry(result & 0xf0000);
			break;
		case 0x45: //SUB r1, LL
			int32_t result = reg[a1] - (int16_t)pp;
			reg[a1] = result & 0xffff;
			chkzero(result);
			chkcarry(result & 0xf0000);
			break;
		case 0x46: //CMP r1, LL
			int32_t result = reg[a1] - (int16_t)pp;
			chkzero(result);
			chkcarry(result & 0xf0000);
			break;
		case 0x47: //INT L, PP
			if (a1 <= MAX_INT)
			{
				intr[a1] = pp;
			}
			break;
	}
}

//UTILITY METHODS

int16_t pop()
{
	return reg[++SP];
}

void push(int16_t value)
{
	reg[SP--] = value;
}

//check if value is zero, and set the zero flag accordingly
void chkzero(int16_t value)
{
	if (value == 0)
	{
		FLAGS = FLAGS|F_ZERO;
	}
	else
	{
		FLAGS = FLAGS & (~F_ZERO);
	}
}

//check if value is not zero and set the carry flag accordingly
void chkcarry(int16_t value)
{
	if (value != 0)
	{
		FLAGS = FLAGS|F_CARRY;
	}
	else
	{
		FLAGS = FLAGS & (~F_CARRY);
	}
}
