/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone.h>
#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/auxiliary_vector.h>
#include <lone/memory/functions.h>

#ifndef IO_BUFFER_SIZE
#define IO_BUFFER_SIZE 1024 * 4096
#endif

#define REQUIRED_PT_NULLS 2

struct elf {
	struct lone_bytes header;
	unsigned char class;
	size_t page_size;

	struct {
		struct {
			size_t file;
			size_t virtual;
			size_t physical;
		} start;
		struct {
			size_t file;
			size_t virtual;
			size_t physical;
		} end;
	} limits;

	struct {
		size_t offset;
		size_t entry_size;
		size_t entry_count;
		size_t nulls_count;
		struct lone_bytes memory;
	} program_header_table;

	struct {
		int descriptor;
		size_t size;
	} file;

	struct {
		int file_descriptor;
		size_t offset;
		size_t size;
	} data;
};

static size_t align(size_t n, size_t a) { return ((size_t) ((n + (a - 1)) / a)) * a; }
static size_t align_to_page(struct elf *elf, size_t n) { return align(n, elf->page_size); }
static size_t min(size_t x, size_t y) { return x < y? x : y; }
static size_t max(size_t x, size_t y) { return x > y? x : y; }

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
	return seek_to_end(fd);
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
	read_bytes(elf->file.descriptor, elf->header);

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
	size_t offset, entry_size, entry_count, size;
	Elf32_Ehdr *elf32;
	Elf64_Ehdr *elf64;
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
	elf->program_header_table.nulls_count = 0;
	elf->program_header_table.memory.count = size;
	elf->program_header_table.memory.pointer = address;

	seek_to(elf->file.descriptor, offset);
	read_bytes(elf->file.descriptor, elf->program_header_table.memory);
}

static void analyze(struct elf *elf)
{
	size_t entry_count = elf->program_header_table.entry_count;
	void *table = elf->program_header_table.memory.pointer;
	struct { size_t file, virtual, physical; } start = {-1, -1, -1}, end = {0, 0, 0};
	Elf32_Phdr *phdr32;
	Elf64_Phdr *phdr64;
	size_t i;

	elf->program_header_table.nulls_count = 0;

	switch (elf->class) {
	case ELFCLASS64:
		for (i = 0; i < entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;

			if (phdr64->p_type == PT_LOAD) {
				start.file = min(phdr64->p_offset, start.file);
				end.file = max(phdr64->p_offset + phdr64->p_filesz, end.file);

				start.virtual = min(phdr64->p_vaddr, start.virtual);
				end.virtual = max(phdr64->p_vaddr + phdr64->p_memsz, end.virtual);

				start.physical = min(phdr64->p_paddr, start.physical);
				end.physical = max(phdr64->p_paddr + phdr64->p_memsz, end.physical);

			} else if (phdr64->p_type == PT_NULL) {
				++elf->program_header_table.nulls_count;
			}
		}

		break;
	case ELFCLASS32:
		for (i = 0; i < entry_count; ++i) {
			phdr32 = ((Elf32_Phdr *) table) + i;

			if (phdr32->p_type == PT_LOAD) {
				start.file = min(phdr32->p_offset, start.file);
				end.file = max(phdr32->p_offset + phdr32->p_filesz, end.file);

				start.virtual = min(phdr32->p_vaddr, start.virtual);
				end.virtual = max(phdr32->p_vaddr + phdr32->p_memsz, end.virtual);

				start.physical = min(phdr32->p_paddr, start.physical);
				end.physical = max(phdr32->p_paddr + phdr32->p_memsz, end.physical);

			} else if (phdr32->p_type == PT_NULL) {
				++elf->program_header_table.nulls_count;
			}
		}

		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(8);
	}

	if (elf->program_header_table.nulls_count < REQUIRED_PT_NULLS) {
		/* Not enough null segments to patch this ELF */ linux_exit(9);
	}

	elf->limits.start.file = start.file;
	elf->limits.start.virtual = start.virtual;
	elf->limits.start.physical = start.physical;
	elf->limits.end.file = end.file;
	elf->limits.end.virtual = end.virtual;
	elf->limits.end.physical = end.physical;

	elf->data.offset = align_to_page(elf, elf->file.size);
}

static void set_lone_segments(struct elf *elf)
{
	size_t entry_count = elf->program_header_table.entry_count;
	void *table = elf->program_header_table.memory.pointer;
	bool set_load_segment = false, set_lone_segment = false;
	Elf32_Phdr *phdr32;
	Elf64_Phdr *phdr64;
	size_t i;

	switch (elf->class) {
	case ELFCLASS64:
		for (i = 0; i < entry_count; ++i) {
			phdr64 = ((Elf64_Phdr *) table) + i;

			switch (phdr64->p_type) {
			case PT_NULL: // linker allocated spare segment

				if (!set_load_segment) {
					phdr64->p_type = PT_LOAD;

					phdr64->p_offset = elf->data.offset;
					phdr64->p_vaddr = phdr64->p_paddr = align_to_page(elf, elf->limits.end.virtual);
					phdr64->p_filesz = phdr64->p_memsz = align_to_page(elf, elf->data.size);
					phdr64->p_align = elf->page_size;
					phdr64->p_flags = PF_R;

					set_load_segment = true;

				} else if (!set_lone_segment) {
					phdr64->p_type = PT_LONE;

					phdr64->p_offset = elf->data.offset;
					phdr64->p_vaddr = phdr64->p_paddr = align_to_page(elf, elf->limits.end.virtual);
					phdr64->p_filesz = phdr64->p_memsz = elf->data.size;
					phdr64->p_align = 1;
					phdr64->p_flags = PF_R;

					set_lone_segment = true;

				} else {
					break;
				}

			default:
				continue;
			}

			if (set_lone_segment && set_load_segment) {
				break;
			}
		}

		break;
	case ELFCLASS32:
		for (i = 0; i < entry_count; ++i) {
			phdr32 = ((Elf32_Phdr *) table) + i;

			switch (phdr32->p_type) {
			case PT_NULL: // linker allocated spare segment

				if (!set_load_segment) {
					phdr32->p_type = PT_LOAD;

					phdr32->p_offset = elf->data.offset;
					phdr32->p_vaddr = phdr32->p_paddr = align_to_page(elf, elf->limits.end.virtual);
					phdr32->p_filesz = phdr32->p_memsz = align_to_page(elf, elf->data.size);
					phdr32->p_align = elf->page_size;
					phdr32->p_flags = PF_R;

					set_load_segment = true;

				} else if (!set_lone_segment) {
					phdr64->p_type = PT_LONE;

					phdr32->p_offset = elf->data.offset;
					phdr32->p_vaddr = phdr32->p_paddr = align_to_page(elf, elf->limits.end.virtual);
					phdr32->p_filesz = phdr32->p_memsz = elf->data.size;
					phdr32->p_align = 1;
					phdr32->p_flags = PF_R;

					set_lone_segment = true;

				} else {
					break;
				}

			default:
				continue;
			}

			if (set_lone_segment && set_load_segment) {
				break;
			}
		}

		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(8);
	}
}

static void set_page_size(struct elf *elf, struct lone_auxiliary_vector *auxvec)
{
	elf->page_size = lone_auxiliary_vector_page_size(auxvec);
}

static void open_files(struct elf *elf, char *elf_path, char *data_path)
{
	elf->file.descriptor = open_path(elf_path);
	elf->data.file_descriptor = open_path(data_path);
}

static void query_file_sizes(struct elf *elf)
{
	elf->file.size = file_size(elf->file.descriptor);
	elf->data.size = file_size(elf->data.file_descriptor);
}

static void patch_program_header_table(struct elf *elf)
{
	seek_to(elf->file.descriptor, elf->program_header_table.offset);
	write_bytes(elf->file.descriptor, elf->program_header_table.memory);
}

static void append_data(struct elf *elf)
{
	static unsigned char io_buffer[IO_BUFFER_SIZE];
	static struct lone_bytes input_buffer = { sizeof(io_buffer), io_buffer }, output_buffer = { 0, io_buffer };

	seek_to_start(elf->data.file_descriptor);
	seek_to(elf->file.descriptor, elf->data.offset);

	while ((output_buffer.count = read_bytes(elf->data.file_descriptor, input_buffer)) > 0) {
		write_bytes(elf->file.descriptor, output_buffer);
	}
}

static void patch(struct elf *elf)
{
	patch_program_header_table(elf);
	append_data(elf);
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxvec)
{
	static unsigned char elf_header_buffer[0x40];
	struct elf elf = { .header = { sizeof(elf_header_buffer), elf_header_buffer } };

	check_arguments(argc, argv);
	open_files(&elf, argv[1], argv[2]);

	validate_elf_header(&elf);

	set_page_size(&elf, auxvec);
	query_file_sizes(&elf);
	load_program_header_table(&elf);
	analyze(&elf);

	set_lone_segments(&elf);

	patch(&elf);

	return 0;
}

#include <lone/architecture/linux/entry_point.c>
