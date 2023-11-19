#include <lone.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/elf.h>
#include <lone/struct/bytes.h>
#include <lone/memory/functions.h>

#ifndef IO_BUFFER_SIZE
#define IO_BUFFER_SIZE 1024 * 4096
#endif

struct elf {
	struct lone_bytes header;
	unsigned char class;

	struct {
		size_t offset;
		size_t entry_size;
		size_t entry_count;
		struct lone_bytes memory;
	} program_header_table;

	int file_descriptor;
	struct {
		int file_descriptor;
		size_t offset;
		size_t size;
	} data;
};

static void check_arguments(int argc, char **argv)
{
	if (argc <= 2) { /* at least 2 arguments are needed */ linux_exit(1); }
}

static int open_path(char *path)
{
	int fd = linux_openat(AT_FDCWD, path, O_RDWR | O_CLOEXEC);
	if (fd < 0) { /* error opening file descriptor */ linux_exit(2); }
	return fd;
}

static size_t read_bytes(int fd, struct lone_bytes buffer)
{
	size_t total = 0;
	ssize_t read_or_error;

	while (total < buffer.count) {
		read_or_error = linux_read(fd, buffer.pointer + total, buffer.count - total);

		if (read_or_error > 0) {
			total += read_or_error;
		} else if (read_or_error == 0) {
			break;
		} else {
			switch (read_or_error) {
			case -EINTR:
			case -EAGAIN:
				continue;
			default:
				/* error reading input file */ linux_exit(3);
			}
		}
	}

	return total;
}

static size_t write_bytes(int fd, struct lone_bytes buffer)
{
	size_t total = 0;
	ssize_t written_or_error;

	while (total < buffer.count) {
		written_or_error = linux_write(fd, buffer.pointer + total, buffer.count - total);

		if (written_or_error > 0) {
			total += written_or_error;
		} else if (written_or_error == 0) {
			break;
		} else {
			switch (written_or_error) {
			case -EINTR:
			case -EAGAIN:
				continue;
			default:
				/* error writing output file */ linux_exit(4);
			}
		}
	}

	return total;
}

static void *map(size_t size)
{
	intptr_t memory = linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (memory < 0) { /* memory allocation error */ linux_exit(5); }
	return (void *) memory;
}

static off_t seek_from_to(int fd, int origin, off_t offset)
{
	off_t offset_or_error = linux_lseek(fd, offset, origin);
	if (offset_or_error < 0) { /* error seeking to offset */ linux_exit(6); }
	return offset_or_error;
}

static off_t seek_to(int fd, off_t offset)
{
	return seek_from_to(fd, SEEK_SET, offset);
}

static off_t seek_to_start(int fd)
{
	return seek_to(fd, 0);
}

static off_t seek_to_end(int fd)
{
	return seek_from_to(fd, SEEK_END, 0);
}

static off_t file_size(int fd)
{
	size_t size = seek_to_end(fd);
	seek_to_start(fd);
	return size;
}

static bool has_valid_elf_magic_numbers(struct lone_bytes buffer)
{
	return buffer.pointer[EI_MAG0] == ELFMAG0 &&
	       buffer.pointer[EI_MAG1] == ELFMAG1 &&
	       buffer.pointer[EI_MAG2] == ELFMAG2 &&
	       buffer.pointer[EI_MAG3] == ELFMAG3;
}

static bool has_valid_elf_class(struct lone_bytes buffer)
{
	return buffer.pointer[EI_CLASS] > ELFCLASSNONE &&
	       buffer.pointer[EI_CLASS] < ELFCLASSNUM;
}

static bool has_valid_elf_header_size(size_t size, unsigned char class)
{
	switch (class) {
	case ELFCLASS64:
		return size >= sizeof(Elf64_Ehdr);
	case ELFCLASS32:
		return size >= sizeof(Elf32_Ehdr);
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(8);
	}
}

static void validate_elf_header(struct elf *elf)
{
	read_bytes(elf->file_descriptor, elf->header);

	elf->class = ELFCLASSNONE;

	if (!(has_valid_elf_magic_numbers(elf->header) && has_valid_elf_class(elf->header))) {
		/* Definitely not an ELF */ linux_exit(7);
	}

	elf->class = elf->header.pointer[EI_CLASS];

	if (!has_valid_elf_header_size(elf->header.count, elf->class)) {
		/* Incomplete or corrupt ELF */ linux_exit(8);
	}
}

static void load_program_header_table(struct elf *elf)
{
	Elf32_Ehdr *elf32;
	Elf64_Ehdr *elf64;
	size_t offset, entry_size, entry_count, size;
	void *address;

	switch (elf->class) {
	case ELFCLASS64:
		elf64 = (Elf64_Ehdr *) elf->header.pointer;
		offset = elf64->e_phoff;
		entry_size = elf64->e_phentsize;
		entry_count = elf64->e_phnum;
		break;
	case ELFCLASS32:
		elf32 = (Elf32_Ehdr *) elf->header.pointer;
		offset = elf32->e_phoff;
		entry_size = elf32->e_phentsize;
		entry_count = elf32->e_phnum;
		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(8);
	}

	size = entry_size * entry_count;
	address = map(size);

	elf->program_header_table.offset = offset;
	elf->program_header_table.entry_size = entry_size;
	elf->program_header_table.entry_count = entry_count;
	elf->program_header_table.memory.count = size;
	elf->program_header_table.memory.pointer = address;

	seek_to(elf->file_descriptor, offset);
	read_bytes(elf->file_descriptor, elf->program_header_table.memory);
}

static void set_lone_entry(struct elf *elf)
{
	unsigned char *table = elf->program_header_table.memory.pointer;
	size_t entry_count = elf->program_header_table.entry_count;
	Elf32_Phdr *pt_phdr32 = 0, *phdr32;
	Elf64_Phdr *pt_phdr64 = 0, *phdr64;
	size_t base;
	size_t i;

	switch (elf->class) {
	case ELFCLASS64:
		for (i = 0; i < entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;

			switch (phdr64->p_type) {
			case PT_NULL: // linker allocated spare segment
set_lone_entry_64:
				phdr64->p_type = PT_LOAD;

			case PT_LONE: // existing lone segment

				base = phdr64->p_vaddr - phdr64->p_offset;
				phdr64->p_filesz = phdr64->p_memsz = elf->data.size;
				phdr64->p_vaddr = phdr64->p_paddr = base + elf->data.offset;
				phdr64->p_offset = elf->data.offset;
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
			linux_exit(9);
		}
		break;
	case ELFCLASS32:
		for (i = 0; i < entry_count; ++i) {
			phdr32 = ((Elf32_Phdr *) table) + i;

			switch (phdr32->p_type) {
			case PT_NULL: // linker allocated spare segment
set_lone_entry_32:
				phdr32->p_type = PT_LOAD;

			case PT_LONE: // existing lone segment

				base = phdr32->p_vaddr - phdr32->p_offset;
				phdr32->p_filesz = phdr32->p_memsz = elf->data.size;
				phdr32->p_vaddr = phdr32->p_paddr = base + elf->data.offset;
				phdr32->p_offset = elf->data.offset;
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
			linux_exit(9);
		}
		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(8);
	}

lone_entry_set:
	return;
}

static void open_files(struct elf *elf, char *elf_path, char *data_path)
{
	elf->file_descriptor = open_path(elf_path);
	elf->data.file_descriptor = open_path(data_path);
}

static void compute_sizes_and_offsets(struct elf *elf)
{
	elf->data.size = file_size(elf->data.file_descriptor);
	elf->data.offset = file_size(elf->file_descriptor);
}

static void patch_program_header_table(struct elf *elf)
{
	seek_to(elf->file_descriptor, elf->program_header_table.offset);
	write_bytes(elf->file_descriptor, elf->program_header_table.memory);
}

static void append_data(struct elf *elf)
{
	static unsigned char io_buffer[IO_BUFFER_SIZE];
	static struct lone_bytes input_buffer = { sizeof(io_buffer), io_buffer }, output_buffer = { 0, io_buffer };
	size_t total;

	seek_to_end(elf->file_descriptor);

	while ((output_buffer.count = read_bytes(elf->data.file_descriptor, input_buffer)) > 0) {
		write_bytes(elf->file_descriptor, output_buffer);
	}
}

static void patch_elf(struct elf *elf)
{
	patch_program_header_table(elf);
	append_data(elf);
}

long lone(int argc, char **argv, char **envp, struct auxiliary_vector *auxvec)
{
	static unsigned char elf_header_buffer[0x40];
	struct elf elf = { .header = { sizeof(elf_header_buffer), elf_header_buffer } };

	check_arguments(argc, argv);
	open_files(&elf, argv[1], argv[2]);

	validate_elf_header(&elf);

	compute_sizes_and_offsets(&elf);
	load_program_header_table(&elf);

	set_lone_entry(&elf);

	patch_elf(&elf);

	return 0;
}

#include <lone/architecture/linux/entry_point.c>
