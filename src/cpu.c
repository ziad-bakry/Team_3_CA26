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

/* el struct de betmassel el instruction w kol el fields beta3etha
   ba3d ma te3melho decode mn el raw binary */
   
typedef struct {
    int valid;
    int32_t raw_instruction;
    int opcode;
    int r1;
    int r2;
    int r3;
    int shamt;
    int32_t immediate;
    int address;
    int format;
    int pc_of_instruction;
    int32_t result;
    int mem_address;

    int ID_cycles_done;
    int EX_cycles_done;

    int32_t vr1;
    int32_t vr2;
    int32_t vr3;
} DecodedInstruction;


int32_t memory[MEMORY_SIZE];
int32_t registers[NUM_REGISTERS];
int PC;

/* el pipeline registers - kol wa7ed by3abar 3an slot wa7da fel conveyor belt */
DecodedInstruction IF_reg;
DecodedInstruction ID_reg;
DecodedInstruction EX_reg;
DecodedInstruction MEM_reg;
DecodedInstruction WB_reg;



int clock_cycle;

int total_instructions;
