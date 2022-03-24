/*
 * CS 261: Main driver
 *
 * Name: Dylan Moreno
 */

#include "p1-check.h"
#include "p2-load.h"
#include "p3-disas.h"
#include "p4-interp.h"
#include <assert.h>

void terminate_bad();

int main (int argc, char **argv)
{
    bool header = false;
    bool segments = false;
    bool membrief = false;
    bool memfull = false;
    bool disas_code = false;
    bool disas_data = false;
    bool exec_normal = false;
    bool exec_trace = false;

    char *filename;
    FILE *file;

    elf_hdr_t hdr;

    // parse command line
    if (parse_command_line_p4(argc, argv, &header, &segments, &membrief,
                              &memfull, &disas_code, &disas_data,
                              &exec_normal, &exec_trace, &filename)) {
        // close if -h option selected
        if (!header && !segments && !membrief && !memfull && !disas_code
              && !disas_data && !exec_normal && !exec_trace) {
            return(EXIT_SUCCESS);
        }
    } else {
        // terminate if invalid arguments
        return(EXIT_FAILURE);
    }

    // open file and check for validity
    file = fopen(filename, "r");
    if (file == NULL) {
        terminate_bad();
    }

    // load the header and check if it is valid
    if (!read_header(file, &hdr)) {
        terminate_bad();
    }

    // allocate memory on the heap
    byte_t *memory = (byte_t*)calloc(MEMSIZE, sizeof(byte_t));
    assert(memory != NULL);

    // read program headers from file into an array of phdrs,
    // load those segments into virtual memory, and check for validity
    elf_phdr_t phdrs[hdr.e_num_phdr];
    int offset;
    for (int i = 0; i < hdr.e_num_phdr; i++) {
        offset = hdr.e_phdr_start + (i * sizeof(elf_phdr_t));
        if (!read_phdr(file, offset, &phdrs[i]) || !load_segment(file, memory, phdrs[i])) {
            free(memory);
            terminate_bad();
        }
    }

    // print the relevant info
    if (header) {
        dump_header(hdr);
    }
    if (segments) {
        dump_phdrs(hdr.e_num_phdr, phdrs);
    }
    if (membrief) {
        for (int i = 0; i < hdr.e_num_phdr; i++) {
            int vaddr = phdrs[i].p_vaddr;
            int filesz = phdrs[i].p_filesz;
            dump_memory(memory, vaddr, vaddr + filesz);
        }
    }
    if (memfull) {
        dump_memory(memory, 0, MEMSIZE);
    }
    if (disas_code) {
        printf("Disassembly of executable contents:\n");

        for (int i = 0; i < hdr.e_num_phdr; i++) {

            // print code segments only
            if (phdrs[i].p_type == CODE) {
                disassemble_code(memory, &phdrs[i], &hdr);
            }
        }
    }
    if (disas_data) {
        printf("Disassembly of data contents:\n");

        for (int i = 0; i < hdr.e_num_phdr; i++) {

            if (phdrs[i].p_type == DATA && phdrs[i].p_flag != 4) {
                // print non-read-only data
                disassemble_data(memory, &phdrs[i]);
            } else if (phdrs[i].p_type == DATA) {
                // print read-only data
                disassemble_rodata(memory, &phdrs[i]);
            }
        }
    }
    if (exec_normal) {
        y86_t cpu;
        memset(&cpu, 0x00, sizeof(cpu));
        cpu.stat = AOK;
        cpu.pc = hdr.e_entry;
        uint32_t count = 0;

        printf("Beginning execution at 0x%04x\n", hdr.e_entry);

        // loop until cpu status is not ok
        while (cpu.stat == AOK) {

            bool cnd = false;
            y86_reg_t valA = 0;
            y86_reg_t valE = 0;
            y86_inst_t inst;

            // fetch instruction
            inst = fetch(&cpu, memory);

            // only continue if cpu status is AOK
            if (cpu.stat == AOK) {
                // decode and execute instruction
                valE = decode_execute(&cpu, inst, &cnd, &valA);

                // write to memory, registers, and update upgram counter
                memory_wb_pc(&cpu, inst, memory, cnd, valA, valE);

                count++;
            }

            // increment pc if status became ADR between decode and pc steps
            if (cpu.stat == ADR) {
                cpu.pc += 10;
            }

            if (cpu.pc >= MEMSIZE) {
                cpu.stat = ADR;
            }

        }

        // dump cpu state
        dump_cpu_state(cpu);
        printf("Total execution count: %d\n", count);
    }
    if (exec_trace) {
        y86_t cpu;
        memset(&cpu, 0x00, sizeof(cpu));
        cpu.stat = AOK;
        cpu.pc = hdr.e_entry;
        uint32_t count = 0;

        printf("Beginning execution at 0x%04x\n", hdr.e_entry);

        // loop until cpu status is not ok
        while (cpu.stat == AOK) {

            bool cnd = false;
            y86_reg_t valA;
            y86_reg_t valE;
            y86_inst_t inst;

            // dump cpu state on each iteration
            dump_cpu_state(cpu);
            
            // fetch instruction
            inst = fetch(&cpu, memory);

            // only continue if cpu status is AOK
            if (cpu.stat == AOK) {
                // print instruction
                printf("\nExecuting: ");
                disassemble(inst);
                printf("\n");

                // decode and execute instruction
                valE = decode_execute(&cpu, inst, &cnd, &valA);

                // write to memory, registers, and update upgram counter
                memory_wb_pc(&cpu, inst, memory, cnd, valA, valE);

                count++;
            } else {
                printf("\nInvalid instruction at 0x%04lx\n", cpu.pc);
            }

            // increment pc if status became ADR between decode and pc steps
            if (cpu.stat == ADR) {
                cpu.pc += 10;
            }
            
            if (cpu.pc >= MEMSIZE) {
                cpu.stat = ADR;
            }

        }

        // dump cpu state
        dump_cpu_state(cpu);
        printf("Total execution count: %d\n\n", count);

        dump_memory(memory, 0, MEMSIZE);
    }

    fclose(file); // close the file
    free(memory); // free the heap memory
    memory = NULL; // safe practice

    return EXIT_SUCCESS;
}

/**
 * Helper method to print file failure message and terminate program.
 */
void terminate_bad()
{
    printf("Failed to read file\n");
    exit(EXIT_FAILURE);
}