/*
 * CS 261 PA3: Mini-ELF disassembler
 *
 * Name: Dylan Moreno
 */

#include "p3-disas.h"

void print_spaces();
void print_reg(y86_reg_t reg);
size_t inst_size(y86_inst_t ins);

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

y86_inst_t fetch(y86_t *cpu, byte_t *memory)
{
    y86_inst_t ins;

    // check for bad parameters
    if (memory == NULL || cpu == NULL || cpu->pc < 0 || cpu->pc >= MEMSIZE) {
        ins.icode = INVALID;
        cpu->stat = ADR;
        return ins;
    }

    // get opcode
    ins.icode  = memory[cpu->pc] >> 4;    // hi- order 4 bits of first byte
    ins.ifun.b = memory[cpu->pc] & 0x0f;  // low-order 4 bits of first byte

    // check for invalid opcode
    if (ins.icode > 0xc || ins.ifun.b > 6) {
        ins.icode = INVALID;
        cpu->stat = INS;
        return ins;
    }

    // calculates address of next instruction (and checks validity)
    switch (ins.icode) {
        // one-byte instr
        case HALT:
        case NOP:
        case RET:
            if (ins.ifun.b != 0) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 1;
            break;
        case IOTRAP:
            if (ins.ifun.b > 5) { // 5 is highest valid ID
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 1;
            break;
        // two-byte instr
        case CMOV:
            ins.ra = memory[cpu->pc + 1] >> 4;
            ins.rb = memory[cpu->pc + 1] & 0x0f;
            // 6 is highest valid cmov
            if (ins.ifun.cmov > 6 || ins.ra == NOREG || ins.rb == NOREG) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            if (cpu->pc < 0 || cpu->pc + 2 >= MEMSIZE) {
                ins.icode = INVALID;
                cpu->stat = ADR;
                return ins;
            }
            ins.valP = cpu->pc + 2;
            break;
        case OPQ:
            ins.ra = memory[cpu->pc + 1] >> 4;
            ins.rb = memory[cpu->pc + 1] & 0x0f;
            // 3 is highest valid opq
            if (ins.ifun.op > 3) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 2;
            break;
        case PUSHQ:
        case POPQ:
            ins.ra = memory[cpu->pc + 1] >> 4;
            ins.rb = memory[cpu->pc + 1] & 0x0f;
            if (ins.ifun.b != 0 || ins.ra == 0xf || ins.rb != 0xf) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 2;
            break;
        // nine-byte instr
        case JUMP: ;
            uint64_t dest1 = 0;
            for (int i = 8; i >= 1; i--) {
                dest1 = dest1 << 8;
                dest1 += memory[cpu->pc + i];
            }
            ins.valC.dest = dest1;
            if (ins.ifun.jump > 6) { // 6 is highest valid jump
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 9;
            break;
        case CALL: ;
            uint64_t dest2 = 0;
            for (int i = 8; i >= 1; i--) {
                dest2 = dest2 << 8;
                dest2 += memory[cpu->pc + i];
            }
            ins.valC.dest = dest2;
            if (ins.ifun.b != 0) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            if (cpu->pc < 0 || cpu->pc + 9 >= MEMSIZE) {
                ins.icode = INVALID;
                cpu->stat = ADR;
                return ins;
            }
            ins.valP = cpu->pc + 9;
            break;
        // ten-byte instr
        case IRMOVQ:
            ins.rb = memory[cpu->pc + 1] & 0x0f;
            uint64_t v = 0;
            for (int i = 9; i >= 2; i--) {
                v = v << 8;
                v += memory[cpu->pc + i];
            }
            ins.valC.v = v;
            if (ins.ifun.b != 0 || (memory[cpu->pc + 1] >> 4) != 0xf) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 10;
            break;
        case RMMOVQ:
            ins.ra = memory[cpu->pc + 1] >> 4;
            ins.rb = memory[cpu->pc + 1] & 0x0f;
            uint64_t d1 = 0;
            for (int i = 9; i >= 2; i--) {
                d1 = d1 << 8;
                d1 += memory[cpu->pc + i];
            }
            ins.valC.d = d1;
            if (ins.ifun.b != 0) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            if (cpu->pc < 0 || cpu->pc + 10 >= MEMSIZE) {
                ins.icode = INVALID;
                cpu->stat = ADR;
                return ins;
            }
            ins.valP = cpu->pc + 10;
            break;
        case MRMOVQ:
            ins.ra = memory[cpu->pc + 1] >> 4;
            ins.rb = memory[cpu->pc + 1] & 0x0f;
            uint64_t d2 = 0;
            for (int i = 9; i >= 2; i--) {
                d2 = d2 << 8;
                d2 += memory[cpu->pc + i];
            }
            ins.valC.d = d2;
            if (ins.ifun.b != 0) {
                ins.icode = INVALID;
                cpu->stat = INS;
                return ins;
            }
            ins.valP = cpu->pc + 10;
            break;
        case INVALID:
            break;
    }

    return ins;
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p3(char **argv)
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
}

bool parse_command_line_p3(int argc, char **argv,
                           bool *print_header, bool *print_segments,
                           bool *print_membrief, bool *print_memfull,
                           bool *disas_code, bool *disas_data,
                           char **filename)
{
    // check for bad parameters
    if (argc < 2 || print_header == NULL || print_segments == NULL ||
          print_membrief == NULL || print_memfull == NULL ||
          disas_code == NULL || disas_data == NULL) {
        usage_p3(argv);
        return false;
    }

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

    // ensure that parameter flags are initialized to false
    *print_header   = false;
    *print_segments = false;
    *print_membrief = false;
    *print_memfull  = false;
    *disas_code     = false;
    *disas_data     = false;

    // parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hHafsmMdD")) != -1) {
        switch (opt) {
            case 'h':  h_selected = true;  break;
            case 'H':  H_selected = true;  break;
            case 'a':  a_selected = true;  break;
            case 'f':  f_selected = true;  break;
            case 's':  s_selected = true;  break;
            case 'm':  m_selected = true;  break;
            case 'M':  M_selected = true;  break;
            case 'd':  d_selected = true;  break;
            case 'D':  D_selected = true;  break;
            default :  usage_p3(argv);     return false;
        }
    }

    // set boolean flags based off of the combination flags
    // a = -H -m -s
    // f = -H -M -s
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
        usage_p3(argv);
        return true;
    }
    if (H_selected) {
        *print_header = true;
    }
    if (s_selected) {
        *print_segments = true;
    }
    if (m_selected) {
        *print_membrief = true;
    }
    if (M_selected) {
        *print_memfull = true;
    }
    if (d_selected) {
        *disas_code = true;
    }
    if (D_selected) {
        *disas_data = true;
    }

    // both -m and -M cannot be selected at the same time
    if (*print_membrief && *print_memfull) {
        *print_membrief = false;
        *print_memfull = false;
        usage_p3(argv);
        return false;
    }

    // load filename
    *filename = argv[optind];
    if (*filename == NULL || argc > optind + 1) {
        usage_p3(argv);
        *filename = NULL;
        return false;
    }

    // return true if valid options were selected
    if (H_selected || s_selected || m_selected || M_selected || d_selected || D_selected) {
        return true;
    }

    return false;
}

void disassemble(y86_inst_t inst)
{
    // switch on icode
    switch (inst.icode) {
        // one-byte instr
        case HALT:    printf("halt");  break;
        case NOP:     printf("nop");   break;
        case RET:     printf("ret");   break;
        case IOTRAP:  printf("iotrap ");
            switch (inst.ifun.trap) {
                case CHAROUT:  printf("0");  break;
                case CHARIN:   printf("1");  break;
                case DECOUT:   printf("2");  break;
                case DECIN:    printf("3");  break;
                case STROUT:   printf("4");  break;
                case FLUSH:    printf("5");  break;
                case BADTRAP:  return;
            }   break;
        // two-byte instr
        case CMOV:
            switch (inst.ifun.cmov) {
                case RRMOVQ:   printf("rrmovq");  break;
                case CMOVLE:   printf("cmovle");  break;
                case CMOVL:    printf("cmovl");   break;
                case CMOVE:    printf("cmove");   break;
                case CMOVNE:   printf("cmovne");  break;
                case CMOVGE:   printf("cmovge");  break;
                case CMOVG:    printf("cmovg");   break;
                case BADCMOV:  return;
            }
            printf(" ");
            print_reg(inst.ra);
            printf(", ");
            print_reg(inst.rb);
            break;
        case OPQ:
            switch (inst.ifun.op) {
                case ADD:      printf("addq");  break;
                case SUB:      printf("subq");  break;
                case AND:      printf("andq");  break;
                case XOR:      printf("xorq");  break;
                case BADOP:    return;
            }
            printf(" ");
            print_reg(inst.ra);
            printf(", ");
            print_reg(inst.rb);
            break;
        case PUSHQ:  printf("pushq ");  print_reg(inst.ra);  break;
        case POPQ:   printf("popq ");   print_reg(inst.ra);  break;
        // nine-byte instr
        case JUMP:
            switch (inst.ifun.jump) {
                case JMP:      printf("jmp");  break;
                case JLE:      printf("jle");  break;
                case JL:       printf("jl");   break;
                case JE:       printf("je");   break;
                case JNE:      printf("jne");  break;
                case JGE:      printf("jge");  break;
                case JG:       printf("jg");   break;
                case BADJUMP:  return;
            }
            printf(" %#lx", inst.valC.dest);
            break;
        case CALL:
            printf("call %#lx", inst.valC.dest);
            break;
        // ten-byte instrr
        case IRMOVQ:
            printf("irmovq ");
            printf("%#lx, ", inst.valC.v);
            print_reg(inst.rb);
            break;
        case RMMOVQ:
            printf("rmmovq ");
            print_reg(inst.ra);
            printf(", %#lx", inst.valC.d);
            if (inst.rb != 0xf) {
                printf("(");
                print_reg(inst.rb);
                printf(")");
            }
            break;
        case MRMOVQ:
            printf("mrmovq ");
            printf("%#lx", inst.valC.d);
            if (inst.rb != 0xf) {
                printf("(");
                print_reg(inst.rb);
                printf("), ");
            } else {
                printf(", ");
            }
            print_reg(inst.ra);
            break;
        case INVALID:
            break;
    }
}

void disassemble_code(byte_t *memory, elf_phdr_t *phdr, elf_hdr_t *hdr)
{
    // check for bad parameters
    if (memory == NULL || phdr == NULL || hdr == NULL) {
        return;
    }

    y86_t cpu;         // simulated CPU to hold PC
    y86_inst_t ins;    // struct to hold fetched instruction

    // start at beginning of the segment
    cpu.pc = phdr->p_vaddr;

    // print segment info
    printf("  0x%03lx:%29s0x%03lx code\n", cpu.pc, "| .pos ", cpu.pc);

    // iterate through the segment one instruction at a time
    while (cpu.pc < phdr->p_vaddr + phdr->p_filesz) {

        // print start label if pc is at hdr entry
        if (cpu.pc == hdr->e_entry) {
            printf("  0x%03lx:%31s\n", cpu.pc, "| _start:");
        }

        // 1. fetch instruction
        ins = fetch(&cpu, memory);
        size_t size = inst_size(ins);

        // 2. print disassembly ONLY if the instruction is valid
        if (ins.icode == INVALID || (size != 1 && size != 2 && size != 9 && size != 10)) {
            printf("Invalid opcode: 0x%x%x\n\n", 0xf, ins.ifun.b);
            cpu.pc += size;
            return;
        }

        printf("  0x%03lx: ", cpu.pc);
        for (int i = cpu.pc; i < cpu.pc + size; i++) {
            printf("%02x", memory[i]);
        }
        print_spaces(size);
        printf(" |   ");
        disassemble(ins);
        printf("\n");

        // 3. update PC (for next instruction)
        cpu.pc = ins.valP;
    }
    printf("\n");
}

void disassemble_data(byte_t *memory, elf_phdr_t *phdr)
{
    // check for bad parameters
    if (memory == NULL || phdr == NULL) {
        return;
    }

    // create a cpu program counter for looping
    y86_t cpu;
    int addr = phdr->p_vaddr;
    int filesz = phdr->p_filesz;
    cpu.pc = addr;

    // print segment info
    printf("  0x%03lx:%29s%#03lx data\n", cpu.pc, "| .pos ", cpu.pc);

    // iterate through the segment one piece at a time
    while (cpu.pc < addr + filesz) {

        printf("  %#03lx: ", cpu.pc);
        for (int i = 0; i < 8; i++) {
            printf("%02x", memory[cpu.pc + i]);
        }

        address_t *data = (address_t*)&memory[cpu.pc];
        printf("%15s", "|   .quad ");
        printf("%p", (void*)*data);
        cpu.pc += 8;
        printf("\n");
    }
    printf("\n");
}

void disassemble_rodata(byte_t *memory, elf_phdr_t *phdr)
{
    // check for bad parameters
    if (memory == NULL || phdr == NULL) {
        return;
    }

    // declare variables
    y86_t cpu;
    uint32_t addr = phdr->p_vaddr;
    uint32_t filesz = phdr->p_filesz;
    cpu.pc = addr;
    bool all_printed = false; // tracks if bytes will continue on next line

    // print segment info
    printf("  0x%03lx:%29s%#03lx rodata\n", cpu.pc, "| .pos ", cpu.pc);

    // loop through this segment and print each string
    while (cpu.pc <  addr + filesz) {

        all_printed = false;

        printf("  0x%03lx: ", (uint64_t) cpu.pc);

        // prints first ten bytes (or less if string ends)
        for (int i = cpu.pc; i < cpu.pc + 10; i++) {

            printf("%02x", memory[i]);
            if (memory[i] == 0x00) {
                all_printed = true;
                print_spaces(i - (cpu.pc -1));
                break;
            }

        }

        // print bytes as characters
        int incr = cpu.pc;
        printf(" |   .string \"");
        while (memory[incr] != 0x00) {
            printf("%c", memory[incr++]);
        }
        printf("\"");

        // calculate the number of bytes in this string
        int bytes = (incr - cpu.pc) + 1;

        // if string > 10 bytes, print rest of bytes on next line
        if (!all_printed) {

            printf("\n");

            // offset bytes by the first ten already printed
            incr = cpu.pc + 10;
            int offset = 0;
            printf("  0x%03lx: ", (uint64_t) incr);

            // loop until all bytes are printed
            while (memory[incr] != 0x00) {
                printf("%02x", memory[incr++]);
                // go to next line once 10 more bytes are printed
                if (++offset % 10 == 0) {
                    printf(" | \n  0x%03lx: ", (uint64_t) incr);
                }
            }
            printf("%02x", memory[incr++]);
            print_spaces((offset  % 10) + 1);
            printf(" | ");
        }

        cpu.pc += bytes; // update pc
        printf("\n");
    }
    printf("\n");
}

void print_spaces(int num_printed)
{
    int spaces = 10 - num_printed;
    for (int i = 0; i < spaces; i++) {
        printf("  ");
    }
}

void print_reg(y86_reg_t reg)
{
    switch(reg) {
        case RAX:
            printf("%%rax");
            break;
        case RCX:
            printf("%%rcx");
            break;
        case RDX:
            printf("%%rdx");
            break;
        case RBX:
            printf("%%rbx");
            break;
        case RSP:
            printf("%%rsp");
            break;
        case RBP:
            printf("%%rbp");
            break;
        case RSI:
            printf("%%rsi");
            break;
        case RDI:
            printf("%%rdi");
            break;
        case R8:
            printf("%%r8");
            break;
        case R9:
            printf("%%r9");
            break;
        case R10:
            printf("%%r10");
            break;
        case R11:
            printf("%%r11");
            break;
        case R12:
            printf("%%r12");
            break;
        case R13:
            printf("%%r13");
            break;
        case R14:
            printf("%%r14");
            break;
    }
}

size_t inst_size(y86_inst_t ins)
{
    switch (ins.icode) {
        case INVALID:
            return 0;
        case HALT:
        case NOP:
        case RET:
        case IOTRAP:
            return 1; // 1 byte instruction
        case CMOV:
        case OPQ:
        case PUSHQ:
        case POPQ:
            return 2; // 2 byte instruction
        case JUMP:
        case CALL:
            return 9; // 9 byte instruction
        default:
            return 10; // 10 byte instruction
    }
}
