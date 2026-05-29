#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MEMORY_SIZE 2048
#define INSTRUCTION_MEM_END 1023
#define DATA_MEM_START 1024

#define NUM_REGISTERS 32 // 31 GPRs + R0 (zero register), PC handled separately

#define OP_ADD  0
#define OP_SUB  1
#define OP_MUL  2
#define OP_MOVI 3
#define OP_JEQ  4
#define OP_AND  5
#define OP_XORI 6
#define OP_JMP  7
#define OP_LSL  8
#define OP_LSR  9
#define OP_MOVR 10
#define OP_MOVM 11


#define FORMAT_R 0
#define FORMAT_I 1
#define FORMAT_J 2