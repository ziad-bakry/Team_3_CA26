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


void load_program(const char* filename) { //filename depends on the name of the actual instruction file
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("ERROR: Cannot open file '%s'\n", filename);
        exit(1);
    }

    char line[100];
    int addr = 0;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') // byshel el empty lines bs
            continue;

        if (addr > INSTRUCTION_MEM_END) {
            printf("ERROR: Too many instructions (max 1024)\n");
            exit(1);
        }

        memory[addr] = encode_instruction(line);
        addr++;
        total_instructions++;
    }

    fclose(f);
    printf("Loaded %d instructions.\n\n", total_instructions);
}


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


void stage_fetch() {
    if (PC > INSTRUCTION_MEM_END) {  /*1024 limit*/
        IF_reg.valid = 0;
        return;
    }


    if (memory[PC] == 0 && PC >= total_instructions) { /*eg 5 instruction so mesh bast3ml whole memory*/
        IF_reg.valid = 0;
        return;
    }

    int fetched_pc = PC;
    int32_t raw = memory[PC];
    PC++;

    IF_reg.raw_instruction = raw ;
    IF_reg.pc_of_instruction = fetched_pc;
    IF_reg.valid = 1;

    printf("  [IF] PC=%d | Raw=0x%08X | Fetched\n",
           fetched_pc, (uint32_t)raw); /*u yaani unsigned*/
}


void stage_decode() {
    if (!ID_reg.valid) {
        printf("  [ID] Empty\n");
        return;
    }

    ID_reg.ID_cycles_done++;

    if (ID_reg.ID_cycles_done == 1) {
        DecodedInstruction d = decode_raw(ID_reg.raw_instruction, ID_reg.pc_of_instruction);
        d.ID_cycles_done = 1;
        ID_reg = d;
        printf("  [ID] Cycle 1/2 - Decoding opcode=%d\n", ID_reg.opcode);
        return;
    }

    //data hazards mi4 handled fa lw el register mi4 updated 7nsta5dem el value el 2adema
    ID_reg.vr1= registers[ID_reg.r1];
    ID_reg.vr2= registers[ID_reg.r2];
    ID_reg.vr3= registers[ID_reg.r3];


    printf("  [ID] Cycle 2/2 - Opcode=%d", ID_reg.opcode);

    if (ID_reg.format == FORMAT_R)
        printf(" | R1=R%d | R2=R%d(%d) | R3=R%d(%d) | SHAMT=%d\n",
               ID_reg.r1, ID_reg.r2, ID_reg.vr2, ID_reg.r3, ID_reg.vr3, ID_reg.shamt);
    else if (ID_reg.format == FORMAT_I)
        printf(" | R1=R%d | R2=R%d(%d) | IMM=%d\n",
               ID_reg.r1, ID_reg.r2, ID_reg.vr2, ID_reg.immediate);
    else
        printf(" | ADDR=%d\n", ID_reg.address);
}


void stage_execute()
{
    if (!EX_reg.valid) {
        printf("  [EX] Empty\n");
        return;
    }

    EX_reg.EX_cycles_done++;

    DecodedInstruction d = EX_reg;

    if (EX_reg.EX_cycles_done == 1)  {
        printf("  [EX] Cycle 1/2 - Processing opcode=%d\n", d.opcode);
        return;
    }

    printf("  [EX] Cycle 2/2 - ");

    int branch_taken  = 0;
    int branch_target = 0;

    switch (d.opcode)
    {
    case OP_ADD:
        EX_reg.result = d.vr2 + d.vr3;
        printf("ADD: R%d(%d) + R%d(%d) = %d\n", d.r2, d.vr2, d.r3, d.vr3, EX_reg.result);
        break;

    case OP_SUB:
        EX_reg.result = d.vr2 - d.vr3;
        printf("SUB: R%d(%d) - R%d(%d) = %d\n", d.r2, d.vr2, d.r3, d.vr3, EX_reg.result);
        break;

    case OP_MUL:
        EX_reg.result = d.vr2 * d.vr3;
        printf("MUL: R%d(%d) * R%d(%d) = %d\n", d.r2, d.vr2, d.r3, d.vr3, EX_reg.result);
        break;

    case OP_AND:
        EX_reg.result = d.vr2 & d.vr3;
        printf("AND: R%d(%d) & R%d(%d) = %d\n", d.r2, d.vr2, d.r3, d.vr3, EX_reg.result);
        break;

    case OP_XORI:
        EX_reg.result = d.vr2 ^ d.immediate;
        printf("XORI: R%d(%d) ^ %d = %d\n", d.r2, d.vr2, d.immediate, EX_reg.result);
        break;

    case OP_JEQ:
        if (d.vr1 == d.vr2)
        {
            branch_taken  = 1;
            branch_target = d.pc_of_instruction + 1 + d.immediate;
            printf("JEQ: R%d(%d) == R%d(%d) --> TAKEN --> jump to %d\n",
                   d.r1, d.vr1,
                   d.r2, d.vr2,
                   branch_target);
        } else {
            printf("JEQ: R%d(%d) != R%d(%d) --> NOT TAKEN\n",
                   d.r1, d.vr1,
                   d.r2, d.vr2);
        }
        break;

    case OP_MOVI:
        EX_reg.result = d.immediate;
        printf("MOVI: R%d = %d\n", d.r1, d.immediate);
        break;

    case OP_JMP:
        branch_taken  = 1;
        branch_target = (PC & 0xF0000000) | d.address;
        printf("JMP: Unconditional jump to %d\n", branch_target);
        break;

    case OP_LSL:
        EX_reg.result = (int32_t)((uint32_t)d.vr2 << d.shamt);
        printf("LSL: R%d(%d) << %d = %d\n", d.r2, d.vr2, d.shamt, EX_reg.result);
        break;

    case OP_LSR:
        EX_reg.result = (int32_t)((uint32_t)d.vr2 >> d.shamt);
        printf("LSR: R%d(%d) >>> %d = %d\n", d.r2, d.vr2, d.shamt, EX_reg.result);
        break;

    case OP_MOVR:
        EX_reg.mem_address = d.vr2 + d.immediate;
        printf("MOVR: mem_address = R%d(%d) + %d = %d\n", d.r2, d.vr2, d.immediate, EX_reg.mem_address);
        break;

    case OP_MOVM:
        EX_reg.mem_address = d.vr2 + d.immediate;
        EX_reg.result      = d.vr1;
        printf("MOVM: mem_address = R%d(%d) + %d = %d | value = R%d(%d)\n",
               d.r2, d.vr2, d.immediate, EX_reg.mem_address, d.r1, d.vr1);
        break;
    default:
        printf("ERROR: Unknown opcode %d reached execute stage\n",
               d.opcode);
        break;

    }


    if (branch_taken) {
        PC = branch_target;
        IF_reg.valid = 0;
        ID_reg.valid = 0;
        printf("  >> Branch taken: PC updated to %d | Pipeline flushed\n", PC);
    }
}

void stage_memory() {

    if (!MEM_reg.valid) {
        printf("  [MEM] Empty\n");
        return;
    }

    DecodedInstruction d = MEM_reg;

    if (d.opcode == OP_MOVR) {
        int actual_addr = d.mem_address + DATA_MEM_START;
        MEM_reg.result  = memory[actual_addr];
        printf("  [MEM] MOVR: Load MEM[%d] = %d into R%d\n",
               actual_addr, MEM_reg.result, d.r1);
    }
    else if (d.opcode == OP_MOVM) {
        int actual_addr     = d.mem_address + DATA_MEM_START;
        memory[actual_addr] = d.result; /*result calculated in execute movm*/
        printf("  [MEM] MOVM: Store %d into MEM[%d]\n",
               d.result, actual_addr);
        printf("  >> MEM[%d] = %d\n", actual_addr, d.result);
    }
    else {

        printf("  [MEM] No memory operation (opcode=%d)\n", d.opcode);
    }
}


void stage_writeback() {

    if (!WB_reg.valid) {
        printf("  [WB] Empty\n");
        return;
    }

    DecodedInstruction d = WB_reg;

    int writes_register = (d.opcode == OP_ADD  || d.opcode == OP_SUB  ||
                           d.opcode == OP_MUL  || d.opcode == OP_MOVI ||
                           d.opcode == OP_AND  || d.opcode == OP_XORI ||
                           d.opcode == OP_LSL  || d.opcode == OP_LSR  ||
                           d.opcode == OP_MOVR);

    if (writes_register) {
        if (d.r1 == 0) {
            printf("  [WB] Destination is R0 (zero register) --> no write\n");
        } else {
            registers[d.r1] = d.result;
            printf("  [WB] R%d = %d\n", d.r1, d.result);
        }
    } else {
        printf("  [WB] No register write (opcode=%d)\n", d.opcode);
    }
}



int is_mem_active_this_cycle() {
    return (MEM_reg.valid &&
            (MEM_reg.opcode == OP_MOVR || MEM_reg.opcode == OP_MOVM));
} /*3shan law mem active mesh hadkhol fetch*/


int pipeline_done() {
    if (clock_cycle == 1) return 0;

    int fetch_done = (PC >= total_instructions);
    int pipeline_empty = (!IF_reg.valid && !ID_reg.valid &&
                          !EX_reg.valid && !MEM_reg.valid);
    return fetch_done && pipeline_empty;
}

void run_pipeline() {

    while (!pipeline_done()) {

        printf("\n---------- Clock Cycle %d ----------\n", clock_cycle);


        WB_reg = MEM_reg;
        MEM_reg.valid = 0;

        if (EX_reg.valid && EX_reg.EX_cycles_done == 2) {
            MEM_reg = EX_reg;
            EX_reg.valid = 0;
        }

        if (!EX_reg.valid && ID_reg.valid && ID_reg.ID_cycles_done == 2) {
            EX_reg = ID_reg;
            ID_reg.valid = 0;
     }

        /* ID gets IF only when ID is empty */
        if (!ID_reg.valid && IF_reg.valid) {
            ID_reg       = IF_reg;
            IF_reg.valid = 0;
        }


        stage_writeback();
        stage_memory();
        stage_execute();
        stage_decode();

        if (clock_cycle % 2 == 1) { //odd
            if (!is_mem_active_this_cycle())
                stage_fetch();
            else
                printf("  [IF] Stalled (MEM using memory this cycle)\n");
        } else {
            if(!IF_reg.valid)
                printf("  [IF] Empty\n");
        }

        clock_cycle++;

    }
}




void print_final_state() {
    printf("\n");
    printf("--------FINAL PROCESSOR STATE--------"" \n");

    /* Print PC */
    printf("\nPC = %d\n", PC);

    printf("\n----Registers ----\n");
    for (int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%d = %d\n", i, registers[i]);
    }

    printf("\n----Instruction Memory (addresses 0 to %d)----\n",
           INSTRUCTION_MEM_END);
    for (int i = 0; i <= INSTRUCTION_MEM_END; i++) {
        if (memory[i] != 0) {
            printf("MEM[%d] = 0x%08X (%d)\n",
                   i, (uint32_t)memory[i], memory[i]);
        }
    }


    printf("\n----Data Memory (addresses %d to %d)----\n",
           DATA_MEM_START, MEMORY_SIZE - 1);
    for (int i = DATA_MEM_START; i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            printf("MEM[%4d] = %d\n", i, memory[i]);
        }
    }

    printf("\n----Simulation Complete: %d clock cycles----\n",
           clock_cycle - 1);
}


int main() {

    clock_cycle= 1;

    load_program("instructions.asm");

    printf("Starting pipeline simulation...\n");
    run_pipeline();

    print_final_state();
    return 0;
}