/*
 * CS 261 PA1: Mini-ELF header verifier
 *
 * Name: Dylan Moreno
 */

#include "p1-check.h"

#define SIZE 16
#define EXPECTED_MAGIC 4607045

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

bool read_header (FILE *file, elf_hdr_t *hdr)
{
    if (hdr == NULL) {
        return false;
    }

    // check if the FILE is big enough and read in its contents
    if (fread(hdr, 1, sizeof(elf_hdr_t), file) < SIZE) {
        return false;
    }

    // check if the ELF file has the correct magic number
    if (hdr->magic != EXPECTED_MAGIC) {
        return false;
    }


    return true;
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p1 (char **argv)
{
    printf("Usage: %s <option(s)> mini-elf-file\n", argv[0]);
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
}

bool parse_command_line_p1 (int argc, char **argv, bool *print_header, char **filename)
{
    bool h_selected = false;
    bool H_selected = false;

    // get the filename (arbitrary value provided if a filename was not given)
    if (argc == 2) {
        *filename = argv[1];
    } else if (argc > 2) {
        *filename = argv[2];
    } else {
        *filename = "";
    }

    // parse command line
    int opt;
    while ((opt = getopt(argc, argv, "hH")) != -1) {
        switch (opt) {
            case 'h':
                h_selected = true;
                break;
            case 'H':
                H_selected = true;
                break;
            default:
                h_selected = true;
                break;
        }
    }

    // act upon option selections
    if (h_selected) {
        usage_p1(argv);
        *print_header = false;
        H_selected = false;
        return true;
    } else if (H_selected) {
        if (argc == 3) {	// allow the header to be printed
            *print_header = true;
        } else {		// if no filename, print the usage text instead
            usage_p1(argv);
            *print_header = false;
        }
        return true;
    }

    return false;
}

void dump_header (elf_hdr_t hdr)
{
    // print little-endian hex dump of the Mini-ELF header data (i.e. the first 16 bytes)
    printf("%02x ", (hdr.e_version & 0xff));
    printf("%02x ", (hdr.e_version >> 8) & 0xff);

    printf("%02x ", (hdr.e_entry & 0xff));
    printf("%02x ", (hdr.e_entry >> 8) & 0xff);

    printf("%02x ", (hdr.e_phdr_start & 0xff));
    printf("%02x ", (hdr.e_phdr_start >> 8) & 0xff);

    printf("%02x ", (hdr.e_num_phdr & 0xff));
    printf("%02x  ", (hdr.e_num_phdr >> 8) & 0xff); // extra space for readability

    printf("%02x ", (hdr.e_symtab & 0xff));
    printf("%02x ", (hdr.e_symtab >> 8) & 0xff);

    printf("%02x ", (hdr.e_strtab & 0xff));
    printf("%02x ", (hdr.e_strtab >> 8) & 0xff);

    printf("%02x ", (hdr.magic & 0xff));
    printf("%02x ", (hdr.magic >> 8) & 0xff);
    printf("%02x ", (hdr.magic >> 16) & 0xff);
    printf("%02x", (hdr.magic >> 24) & 0xff);

    printf("\n");

    // print the version
    printf("Mini-ELF version ");
    printf("%d", hdr.e_version);

    printf("\n");

    // print the entry point address
    printf("Entry point ");
    printf("%#03x", hdr.e_entry);

    printf("\n");

    // print information about the program headers
    printf("%s%d%s", "There are ", hdr.e_num_phdr, " program headers, ");
    printf("%s%d", "starting at offset ", hdr.e_phdr_start);
    printf("%s%#2x%s", " (", hdr.e_phdr_start, ")");

    printf("\n");

    // print information about the symbol table
    if (hdr.e_symtab == 0) {
        printf("There is no symbol table present");
    } else {
        printf("There is a symbol table starting at offset ");
        printf("%d%s%#x%s", hdr.e_symtab, " (", hdr.e_symtab, ")");
    }

    printf("\n");

    // print information about the string table
    if (hdr.e_strtab == 0) {
        printf("There is no string table present");
    } else {
        printf("There is a string table starting at offset ");
        printf("%d%s%#2x%s", hdr.e_strtab, " (", hdr.e_strtab, ")");
    }

    printf("\n");
}

