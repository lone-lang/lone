#include <lone.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/elf.h>
#include <lone/struct/elf.h>
#include <lone/struct/bytes.h>
#include <lone/memory/functions.h>

static void check_arguments(int argc, char **argv)
{
	if (argc <= 2) { /* at least two arguments are needed */ linux_exit(1); }
}

static size_t read_elf(char *path, struct lone_bytes buffer)
{
	size_t bytes_read;
	int fd;

	fd = linux_openat(AT_FDCWD, path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) { /* error opening ELF */ linux_exit(2); }

	bytes_read = linux_read(fd, buffer.pointer, buffer.count);
	if (bytes_read < 0) { /* error reading ELF */ linux_exit(3); }

	fd = linux_close(fd);
	if (fd < 0) { /* error closing ELF */ linux_exit(4); }

	return bytes_read;
}

static bool has_valid_elf_magic_numbers(struct lone_bytes elf)
{
	return elf.count            >= SELFMAG &&
	       elf.pointer[EI_MAG0] == ELFMAG0 &&
	       elf.pointer[EI_MAG1] == ELFMAG1 &&
	       elf.pointer[EI_MAG2] == ELFMAG2 &&
	       elf.pointer[EI_MAG3] == ELFMAG3;
}

static bool has_valid_elf_class(struct lone_bytes elf)
{
	return elf.count             >= SELFMAG + 1  &&
	       elf.pointer[EI_CLASS] >  ELFCLASSNONE &&
	       elf.pointer[EI_CLASS] <  ELFCLASSNUM;
}

static bool has_valid_elf_header_size(size_t elf_size, unsigned char class)
{
	switch (class) {
	case ELFCLASS64:
		return elf_size >= sizeof(Elf64_Ehdr);
	case ELFCLASS32:
		return elf_size >= sizeof(Elf32_Ehdr);
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}
}

static unsigned char validate_elf_header(struct lone_bytes elf)
{
	unsigned char elf_class = ELFCLASSNONE;

	if (!(has_valid_elf_magic_numbers(elf) && has_valid_elf_class(elf))) {
		/* Definitely not an ELF */ linux_exit(5);
	}

	elf_class = elf.pointer[EI_CLASS];
	if (!has_valid_elf_header_size(elf.count, elf_class)) {
		/* Incomplete or corrupt ELF */ linux_exit(6);
	}

	return elf_class;
}

static struct lone_elf_program_header_table find_program_header_table(struct lone_bytes elf, unsigned char elf_class)
{
	Elf32_Ehdr *elf32;
	Elf64_Ehdr *elf64;

	switch (elf_class) {
	case ELFCLASS64:
		elf64 = (Elf64_Ehdr *) elf.pointer;
		return (struct lone_elf_program_header_table) {
			.address = elf.pointer + elf64->e_phoff,
			.entry_size = elf64->e_phentsize,
			.entry_count = elf64->e_phnum
		};
	case ELFCLASS32:
		elf32 = (Elf32_Ehdr *) elf.pointer;
		return (struct lone_elf_program_header_table) {
			.address = elf.pointer + elf32->e_phoff,
			.entry_size = elf32->e_phentsize,
			.entry_count = elf32->e_phnum
		};
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}
}

static void set_lone_entry(struct lone_bytes elf, unsigned char elf_class, struct lone_elf_program_header_table phdrs)
{
	unsigned char *table = phdrs.address;
	size_t base, address, size, alignment, flags, i;
	Elf32_Phdr *pt_phdr32 = NULL, *phdr32;
	Elf64_Phdr *pt_phdr64 = NULL, *phdr64;

	switch (elf_class) {
	case ELFCLASS64:
		for (i = 0; i < phdrs.entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;

			switch (phdr64->p_type) {
			case PT_NULL:
				// linker allocated spare program header
set_lone_entry_64:
				phdr64->p_type = PT_LONE;

				phdr64->p_filesz = phdr64->p_memsz = 0;
				phdr64->p_vaddr = phdr64->p_paddr = phdr64->p_vaddr - phdr64->p_offset;
				phdr64->p_offset = 0;
				phdr64->p_align = 1;
				phdr64->p_flags = PF_R;

				goto lone_entry_set;

			case PT_PHDR:
				/* The PT_PHDR entry is optional
				   and will become a PT_LONE entry
				   if no linker spares are provided */

				pt_phdr64 = phdr64;

				break;

			default:
				continue;
			}
		}

		// no spare segments were provided by the linker...
		if (pt_phdr64) {
			// ... but there's a PT_PHDR segment
			phdr64 = pt_phdr64;
			goto set_lone_entry_64;
		} else {
			// ... and there's no PT_PHDR segment
			linux_exit(7);
		}

		break;
	case ELFCLASS32:
		for (i = 0; i < phdrs.entry_count; ++i) {
			phdr32 = ((Elf32_Phdr *) table) + i;

			switch (phdr32->p_type) {
			case PT_NULL:
				// linker allocated spare program header
set_lone_entry_32:
				phdr32->p_type = PT_LONE;

				phdr32->p_filesz = phdr32->p_memsz = 0;
				phdr32->p_vaddr = phdr32->p_paddr = phdr32->p_vaddr - phdr32->p_offset;
				phdr32->p_offset = 0;
				phdr32->p_align = 1;
				phdr32->p_flags = PF_R;

				goto lone_entry_set;

			case PT_PHDR:
				/* The PT_PHDR entry is optional
				   and will become a PT_LONE entry
				   if no linker spares are provided */

				pt_phdr32 = phdr32;

				break;

			default:
				continue;
			}
		}

		// no spare segments were provided by the linker...
		if (pt_phdr32) {
			// ... but there's a PT_PHDR segment
			phdr32 = pt_phdr32;
			goto set_lone_entry_32;
		} else {
			// ... and there's no PT_PHDR segment
			linux_exit(7);
		}
		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}

lone_entry_set:
	return;
}

static size_t write_elf(char *path, struct lone_bytes buffer, size_t elf_size)
{
	size_t bytes_written;
	int fd;

	fd = linux_openat(AT_FDCWD, path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC);
	if (fd < 0) { /* error opening output file */ linux_exit(8); }

	bytes_written = linux_write(fd, buffer.pointer, elf_size);
	if (bytes_written < 0) { /* error writing output file */ linux_exit(9); }

	fd = linux_close(fd);
	if (fd < 0) { /* error closing output file */ linux_exit(10); }

	return bytes_written;
}

long lone(int argc, char **argv, char **envp, struct auxiliary_vector *auxvec)
{
	static unsigned char buffer[1024 * 1024];
	struct lone_bytes elf = { sizeof(buffer), buffer };
	struct lone_elf_program_header_table phdrs;
	unsigned char elf_class;
	size_t elf_size;

	check_arguments(argc, argv);
	elf_size = read_elf(argv[1], elf);
	elf_class = validate_elf_header(elf);
	phdrs = find_program_header_table(elf, elf_class);
	set_lone_entry(elf, elf_class, phdrs);
	write_elf(argv[2], elf, elf_size);

	return 0;
}

#include <lone/architecture/linux/entry_point.c>
