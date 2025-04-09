#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char type[3];      // "R", "I", "S", "SB", "U", "UJ"
    char operation[10]; // Instruction name (e.g., "add", "sub")
    uint8_t funct3;
    uint8_t funct7;
    uint8_t rs1;
    uint8_t rs2;
    uint8_t rd;
    
    // Fields for immediate values (for different instruction types)
    uint32_t imm31_12;  // for U-Type, UJ-Type
    uint32_t imm12_10;  // for SB-Type
    uint32_t imm11_5;   // for S-Type
    uint32_t imm4_0;    // for S-Type
    uint32_t imm20_10;  // for UJ-Type
} Instruction;

// A helper function to sign-extend an integer 'val' that is 'bits' wide into a 32-bit signed value.
static int32_t sign_extend(uint32_t val, int bits) {
    int shift = 32 - bits;
    return (int32_t)((val << shift) >> shift);
}


void decode_r_type(uint32_t instruction, Instruction *inst) {
    strcpy(inst->type, "R");

    inst->funct7 = (instruction >> 25) & 0x7F; // bits [25-31]
    inst->rs2 = (instruction >> 20) & 0x1F;    // bits [20-24]
    inst->rs1 = (instruction >> 15) & 0x1F;    // bits [15-19]
    inst->funct3 = (instruction >> 12) & 0x7;  // bits [12-14]
    inst->rd = (instruction >> 7) & 0x1F;      // bits [7-11]

    strcpy(inst->operation, "unknown");  // default

    // Determine operation based on funct3 and funct7
    if (inst->funct3 == 0) {
        if (inst->funct7 == 0x00) {
            strcpy(inst->operation, "add");
        } else if (inst->funct7 == 0x20) {
            strcpy(inst->operation, "sub");
        }
    } else if (inst->funct3 == 7 && inst->funct7 == 0x00) {
        strcpy(inst->operation, "and");
    } else if (inst->funct3 == 6 && inst->funct7 == 0x00) {
        strcpy(inst->operation, "or");
    } else if (inst->funct3 == 4 && inst->funct7 == 0x00) {
        strcpy(inst->operation, "xor");
    } else if (inst->funct3 == 1 && inst->funct7 == 0x00) {
        strcpy(inst->operation, "sll");
    } else if (inst->funct3 == 5) {
        if (inst->funct7 == 0x00) {
            strcpy(inst->operation, "srl");
        } else if (inst->funct7 == 0x20) {
            strcpy(inst->operation, "sra");
        }
    } else if (inst->funct3 == 2 && inst->funct7 == 0x00) {
        strcpy(inst->operation, "slt");
    } else if (inst->funct3 == 3 && inst->funct7 == 0x00) {
        strcpy(inst->operation, "sltu");
    }
}


// 
void decode_i_type(uint32_t instruction, Instruction *inst) {
    strcpy(inst->type, "I");

    inst->imm31_12 = (instruction >> 20) & 0xFFF; // bits [31-20]
    inst->rs1 = (instruction >> 15) & 0x1F;       // bits [15-19]
    inst->funct3 = (instruction >> 12) & 0x7;     // bits [12-14]
    inst->rd = (instruction >> 7) & 0x1F;         // bits [7-11]

    int32_t simm12 = sign_extend(inst->imm31_12, 12); // Sign-extend the 12-bit immediate
    strcpy(inst->operation, "unknown");  
    uint8_t opcode = instruction & 0x7F; // Extract opcode

    if (opcode == 0x03) {
        // This is a load (lb=000, lh=001, lw=010)
        if (inst->funct3 == 0) {
            strcpy(inst->operation, "lb");
        } else if (inst->funct3 == 1) {
            strcpy(inst->operation, "lh");
        } else if (inst->funct3 == 2) {
            strcpy(inst->operation, "lw");
        }
    } 
    else if (opcode == 0x67) {
        // This is jalr (funct3=000)
        strcpy(inst->operation, "jalr");
    }
    else {

        // opcode == 0x13 => I-type ALU
        switch (inst->funct3) {
            case 0x0: strcpy(inst->operation, "addi"); break;
            case 0x7: strcpy(inst->operation, "andi"); break;
            case 0x6: strcpy(inst->operation, "ori");  break;
            case 0x4: strcpy(inst->operation, "xori"); break;
            case 0x2: strcpy(inst->operation, "slti"); break;
            case 0x3: strcpy(inst->operation, "sltiu"); break;
            case 0x1: // slli
            {
                strcpy(inst->operation, "slli");
                break;
            }
            case 0x5:
            {
                // srli/srai => look at bit [30] to see if it's 0 or 1
                // bit[30] => (inst->imm31_12 >> 10) & 1
                uint8_t topBit = (inst->imm31_12 >> 10) & 1;
                if (topBit == 0) {
                    strcpy(inst->operation, "srli");
                } else {
                    strcpy(inst->operation, "srai");
                }
                break;
            }
            default: break;
        }
    }


    printf("Instruction Type: I\n");
    printf("Operation: %s\n", inst->operation);
    printf("Rs1: x%d\n", inst->rs1);
    printf("Rd: x%d\n", inst->rd);
    printf("Immediate: %d (or 0x%X)\n", simm12, (unsigned)(simm12 & 0xFFF));
}


void decode_s_type(uint32_t instruction, Instruction *inst) {
    strcpy(inst->type, "S");

    inst->imm11_5 = (instruction >> 25) & 0x7F; // bits [25-31]
    inst->rs2 = (instruction >> 20) & 0x1F;     // bits [20-24]
    inst->rs1 = (instruction >> 15) & 0x1F;     // bits [15-19]
    inst->funct3 = (instruction >> 12) & 0x7;   // bits [12-14]
    inst->imm4_0 = (instruction >> 7) & 0x1F;   // bits [7-11]

    uint32_t imm12 = (inst->imm11_5 << 5) | inst->imm4_0;
    int32_t simm12 = sign_extend(imm12, 12);

    strcpy(inst->operation, "unknown");
    if (inst->funct3 == 0x0) {
        strcpy(inst->operation, "sb");
    } else if (inst->funct3 == 0x1) {
        strcpy(inst->operation, "sh");
    } else if (inst->funct3 == 0x2) {
        strcpy(inst->operation, "sw");
    }

    printf("Instruction Type: S\n");
    printf("Operation: %s\n", inst->operation);
    printf("Rs1: x%d\n", inst->rs1);
    printf("Rs2: x%d\n", inst->rs2);
    printf("Immediate: %d (or 0x%X)\n", simm12, (unsigned)(simm12 & 0xFFF));
}


void decode_sb_type(uint32_t instruction, Instruction *inst) {
    strcpy(inst->type, "SB");

    inst->imm12_10 = (instruction >> 25) & 0x7F; // bits [25-31]
    inst->rs2 = (instruction >> 20) & 0x1F;     // bits [20-24]
    inst->rs1 = (instruction >> 15) & 0x1F;     // bits [15-19]
    inst->funct3 = (instruction >> 12) & 0x7;   // bits [12-14]
    inst->imm4_0 = (instruction >> 7) & 0x1F;   // bits [7-11]

    //Reconstruct the 13-bit branch immediate from bits in imm12_10 and imm4_0
    // Branch immediate format:
    //   imm[12]   = bit [31]
    //   imm[10:5] = bits [30:25]
    //   imm[4:1]  = bits [11:8]
    //   imm[11]   = bit [7]
    //   imm[0]    = 0
    uint32_t bit31   = (instruction >> 31) & 0x1;   // top bit
    uint32_t bits30_25 = (instruction >> 25) & 0x3F;
    uint32_t bits11_8  = (instruction >> 8)  & 0xF;
    uint32_t bit7    = (instruction >> 7)  & 0x1;

    uint32_t branch_imm = 0;
    branch_imm |= (bit31 << 12);
    branch_imm |= (bit7 << 11);
    branch_imm |= (bits30_25 << 5);
    branch_imm |= (bits11_8 << 1);

    int32_t sbranch_imm = sign_extend(branch_imm, 13);

    // Now handle beq(000), bne(001), blt(100), bge(101)
    strcpy(inst->operation, "unknown");
    switch (inst->funct3) {
        case 0: strcpy(inst->operation, "beq"); break;
        case 1: strcpy(inst->operation, "bne"); break;
        case 4: strcpy(inst->operation, "blt"); break;
        case 5: strcpy(inst->operation, "bge"); break;
        default: break;
    }

    // Print in the new format
    printf("Instruction Type: SB\n");
    printf("Operation: %s\n", inst->operation);
    printf("Rs1: x%d\n", inst->rs1);
    printf("Rs2: x%d\n", inst->rs2);
    printf("Immediate: %d (or 0x%X)\n", sbranch_imm, (unsigned)(sbranch_imm & 0x1FFF));
}


// Decode U-type instruction
void decode_u_type(uint32_t instruction, Instruction *inst) {
    strcpy(inst->type, "U");

    inst->imm31_12 = (instruction >> 12) & 0xFFFFF; // bits [31-12]
    inst->rd = (instruction >> 7) & 0x1F;           // bits [7-11]
    strcpy(inst->operation, "lui");

    // For completeness, you might decode both lui and auipc by opcode if needed.

    printf("Instruction Type: U\n");
    printf("Operation: %s\n", inst->operation);
    printf("Rd: x%d\n", inst->rd);

    int32_t simm20 = (int32_t)(inst->imm31_12 << 12);

    printf("Immediate: %d (or 0x%X)\n", simm20, (unsigned)simm20);
}

void decode_uj_type(uint32_t instruction, Instruction *inst) {
    strcpy(inst->type, "UJ");

    inst->imm20_10 = (instruction >> 12) & 0x7FF;  // bits [12-20]
    inst->rd = (instruction >> 7) & 0x1F;          // bits [7-11]

    // Determine operation (UJ-Type typically is "jal")
    strcpy(inst->operation, "jal");

    // Print in original format
    uint32_t bit31    = (instruction >> 31) & 0x1;
    uint32_t bits30_21= (instruction >> 21) & 0x3FF;
    uint32_t bit20    = (instruction >> 20) & 0x1;
    uint32_t bits19_12= (instruction >> 12) & 0xFF;

    uint32_t uj_imm = 0;
    uj_imm |= (bit31 << 20);
    uj_imm |= (bits19_12 << 12);
    uj_imm |= (bit20 << 11);
    uj_imm |= (bits30_21 << 1);

    int32_t suj_imm = sign_extend(uj_imm, 21);

    printf("Instruction Type: UJ\n");
    printf("Operation: %s\n", inst->operation);
    printf("Rd: x%d\n", inst->rd);
    printf("Immediate: %d (or 0x%X)\n", suj_imm, (unsigned)(suj_imm & 0x1FFFFF));
}

// Decode instruction based on opcode
void decode_instruction(uint32_t instruction) {
    Instruction inst;  // Create an instruction struct
    uint8_t opcode = instruction & 0x7F; // Extract opcode

    printf("Extracted opcode: %07b\n", opcode);


    switch (opcode) {
        case 0x33:  // 0110011 (R-Type)
            decode_r_type(instruction, &inst);

            // Print R-type fields
            printf("Instruction Type: %s\n", inst.type);
            printf("Operation: %s\n", inst.operation);
            printf("Rs1: x%d\n", inst.rs1);
            printf("Rs2: x%d\n", inst.rs2);
            printf("Rd: x%d\n", inst.rd);
            printf("Funct3: %d\n", inst.funct3);
            printf("Funct7: %d\n", inst.funct7);
            return;

        case 0x13: // I-type (ALU)
        case 0x03: // I-type (load)
        case 0x67: // I-type (jalr)
            decode_i_type(instruction, &inst);
            return;

        case 0x23: // S-type
            decode_s_type(instruction, &inst);
            return;

        case 0x63: // SB-type
            decode_sb_type(instruction, &inst);
            return;

        case 0x6F: // UJ-type (jal)
            decode_uj_type(instruction, &inst);
            return;

        case 0x37: // U-type (lui) [not in your test set, but we can handle]
        case 0x17: // U-type (auipc), if you want
            decode_u_type(instruction, &inst);
            return;

        default:
            printf("Unknown opcode.\n");
            return;
    }
}

int main() {
    char user_input[33]; // 32-bit binary + null terminator
    uint32_t instruction = 0;

    printf("Enter a 32-bit instruction in binary: ");
    scanf("%32s", user_input);  

    // Convert binary string to a 32-bit integer
    instruction = (uint32_t) strtol(user_input, NULL, 2);

    decode_instruction(instruction);

    return 0;
}