#include "hnm.h"

/**
 * print_symbol_table32 - prints the symbol table for a 32-bit ELF file
 * @section_header: a pointer to the section header of the symbol table
 * @symbol_table: a pointer to the beginning of the symbol table
 * @string_table: a pointer to the beginning of the string table,
 *                which contains the names of the symbols
 * @section_headers: a pointer to the array of section headers for the ELF file
 * Return: nothing (void)
 */
void print_symbol_table32(Elf32_Shdr *section_header, Elf32_Sym *symbol_table,
                           char *string_table, Elf32_Shdr *section_headers)
{
    int i;
    int symbol_count = section_header->sh_size / sizeof(Elf32_Sym);
    char *symbol_name, symbol_type;

    for (i = 0; i < symbol_count; i++) {
        Elf32_Sym symbol = symbol_table[i];
        symbol_name = string_table + symbol.st_name;

        if (symbol.st_name != 0 && ELF32_ST_TYPE(symbol.st_info) != STT_FILE) {
            symbol_type = get_symbol_type(symbol, section_headers);
            print_symbol_info(symbol, symbol_name, symbol_type);
        }
    }
}

/**
 * get_symbol_type - determines the type of a symbol based on its attributes
 * @symbol: the symbol to analyze
 * @section_headers: the array of section headers
 * Return: a character representing the symbol type
 */
char get_symbol_type(Elf32_Sym symbol, Elf32_Shdr *section_headers) {
    char symbol_type = '?';

    // Handle weak symbols
    if (ELF32_ST_BIND(symbol.st_info) == STB_WEAK) {
        if (symbol.st_shndx == SHN_UNDEF) {
            symbol_type = 'w'; // weak undefined
        } else if (ELF32_ST_TYPE(symbol.st_info) == STT_OBJECT) {
            symbol_type = 'V'; // weak object
        } else {
            symbol_type = 'W'; // weak defined
        }
    } else if (symbol.st_shndx == SHN_UNDEF) {
        symbol_type = 'U'; // undefined
    } else if (symbol.st_shndx == SHN_ABS) {
        symbol_type = 'A'; // absolute
    } else if (symbol.st_shndx == SHN_COMMON) {
        symbol_type = 'C'; // common
    } else if (symbol.st_shndx < SHN_LORESERVE) {
        Elf32_Shdr symbol_section = section_headers[symbol.st_shndx];

        if (ELF32_ST_BIND(symbol.st_info) == STB_GNU_UNIQUE) {
            symbol_type = 'u'; // unique
        } else if (symbol_section.sh_type == SHT_NOBITS &&
                   symbol_section.sh_flags == (SHF_ALLOC | SHF_WRITE)) {
            symbol_type = 'B'; // bss
        } else if (symbol_section.sh_type == SHT_PROGBITS) {
            if (symbol_section.sh_flags == (SHF_ALLOC | SHF_EXECINSTR)) {
                symbol_type = 'T'; // text
            } else if (symbol_section.sh_flags == SHF_ALLOC) {
                symbol_type = 'R'; // read-only
            } else if (symbol_section.sh_flags == (SHF_ALLOC | SHF_WRITE)) {
                symbol_type = 'D'; // data
            }
        } else if (symbol_section.sh_type == SHT_DYNAMIC) {
            symbol_type = 'D'; // dynamic
        } else {
            symbol_type = 't'; // local text
        }
    }

    return symbol_type;
}

/**
 * print_symbol_info - prints the symbol information
 * @symbol: the symbol to print
 * @symbol_name: the name of the symbol
 * @symbol_type: the type of the symbol
 * Return: nothing (void)
 */
void print_symbol_info(Elf32_Sym symbol, char *symbol_name, char symbol_type) {
    // Convert to lowercase for local symbols
    if (ELF32_ST_BIND(symbol.st_info) == STB_LOCAL) {
        symbol_type = tolower(symbol_type);
    }

    // Print symbol address or just type/name if undefined
    if (symbol_type != 'U' && symbol_type != 'w') {
        printf("%08x %c %s\n", symbol.st_value, symbol_type, symbol_name);
    } else {
        printf("         %c %s\n", symbol_type, symbol_name);
    }
}

/**
 * process_elf_file32 - processes a 32-bit ELF file located at the given file path
 * @file_path: a pointer to a string that contains the path to the ELF file to be processed
 * Return: nothing (void)
 */
void process_elf_file32(char *file_path) {
    int symbol_table_index = -1;
    int i;
    int is_little_endian, is_big_endian;
    int string_table_index;

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "./hnm: %s: failed to open file\n", file_path);
        return;
    }

    Elf32_Ehdr elf_header;
    fread(&elf_header, sizeof(Elf32_Ehdr), 1, file);

    // Check ELF file type
    if (elf_header.e_ident[EI_CLASS] != ELFCLASS32 &&
        elf_header.e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "./hnm: %s: unsupported ELF file format\n", file_path);
        fclose(file);
        return;
    }

    // Check endianness
    is_little_endian = (elf_header.e_ident[EI_DATA] == ELFDATA2LSB);
    is_big_endian = (elf_header.e_ident[EI_DATA] == ELFDATA2MSB);
    if (!is_little_endian && !is_big_endian) {
        fprintf(stderr, "./hnm: %s: unsupported ELF file endianness\n", file_path);
        fclose(file);
        return;
    }

    // Read section headers
    Elf32_Shdr *section_headers = malloc(elf_header.e_shentsize * elf_header.e_shnum);
    if (section_headers == NULL) {
        fprintf(stderr, "./hnm: %s: memory allocation error for section_headers\n", file_path);
        fclose(file);
        return;
    }

    fseek(file, elf_header.e_shoff, SEEK_SET);
    fread(section_headers, elf_header.e_shentsize, elf_header.e_shnum, file);

    // Find symbol table index
    for (i = 0; i < elf_header.e_shnum; i++) {
        if (section_headers[i].sh_type == SHT_SYMTAB) {
            symbol_table_index = i;
            break;
        }
    }

    if (symbol_table_index == -1) {
        fprintf(stderr, "./hnm: %s: no symbols\n", file_path);
        fclose(file);
        free(section_headers);
        return;
    }

    // Read symbol table
    Elf32_Shdr symbol_table_header = section_headers[symbol_table_index];
    Elf32_Sym *symbol_table = malloc(symbol_table_header.sh_size);
    if (symbol_table == NULL) {
        fprintf(stderr, "./hnm: %s: memory allocation error for symbol_table\n", file_path);
        fclose(file);
        free(section_headers);
        return;
    }

    fseek(file, symbol_table_header.sh_offset, SEEK_SET);
    fread(symbol_table, symbol_table_header.sh_size, 1, file);

    // Read string table
    string_table_index = symbol_table_header.sh_link;
    Elf32_Shdr string_table_header = section_headers[string_table_index];
    char *string_table = malloc(string_table_header.sh_size);
    if (string_table == NULL) {
        fprintf(stderr, "./hnm: %s: memory allocation error for string_table\n", file_path);
        fclose(file);
        free(symbol_table);
        free(section_headers);
        return;
    }

    fseek(file, string_table_header.sh_offset, SEEK_SET);
    fread(string_table, string_table_header.sh_size, 1, file);

    // Print the symbol table
    print_symbol_table32(&symbol_table_header, symbol_table, string_table, section_headers);

    fclose(file);
    free(section_headers);
    free(symbol_table);
    free(string_table);
}