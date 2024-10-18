#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

void print_symbol(const char *name, Elf64_Sym *sym, char *strtab) {
    char type;
    switch (ELF64_ST_TYPE(sym->st_info)) {
        case STT_FUNC: type = 'T'; break;
        case STT_OBJECT: type = 'D'; break;
        case STT_NOTYPE: type = ' '; break;
        default: type = '?'; break;
    }

    printf("%016lx %c %s\n", (unsigned long)sym->st_value, type, &strtab[sym->st_name]);
}

void read_elf(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    Elf64_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        perror("read");
        close(fd);
        return;
    }

    lseek(fd, ehdr.e_shoff, SEEK_SET);
    Elf64_Shdr *shdrs = malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));
    read(fd, shdrs, ehdr.e_shnum * sizeof(Elf64_Shdr));

    char *strtab = NULL;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_STRTAB && i == ehdr.e_shstrndx) {
            strtab = malloc(shdrs[i].sh_size);
            lseek(fd, shdrs[i].sh_offset, SEEK_SET);
            read(fd, strtab, shdrs[i].sh_size);
            break;
        }
    }

    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            Elf64_Sym *syms = malloc(shdrs[i].sh_size);
            lseek(fd, shdrs[i].sh_offset, SEEK_SET);
            read(fd, syms, shdrs[i].sh_size);

            for (size_t j = 0; j < shdrs[i].sh_size / sizeof(Elf64_Sym); j++) {
                print_symbol(strtab, &syms[j], strtab);
            }
            free(syms);
        }
    }

    free(shdrs);
    free(strtab);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [objfile ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        read_elf(argv[i]);
    }

    return EXIT_SUCCESS;
}
