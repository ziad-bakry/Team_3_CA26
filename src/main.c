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


DecodedInstruction decode_raw(int32_t raw, int pc_of_instr) {
    DecodedInstruction d;
    d.valid             = 1;
    d.result            = 0;
    d.mem_address       = 0;
    d.ID_cycles_done = 0;
    d.EX_cycles_done = 0;
    d.pc_of_instruction = pc_of_instr;


    d.opcode = (raw >> 28) & 0xF;
    d.r1 = (raw >> 23) & 0x1F;  /* masking done in all but req in anyth other than opcode since andy values at their left */
    d.r2 = (raw >> 18) & 0x1F;
    d.r3 = (raw >> 13) & 0x1F;
    d.shamt = raw & 0x1FFF; /* hena msh benetar n shift ashan aslan el shamt fl a5er fa no reason to shift*/

    int32_t raw_imm = raw & 0x3FFFF;
    if (raw_imm & (1 << 17))
        d.immediate = raw_imm | 0xFFFC0000;
    else
        d.immediate = raw_imm;


    d.address = raw & 0x0FFFFFFF;

    // chooses the correct formate
    if (d.opcode == OP_ADD || d.opcode == OP_SUB || d.opcode == OP_MUL ||
        d.opcode == OP_AND || d.opcode == OP_LSL || d.opcode == OP_LSR)
        d.format = FORMAT_R;
    else if (d.opcode == OP_JMP)
        d.format = FORMAT_J;
    else
        d.format = FORMAT_I;

    return d;
}


int32_t encode_instruction(char* line) {
    char mnemonic[10];  /* ashan mayeb2oosh 3ala ad bzabty w ye7sal buffer overflow */
    int32_t encoded = 0;

    sscanf(line, "%s", mnemonic);

    /* R-FORMAT INSTRUCTIONS */

    if (strcmp(mnemonic, "ADD") == 0) {
        int r1, r2, r3;
        sscanf(line, "%s R%d R%d R%d", mnemonic, &r1, &r2, &r3);
        // kol register bya5od 5 bits, bn shift kol wa7ed lel makan beta3o
        encoded = (OP_ADD << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13);
    }
    else if (strcmp(mnemonic, "SUB") == 0) {
        int r1, r2, r3;
        sscanf(line, "%s R%d R%d R%d", mnemonic, &r1, &r2, &r3);
        encoded = (OP_SUB << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13);
    }
    else if (strcmp(mnemonic, "MUL") == 0) {
        int r1, r2, r3;
        sscanf(line, "%s R%d R%d R%d", mnemonic, &r1, &r2, &r3);
        encoded = (OP_MUL << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13);
    }
    else if (strcmp(mnemonic, "AND") == 0) {
        int r1, r2, r3;
        sscanf(line, "%s R%d R%d R%d", mnemonic, &r1, &r2, &r3);
        encoded = (OP_AND << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13);
    }
    else if (strcmp(mnemonic, "LSL") == 0) {
        int r1, r2, shamt;
        sscanf(line, "%s R%d R%d %d", mnemonic, &r1, &r2, &shamt);
        // bn enforce eno yeb2a 13 bits bas ashan law da5aly rakam kebyr maybawazsh el denya
        encoded = (OP_LSL << 28) | (r1 << 23) | (r2 << 18) | (shamt & 0x1FFF);
    }
    else if (strcmp(mnemonic, "LSR") == 0) {
        int r1, r2, shamt;
        sscanf(line, "%s R%d R%d %d", mnemonic, &r1, &r2, &shamt);
        encoded = (OP_LSR << 28) | (r1 << 23) | (r2 << 18) | (shamt & 0x1FFF);
    }
    /* I-FORMAT INSTRUCTIONS */

    else if (strcmp(mnemonic, "MOVI") == 0) {
        int r1;
        int32_t imm;
        sscanf(line, "%s R%d %d", mnemonic, &r1, &imm);
        encoded = (OP_MOVI << 28) | (r1 << 23) | (0 << 18) | (imm & 0x3FFFF); /* bn enforce eno yeb2a 18 bits basss ashan law da5aly rakam kebyr maybawazsh el denya */
    }
    else if (strcmp(mnemonic, "JEQ") == 0) {
        int r1, r2;
        int32_t imm;
        sscanf(line, "%s R%d R%d %d", mnemonic, &r1, &r2, &imm);
        /* el immediate hena byeb2a el offset beta3 el branch */
        encoded = (OP_JEQ << 28) | (r1 << 23) | (r2 << 18) | (imm & 0x3FFFF);
    }
    else if (strcmp(mnemonic, "XORI") == 0) {
        int r1, r2;
        int32_t imm;
        sscanf(line, "%s R%d R%d %d", mnemonic, &r1, &r2, &imm);
        encoded = (OP_XORI << 28) | (r1 << 23) | (r2 << 18) | (imm & 0x3FFFF);
    }
    else if (strcmp(mnemonic, "MOVR") == 0) {
        int r1, r2;
        int32_t imm;
        sscanf(line, "%s R%d R%d %d", mnemonic, &r1, &r2, &imm);
        encoded = (OP_MOVR << 28) | (r1 << 23) | (r2 << 18) | (imm & 0x3FFFF);
    }
    else if (strcmp(mnemonic, "MOVM") == 0) {
        int r1, r2;
        int32_t imm;
        sscanf(line, "%s R%d R%d %d", mnemonic, &r1, &r2, &imm);
        encoded = (OP_MOVM << 28) | (r1 << 23) | (r2 << 18) | (imm & 0x3FFFF);
    }

    /* J-FORMAT INSTRUCTIONS */

    else if (strcmp(mnemonic, "JMP") == 0) {
        int addr;
        sscanf(line, "%s %d", mnemonic, &addr);
        /* el address 28 bits - el 4 bits el fo2 begyebu mn el PC*/
        encoded = (OP_JMP << 28) | (addr & 0x0FFFFFFF);
    }
    else {
        printf("ERROR: Unknown instruction: %s\n", mnemonic);
        exit(1);
    }

    return encoded;
}