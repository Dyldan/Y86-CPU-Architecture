/*
 * CS 261 PA4: Mini-ELF interpreter
 *
 * Name: Dylan Moreno
 */

#include "p4-interp.h"

char buffer[100]; // buffer array for iotraps

y86_reg_t get_reg(y86_t *cpu, y86_regnum_t reg_num);
bool get_cmov_cnd(y86_t *cpu, y86_cmov_t cmov);
bool get_jump_cnd(y86_t *cpu, y86_jump_t jump);
y86_reg_t op(y86_t *cpu, y86_inst_t inst, y86_reg_t valA, y86_reg_t valB);
void write_back(y86_t *cpu, y86_regnum_t reg, y86_reg_t val);
void iotrap(y86_t *cpu, y86_inst_t inst, byte_t *memory);
size_t inst_size(y86_inst_t inst);

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

y86_reg_t decode_execute (y86_t *cpu, y86_inst_t inst, bool *cnd, y86_reg_t *valA)
{
    // check for bad parameters
    if (cpu == NULL || cnd == NULL || valA == NULL || inst.icode > 0xc) {
        cpu->stat = INS;
        return 0;
    }

    y86_reg_t valE = 0; // return value
    y86_reg_t valB = 0;

    // decode + execute according to the instruction
    switch (inst.icode) {
        case HALT:
            cpu->stat = HLT;
            break;
        case NOP:
            break;
        case CMOV:
            *valA = get_reg(cpu, inst.ra);
            valE = *valA;
            *cnd = get_cmov_cnd(cpu, inst.ifun.cmov);
            break;
        case IRMOVQ:
            valE = inst.valC.v;
            break;
        case RMMOVQ:
            *valA = get_reg(cpu, inst.ra);
            valB = get_reg(cpu, inst.rb);
            valE = valB + inst.valC.d;
            break;
        case MRMOVQ:
            valB = get_reg(cpu, inst.rb);
            valE = valB + inst.valC.d;
            break;
        case OPQ:
            *valA = get_reg(cpu, inst.ra);
            valB = get_reg(cpu, inst.rb);
            valE = op(cpu, inst, *valA, valB);
            break;
        case JUMP:
            *cnd = get_jump_cnd(cpu, inst.ifun.jump);
            break;
        case CALL:
            valB = cpu->reg[RSP];
            valE = valB - 8;
            break;
        case RET:
            *valA = cpu->reg[RSP];
            valB = cpu->reg[RSP];
            valE = valB + 8;
            break;
        case PUSHQ:
            *valA = get_reg(cpu, inst.ra);
            valB = cpu->reg[RSP];
            valE = valB - 8;
            break;
        case POPQ:
            *valA = cpu->reg[RSP];
            valB = cpu->reg[RSP];
            valE = valB + 8;
            break;
        case IOTRAP: break;
        case INVALID: break;
    }

    return valE;
}

void memory_wb_pc (y86_t *cpu, y86_inst_t inst, byte_t *memory,
        bool cnd, y86_reg_t valA, y86_reg_t valE)
{
    // check for bad parameters
    if (cpu == NULL || memory == NULL || cpu->pc < 0 || valA < 0 || valE < 0) {
        cpu->stat = ADR;
        return;
    }

    y86_reg_t valM;
    uint64_t *mem_block;

    // perform last stages according to the instruction
    switch (inst.icode) {
        case HALT:
            cpu->zf = false;
            cpu->sf = false;
            cpu->of = false;
            cpu->pc = inst.valP;
            break;
        case NOP:
            cpu->pc = inst.valP;
            break;
        case CMOV:
            if (cnd) {
                write_back(cpu, inst.rb, valE);
            }
            cpu->pc = inst.valP;
            break;
        case IRMOVQ:
            write_back(cpu, inst.rb, valE);
            cpu->pc = inst.valP;
            break;
        case RMMOVQ:
            mem_block = (uint64_t*) &memory[valE];
            *mem_block = valA;
            cpu->pc = inst.valP;
            break;
        case MRMOVQ:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            mem_block = (uint64_t*) &memory[valE];
            valM = *mem_block;
            write_back(cpu, inst.ra, valM);
            cpu->pc = inst.valP;
            break;
        case OPQ:
            write_back(cpu, inst.rb, valE);
            cpu->pc = inst.valP;
            break;
        case JUMP:
            if (cnd) {
                cpu->pc = inst.valC.dest;
                break;
            }
            cpu->pc = inst.valP;
            break;
        case CALL:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            mem_block = (uint64_t*) &memory[valE];
            *mem_block = inst.valP;
            cpu->reg[RSP] = valE;
            cpu->pc = inst.valC.dest;
            break;
        case RET:
            mem_block = (uint64_t*) &memory[valA];
            valM = *mem_block;
            cpu->reg[RSP] = valE;
            cpu->pc = valM;
            break;
        case PUSHQ:
            mem_block = (uint64_t*) &memory[valE];
            *mem_block = valA;
            cpu->reg[RSP] = valE;
            cpu->pc = inst.valP;
            break;
        case POPQ:
            mem_block = (uint64_t*) &memory[valA];
            valM = *mem_block;
            cpu->reg[RSP] = valE;
            write_back(cpu, inst.ra, valM);
            cpu->pc = inst.valP;
            break;
        case IOTRAP:
            iotrap(cpu, inst, memory);
            cpu->pc = inst.valP;
        case INVALID:
            return;
    }
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p4 (char **argv)
{
    printf("Usage: %s <option(s)> mini-elf-file\n", argv[0]);
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");
    printf("  -d      Disassemble code contents\n");
    printf("  -D      Disassemble data contents\n");
    printf("  -e      Execute program\n");
    printf("  -E      Execute program (trace mode)\n");
}

bool parse_command_line_p4 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        bool *disas_code, bool *disas_data,
        bool *exec_normal, bool *exec_trace, char **filename)
{
    // check for bad parameters
    if (argc < 2 || header == NULL || segments == NULL ||
          membrief == NULL || memfull == NULL || disas_code == NULL ||
          disas_data == NULL || exec_normal == NULL || exec_trace == NULL) {
        usage_p4(argv);
        return false;
    }

    // ensure that parameter flags are declared to be false
    *header = false;
    *segments = false;
    *membrief = false;
    *memfull = false;
    *disas_code = false;
    *disas_data = false;
    *exec_normal = false;
    *exec_trace = false;

    // boolean flags
    bool h_selected = false;
    bool H_selected = false;
    bool a_selected = false;
    bool f_selected = false;
    bool s_selected = false;
    bool m_selected = false;
    bool M_selected = false;
    bool d_selected = false;
    bool D_selected = false;
    bool e_selected = false;
    bool E_selected = false;

    // parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hHafsmMdDeE")) != -1) {
        switch (opt) {
            case 'h': h_selected = true; break;
            case 'H': H_selected = true; break;
            case 'a': a_selected = true; break;
            case 'f': f_selected = true; break;
            case 's': s_selected = true; break;
            case 'm': m_selected = true; break;
            case 'M': M_selected = true; break;
            case 'd': d_selected = true; break;
            case 'D': D_selected = true; break;
            case 'e': e_selected = true; break;
            case 'E': E_selected = true; break;
            default: usage_p4(argv); return false;
        }
    }

    /* set boolean flags based on combination flags
     * a = -H -m -s
     * f = -H -M -s
     */
    if (a_selected) {
        H_selected = true;
        m_selected = true;
        s_selected = true;
    }
    if (f_selected) {
        H_selected = true;
        M_selected = true;
        s_selected = true;
    }

    // act according to which options were selected
    if (h_selected) {
        usage_p4(argv);
        return true;
    }
    if (H_selected) {
        *header = true;
    }
    if (s_selected) {
        *segments = true;
    }
    if (m_selected) {
        *membrief = true;
    }
    if (M_selected) {
        *memfull = true;
    }
    if (d_selected) {
        *disas_code = true;
    }
    if (D_selected) {
        *disas_data = true;
    }
    if (e_selected) {
        *exec_normal = true;
    }
    if (E_selected) {
        *exec_trace = true;
    }

    // both -m and -M cannot be selected at the same time
    if (*membrief && *memfull) {
        *membrief = false;
        *memfull = false;
        usage_p4(argv);
        return false;
    }
    // both -e and -E cannot be selected at the same time
    if (*exec_normal && *exec_trace) {
        *exec_normal = false;
        *exec_trace = false;
        usage_p4(argv);
        return false;
    }

    // load filename
    *filename = argv[optind];
    if (*filename == NULL || argc > optind + 1) {
        usage_p4(argv);
        *filename = NULL;
        return false;
    }

    // return true if valid options were selected
    // note that -h is a special case
    if (H_selected || s_selected || m_selected || M_selected || d_selected ||
          D_selected || e_selected || E_selected) {
        return true;
    }

    return false;
}

void dump_cpu_state (y86_t cpu)
{
    // print flags
    printf("Y86 CPU state:\n");
    printf("  %%rip: %016lx   flags: Z%d S%d O%d     ", cpu.pc, cpu.zf, cpu.sf, cpu.of);

    // print cpu status
    switch(cpu.stat) {
        case(1): printf("AOK\n"); break;
        case(2): printf("HLT\n"); break;
        case(3): printf("ADR\n"); break;
        case(4): printf("INS\n"); break;
    }

    // print registers
    printf("  %%rax: %016lx    %%rcx: %016lx\n", get_reg(&cpu, RAX), get_reg(&cpu, RCX));
    printf("  %%rdx: %016lx    %%rbx: %016lx\n", get_reg(&cpu, RDX), get_reg(&cpu, RBX));
    printf("  %%rsp: %016lx    %%rbp: %016lx\n", get_reg(&cpu, RSP), get_reg(&cpu, RBP));
    printf("  %%rsi: %016lx    %%rdi: %016lx\n", get_reg(&cpu, RSI), get_reg(&cpu, RDI));
    printf("   %%r8: %016lx     %%r9: %016lx\n", get_reg(&cpu, R8), get_reg(&cpu, R9));
    printf("  %%r10: %016lx    %%r11: %016lx\n", get_reg(&cpu, R10), get_reg(&cpu, R11));
    printf("  %%r12: %016lx    %%r13: %016lx\n", get_reg(&cpu, R12), get_reg(&cpu, R13));
    printf("  %%r14: %016lx\n", get_reg(&cpu, R14));
}

/**********************************************************************
 *                         HELPER FUNCTIONS
 *********************************************************************/

/**
 * retrieve the value of the specified register of the given cpu. Function exists for the purpose of future-proofing.
 */
y86_reg_t get_reg(y86_t *cpu, y86_regnum_t reg_num)
{
    return cpu->reg[reg_num];
}

/**
 * check cmov condition
 */
bool get_cmov_cnd(y86_t *cpu, y86_cmov_t cmov)
{
    switch (cmov) {
        case RRMOVQ:
            return true;
        case CMOVLE:
            if (cpu->zf || (cpu->sf ^ cpu->of)) {
                return true;
            }
            break;
        case CMOVL:
            if (cpu->sf ^ cpu->of) {
                return true;
            }
            break;
        case CMOVE:
            if (cpu->zf) {
                return true;
            }
            break;
        case CMOVNE:
            if (!cpu->zf) {
                return true;
            }
            break;
        case CMOVGE:
            if (cpu->sf == cpu->of) {
                return true;
            }
            break;
        case CMOVG:
            if (!cpu->zf && cpu->sf == cpu->of) {
                return true;
            }
            break;
        case BADCMOV:
            cpu->stat = INS;
            break;
    }

    return false;
}

/**
 * check jump condition
 */
bool get_jump_cnd(y86_t *cpu, y86_jump_t jump)
{
    switch (jump) {
        case JMP:
            return true;
        case JLE:
            if (cpu->zf || (cpu->sf ^ cpu->of)) {
                return true;
            }
            break;
        case JL:
            if (cpu->sf ^ cpu->of) {
                return true;
            }
            break;
        case JE:
            if (cpu->zf) {
                return true;
            }
            break;
        case JNE:
            if (!cpu->zf) {
                return true;
            }
            break;
        case JGE:
            if (cpu->sf == cpu->of) {
                return true;
            }
            break;
        case JG:
            if (!cpu->zf && cpu->sf == cpu->of) {
                return true;
            }
            break;
        case BADJUMP:
            cpu->stat = INS;
            break;
    }

    return false;
}

/**
 * act according to the option (add, sub, and, xor)
 */
y86_reg_t op(y86_t *cpu, y86_inst_t inst, y86_reg_t valA, y86_reg_t valB)
{
    y86_reg_t valE = 0;
    int64_t sigValE = 0;
    int64_t sigValB = valB;
    int64_t sigValA = valA;
    
    switch(inst.ifun.op) {
        case(ADD):
            sigValE = sigValB + sigValA;
            // set overflow flag for addition
            cpu->of = (sigValB < 0 && sigValA < 0 && sigValE > 0) || (sigValB > 0 && sigValA > 0 && sigValE < 0);
            valE = sigValE;
            break;
        case(SUB):
            sigValE = sigValB - sigValA;
            // set overflow flag for subtraction
            cpu->of = ((sigValB < 0 && sigValA > 0 && sigValE > 0) || (sigValB > 0 && sigValA < 0 && sigValE < 0 )); 
            valE = sigValE;
            break;
        case(AND):
            valE = valB & valA;
            break;
        case(XOR):
            valE = valA ^ valB;
            break;
        case(BADOP):
            cpu->stat = INS;
            return valE;
    }
    
    // set sign and zero flags
    cpu->sf = (valE >> 63 == 1);
    cpu->zf = (valE == 0);

    return valE;
}

/**
 * write the value to the specified register. Function exists for the purpose of future-proofing.
 */
void write_back(y86_t *cpu, y86_regnum_t reg, y86_reg_t val)
{
    cpu->reg[reg] = val;
}

/**
 * handle input/output dependent on the trap ID
 */
void iotrap(y86_t *cpu, y86_inst_t inst, byte_t *memory)
{
    // what the frick is this
    switch (inst.ifun.trap) {
        case CHAROUT: // 0
            break;
        case CHARIN: // 1
            scanf("%c", &memory[RDI]);
            break;
        case DECOUT: // 2
            //snprintf(buffer, sizeof(int64_t), "%d", (int) &memory[RSI]);
            break;
        case DECIN:; // 3
            int input;
            int result = scanf("%d", &input);
            if (result == EOF || result == 0) {
                printf("I/O Error");
                cpu->stat = HLT;
                break;
            }
            memory[RDI] = input;
            break;
        case STROUT: // 4
            break;
        case FLUSH: // 5
            fwrite(buffer, sizeof(char), 100, stdout);
            memset(buffer, 0, sizeof(char));
            break;
        case BADTRAP:
            printf("I/O Error");
            cpu->stat = HLT;
            return;
    }
}

/**
 * get the byte size of the instruction
 */
size_t inst_size(y86_inst_t inst)
{
    switch (inst.icode) {
        case INVALID:
            return 0;
        case HALT:
        case NOP:
        case RET:
        case IOTRAP:
            return 1; // 1 byte instructions
        case CMOV:
        case OPQ:
        case PUSHQ:
        case POPQ:
            return 2; // 2 byte instructions
        case JUMP:
        case CALL:
            return 9; // 9 byte instructions
        default:
            return 10; // 10 byte instructions
    }
}
