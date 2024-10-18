#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

void print_symbol(Elf64_Sym *sym, char *strtab) {
    char type;
    
    switch (ELF64_ST_TYPE(sym->st_info)) {
        case STT_FUNC:     type = 'T'; break;
        case STT_OBJECT:   type = 'D'; break;
        case STT_NOTYPE:   type = ' '; break;
        default:           type = '?'; break;
    }


    printf("%016lx %c %s\n", (unsigned long)sym->st_value, type, &strtab[sym->st_name]);
}

void read_elf(const char *filename) {
    int fd;
    Elf64_Ehdr ehdr;
    Elf64_Shdr *shdrs;
    char *strtab = NULL;
    int i;
    size_t j;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        perror("read");
        close(fd);
        return;
    }

    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) !0) {
        fprintf(stderr, "Not a valid ELF file\n")
        close(fd);
        return;
    }

    shdrs = malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));
    if (shdrs == NULL) {
        perror("malloc");
        close(fd);
        return;
    }

    if (lseek(fd, ehdr.e_shoff, SEEK_SET) == -1) {
        perror("lseek");
        free(shdrs);
        close(fd);
        return;
    }

    if (read(fd, shdrs, ehdr.e_shnum * sizeof(Elf64_Shdr)) != (ssize_t)(ehdr.e_shnum * sizeof(Elf64_Shdr))) {
        perror("read");
        free(shdrs);
        close(fd);
        return;
    }

    for (i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_STRTAB && i == ehdr.e_shstrndx) {
            strtab = malloc(shdrs[i].sh_size);
            if (strtab == NULL) {
                perror("malloc");
                free(shdrs);
                close(fd);
                return;
            }
            lseek(fd, shdrs[i].sh_offset, SEEK_SET);
            if (read(fd, strtab, shdrs[i].sh_size) != shdrs[i].sh_size) {
                perror("read");
                free(strtab);
                free(shdrs);
                close(fd);
                return;
            }
            break;
        }
    }

    for (i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            Elf64_Sym *syms = malloc(shdrs[i].sh_size);
            if (syms == NULL) {
                perror("malloc");
                free(shdrs);
                free(strtab);
                close(fd);
                return;
            }

            lseek(fd, shdrs[i].sh_offset, SEEK_SET);
            if (read(fd, syms, shdrs[i].sh_size) != shdrs[i].sh_size) {
                free(syms);
                free(shdrs);
                free(strtab);
                close(fd);
                return;
            };

            for (j = 0; j < shdrs[i].sh_size / sizeof(Elf64_Sym); j++) {
                print_symbol(&syms[j], strtab);
            }

            free(syms);
        }
    }

    free(shdrs);
    free(strtab);
    close(fd);
}

int main(int argc, char *argv[]) {
    int i;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [objfile ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc; i++) {
        read_elf(argv[i]);
    }

    return EXIT_SUCCESS;
}
