#include "injector.h"

extern const char _binary_shellcode_bin_start[];
extern const char _binary_shellcode_bin_end[];

/**
 * find_valid_section - Find the section to be patched
 *
 * This finds the last injectable section in the executable segments.
 * This method is derived from the repository `drow`[1].
 *
 * [1]: https://github.com/zznop/drow
 */
int find_valid_section(char *image,
                       unsigned long patch_size,
                       unsigned long *ppatch_base,
                       unsigned long *ppatch_virtual_base)
{
    Elf64_Ehdr *elf_header = (Elf64_Ehdr *) image;
    Elf64_Shdr *section_headers = (Elf64_Shdr *) (image + elf_header->e_shoff);
    Elf64_Phdr *program_headers = (Elf64_Phdr *) (image + elf_header->e_phoff);
    char *str_table = image + section_headers[elf_header->e_shstrndx].sh_offset;
    int mx = 0;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (!(program_headers[i].p_flags & PF_X))
            continue;
        for (int j = 0; j < elf_header->e_shnum; j++)
            if (section_headers[j].sh_offset + section_headers[j].sh_size <=
                    program_headers[i].p_offset + program_headers[i].p_filesz &&
                j + 1 != elf_header->e_shnum &&
                !(section_headers[j + 1].sh_offset -
                          section_headers[j].sh_offset -
                          section_headers[j].sh_size <
                      patch_size &&
                  section_headers[j + 1].sh_addr - section_headers[j].sh_addr -
                          section_headers[j].sh_size <
                      patch_size))
                mx = j;
        if (!mx)
            continue;
        printf(INFO "Section found: 0x%lx %s\n", section_headers[mx].sh_offset,
               str_table + section_headers[mx].sh_name);
        *ppatch_base =
            section_headers[mx].sh_offset + section_headers[mx].sh_size;
        *ppatch_virtual_base =
            section_headers[mx].sh_addr + section_headers[mx].sh_size;
        program_headers[i].p_filesz += patch_size;
        program_headers[i].p_memsz += patch_size;
        section_headers[mx].sh_size += patch_size;
        return 0;
    }

    return -1;
}
