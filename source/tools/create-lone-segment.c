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

static size_t move_program_header_table(struct lone_bytes elf, size_t elf_size, struct lone_elf_program_header_table *phdrs)
{
	size_t table_size = phdrs->entry_size * phdrs->entry_count, new_size = elf_size + table_size;
	void *new_address;

	if (elf.count < new_size) { /* Not enough space in buffer */ linux_exit(7); }

	new_address = elf.pointer + elf_size;
	lone_memory_move(phdrs->address, new_address, table_size);
	phdrs->address = new_address;

	return new_size;
}

static size_t append_program_header_table_entry(struct lone_bytes elf, size_t elf_size, unsigned char elf_class, struct lone_elf_program_header_table *phdrs)
{
	size_t new_size = elf_size + phdrs->entry_size;
	unsigned char *entry = phdrs->address;
	Elf32_Phdr *phdr32;
	Elf64_Phdr *phdr64;

	if (elf.count < new_size) { /* Not enough space in buffer */ linux_exit(8); }

	entry += phdrs->entry_size * phdrs->entry_count++;

	switch (elf_class) {
	case ELFCLASS64:
		phdr64 = (Elf64_Phdr *) entry;
		phdr64->p_type = PT_NULL;
		phdr64->p_flags = PF_R;
		phdr64->p_offset = 0;
		phdr64->p_vaddr = 0;
		phdr64->p_paddr = 0;
		phdr64->p_filesz = 0;
		phdr64->p_memsz = 0;
		phdr64->p_align = 1;
		break;
	case ELFCLASS32:
		phdr32 = (Elf32_Phdr *) entry;
		phdr32->p_type = PT_NULL;
		phdr32->p_flags = PF_R;
		phdr32->p_offset = 0;
		phdr32->p_vaddr = 0;
		phdr32->p_paddr = 0;
		phdr32->p_filesz = 0;
		phdr32->p_memsz = 0;
		phdr32->p_align = 1;
		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}

	return new_size;
}

static size_t adjust_elf_header(struct lone_bytes elf, unsigned char elf_class, struct lone_elf_program_header_table phdrs)
{
	unsigned char *address = phdrs.address;
	Elf32_Ehdr *elf32;
	Elf64_Ehdr *elf64;

	switch (elf_class) {
	case ELFCLASS64:
		elf64 = (Elf64_Ehdr *) elf.pointer;
		elf64->e_phoff = address - elf.pointer;
		elf64->e_phentsize = phdrs.entry_size;
		elf64->e_phnum = phdrs.entry_count;
		return elf64->e_phoff;
	case ELFCLASS32:
		elf32 = (Elf32_Ehdr *) elf.pointer;
		elf32->e_phoff = address - elf.pointer;
		elf32->e_phentsize = phdrs.entry_size;
		elf32->e_phnum = phdrs.entry_count;
		return elf32->e_phoff;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}
}

static void adjust_and_cover_phdr_entry(struct lone_bytes elf, unsigned char elf_class, struct lone_elf_program_header_table phdrs)
{
	unsigned char *table = phdrs.address;
	size_t table_offset = table - elf.pointer;
	size_t size = phdrs.entry_size * phdrs.entry_count;
	Elf32_Phdr *phdr32;
	Elf64_Phdr *phdr64;
	size_t base, address, alignment, flags, i;

	switch (elf_class) {
	case ELFCLASS64:
		for (i = 0; i < phdrs.entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;
			if (phdr64->p_type == PT_PHDR) {
				base = phdr64->p_vaddr - phdr64->p_offset;
				address = base + table_offset;
				alignment = phdr64->p_align;
				flags = phdr64->p_flags;

				phdr64->p_offset = table_offset;
				phdr64->p_vaddr = phdr64->p_paddr = address;
				phdr64->p_filesz = phdr64->p_memsz = size;
				break;
			}
		}
		for (/* i */; i < phdrs.entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;
			if (phdr64->p_type == PT_NULL) {
				phdr64->p_type = PT_LOAD;

				phdr64->p_offset = table_offset;
				phdr64->p_vaddr = phdr64->p_paddr = address;
				phdr64->p_filesz = phdr64->p_memsz = size;
				phdr64->p_align = alignment;
				phdr64->p_flags = flags;
				break;
			}
		}
		break;
	case ELFCLASS32:
		for (i = 0; i < phdrs.entry_count; ++i) {
			phdr32 = ((Elf32_Phdr *) table) + i;
			if (phdr32->p_type == PT_PHDR) {
				base = phdr32->p_vaddr - phdr32->p_offset;
				address = base + table_offset;
				alignment = phdr32->p_align;
				flags = phdr32->p_flags;

				phdr32->p_offset = table_offset;
				phdr32->p_vaddr = phdr32->p_paddr = address;
				phdr32->p_filesz = phdr32->p_memsz = size;
				break;
			}
		}
		for (/* i */; i < phdrs.entry_count; ++i) {
			phdr64 = ((Elf32_Phdr *) table) + i;
			if (phdr32->p_type == PT_NULL) {
				phdr32->p_type = PT_LOAD;

				phdr32->p_offset = table_offset;
				phdr32->p_vaddr = phdr32->p_paddr = address;
				phdr32->p_filesz = phdr32->p_memsz = size;
				phdr32->p_align = alignment;
				phdr32->p_flags = flags;
				break;
			}
		}
		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}
}

static void set_lone_entry(struct lone_bytes elf, unsigned char elf_class, struct lone_elf_program_header_table phdrs)
{
	unsigned char *table = phdrs.address;
	Elf32_Phdr *phdr32;
	Elf64_Phdr *phdr64;
	size_t i;

	switch (elf_class) {
	case ELFCLASS64:
		for (i = 0; i < phdrs.entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;
			if (phdr64->p_type == PT_NULL) {
				phdr64->p_type = PT_LONE;
				break;
			}
		}
		break;
	case ELFCLASS32:
		for (i = 0; i < phdrs.entry_count; ++i) {
			phdr32 = ((Elf32_Phdr *) table) + i;
			if (phdr32->p_type == PT_NULL) {
				phdr32->p_type = PT_LONE;
				break;
			}
		}
		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(6);
	}
}

static size_t write_elf(char *path, struct lone_bytes buffer, size_t elf_size)
{
	size_t bytes_written;
	int fd;

	fd = linux_openat(AT_FDCWD, path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC);
	if (fd < 0) { /* error opening output file */ linux_exit(9); }

	bytes_written = linux_write(fd, buffer.pointer, elf_size);
	if (bytes_written < 0) { /* error writing output file */ linux_exit(10); }

	fd = linux_close(fd);
	if (fd < 0) { /* error closing output file */ linux_exit(11); }

	return bytes_written;
}

long lone(int argc, char **argv, char **envp, struct auxiliary_vector *auxvec)
{
	static unsigned char buffer[1024 * 1024];
	struct lone_bytes elf = { sizeof(buffer), buffer };
	struct lone_elf_program_header_table phdrs;
	unsigned char elf_class;
	size_t elf_size, new_offset;

	check_arguments(argc, argv);
	elf_size = read_elf(argv[1], elf);
	elf_class = validate_elf_header(elf);
	phdrs = find_program_header_table(elf, elf_class);
	elf_size = move_program_header_table(elf, elf_size, &phdrs);
	elf_size = append_program_header_table_entry(elf, elf_size, elf_class, &phdrs);
	elf_size = append_program_header_table_entry(elf, elf_size, elf_class, &phdrs);
	new_offset = adjust_elf_header(elf, elf_class, phdrs);
	adjust_and_cover_phdr_entry(elf, elf_class, phdrs);
	set_lone_entry(elf, elf_class, phdrs);
	write_elf(argv[2], elf, elf_size);

	return 0;
}

#include <lone/architecture/linux/entry_point.c>
