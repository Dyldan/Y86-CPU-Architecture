/*
 * CS 261 PA2: Mini-ELF loader
 *
 * Name: Dylan Moreno
 */

#include "p2-load.h"

#define MAGIC 0xDEADBEEF
#define _R 4
#define _W 2
#define _X 1

/**
 * @brief Return the string representation of the given hexadecimal seg type
 * based on the elf_segtype_t enum.
 *
 * @param type_int Hexadecimal representation of the seg type
 * @returns The string representation of the type
 */
char* get_seg_type(int type_int);

/**
 * @brief Return the string representation of the given hexadecimal seg flag
 *
 * @param flag_int Hexadecimal representation of the flag
 * @returns The segment's flag as a string
 */
char* get_seg_flag(int flag_int);

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

bool read_phdr (FILE *file, uint16_t offset, elf_phdr_t *phdr)
{
    // check if phdr is null
    if (phdr == NULL) {
        return false;
    }

    // check if file is null
    if (file == NULL) {
        return false;
    }

    // seek to correct location in file, returning false if it is invalid
    if (fseek(file, offset, SEEK_SET) != 0) {
        return false;
    }

    // load the phdr
    fread(phdr, 1, sizeof(elf_phdr_t), file);

    // check if the magic number is wrong
    if (phdr->magic != MAGIC) {
        return false;
    }

    return true; // everything worked as intended
}

bool load_segment (FILE *file, byte_t *memory, elf_phdr_t phdr)
{
    // check if memory is null
    if (memory == NULL) {
        return false;
    }

    // check if file is null
    if (file == NULL) {
        return false;
    }

    // seek to correct location in file, returning false if it is invalid
    if (fseek(file, phdr.p_offset, SEEK_SET) != 0) {
        return false;
    }

    // check if address is wrong size
    if (phdr.p_vaddr < 0 || phdr.p_vaddr > MEMSIZE) {
        return false;
    }

    // reads the program header into virtual memory and checks if it worked
    fread(&memory[phdr.p_vaddr], 1, phdr.p_filesz, file);

    return true; // everything worked as intended
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p2 (char **argv)
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
}

bool parse_command_line_p2 (int argc, char **argv,
        bool *print_header, bool *print_segments,
        bool *print_membrief, bool *print_memfull,
        char **filename)
{
    // check for invalid / null-pointer parameters
    if (argc < 2 || print_header == NULL || print_segments == NULL ||
    print_membrief == NULL || print_memfull == NULL) {
        usage_p2(argv);        // print usage as default/abort case
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

    // ensure that parameter boolean flags are initialized to false
    *print_header   = false;
    *print_segments = false;
    *print_membrief = false;
    *print_memfull  = false;

    // parse command-line arguments. Invalid option prints usage and aborts program
    int opt;
    while ((opt = getopt(argc, argv, "hHafsmM")) != -1) {
        switch (opt) {
            case 'h':
                h_selected = true;     break;
            case 'H':
                H_selected = true;     break;
            case 'a':
                a_selected = true;     break;
            case 'f':
                f_selected = true;     break;
            case 's':
                s_selected = true;     break;
            case 'm':
                m_selected = true;     break;
            case 'M':
                M_selected = true;     break;
            default:
                usage_p2(argv);        return false;
        }
    }

    /* set boolean flags for options based off of their shortcut options
     * -a = -H -s -m
     * -f = -H -s -M
     */
    if (a_selected) {
        H_selected = true;
        s_selected = true;
        m_selected = true;
    }
    if (f_selected) {
        H_selected = true;
        s_selected = true;
        M_selected = true;
    }

    // act according to which options were activated
    if (h_selected) {
        usage_p2(argv);
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

    // checks if both print_membrief and print_memfull are true, returning false if so
    if (*print_membrief && *print_memfull) {
        usage_p2(argv);
        *print_membrief = false;
        *print_memfull = false;
        return false;
    }

    // loads the filename and checks for validity
    *filename = argv[optind];
    if (*filename == NULL || argc > optind + 1) {
        usage_p2(argv);
        *filename = NULL;
        return false;
    }

    // return true if a valid option was selected
    if (H_selected || s_selected || m_selected || M_selected) {
        return true;
    }

    return false;
}

void dump_phdrs (uint16_t numphdrs, elf_phdr_t phdr[])
{
    printf(" Segment   Offset    VirtAddr  FileSize  Type      Flag\n");

    for (int i = 0; i < numphdrs; i++) {
        elf_phdr_t cur = phdr[i];
        char *type = get_seg_type(cur.p_type); // get string rep of seg type
        char *flags = get_seg_flag(cur.p_flag); // get string rep of seg flag

        printf("%s%02d", "  ", i);
        printf("       0x%04x", cur.p_offset);
        printf("    0x%04x", cur.p_vaddr);
        printf("    0x%04x", cur.p_filesz);
        printf("    %s", type);
        printf("%s", flags);
        printf("\n");
    }
}

void dump_memory (byte_t *memory, uint16_t start, uint16_t end)
{
    int byte_count = 0;
    int row_count = 0;
    int offset = 0;

    printf("%s%04x%s%04x%s", "Contents of memory from ", start, " to ", end, ":\n");

    // check for invalid ends
    if (start >= end) {
        return;
    }

    // handle unaligment cases
    offset = start % 16;
    if (offset != 0) {
        start = start - offset;
        printf("  %04x  ", start);
        for (int i = 0; i < offset; i++) {
            printf("   ");
            byte_count++;
        }
        // extra space
        if (offset >= 8) {
            printf(" ");
        }
    }

    // print each byte
    for (int i = start + offset; i < end; i++) {
        if (byte_count == 0) {
            printf("  %04x  ", start + row_count * 16);
        }

        printf("%02x", memory[i]);

        byte_count++;

        if (byte_count != 16 && i != end - 1) {
            printf(" ");
        }

        if (byte_count == 8) {
            printf(" "); // print an extra space every 8 bytes for readability
        }

        if (byte_count == 16) {
            byte_count = 0;
            row_count++;
            if (i != end - 1) {
                printf("\n");
            }
        }
    }

    printf("\n");
}

char* get_seg_type(int type_int)
{
    char *type_string;

    // basically convert into to string
    switch (type_int) {
        case DATA:
            type_string = "DATA      ";    break;
        case CODE:
            type_string = "CODE      ";    break;
        case STACK:
            type_string = "STACK     ";    break;
        case HEAP:
            type_string = "HEAP      ";    break;
        default:
            type_string = "UNKNOWN   ";    break;
    }

    return type_string;
}

char* get_seg_flag(int flag_int)
{
    char *flag_string;

    /* determine flags based off of high-order number combinations
     * R = 4
     * W = 2
     * X = 1
     */
    if (flag_int == _X) {
        flag_string = "  X";
    } else if (flag_int == _W) {
        flag_string = " W ";
    } else if (flag_int == _W + _X) {
        flag_string = " WX";
    } else if (flag_int == _R) {
        flag_string = "R  ";
    } else if (flag_int == _R + _X) {
        flag_string = "R X";
    } else if (flag_int == _R + _W) {
        flag_string = "RW ";
    } else if (flag_int == _R + _W + _X) {
        flag_string = "RWX";
    } else {
        flag_string = "   ";
    }

    return flag_string;
}
