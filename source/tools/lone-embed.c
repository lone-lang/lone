/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone.h>
#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/elf.h>
#include <lone/linux.h>
#include <lone/auxiliary_vector.h>
#include <lone/memory/functions.h>

#ifndef IO_BUFFER_SIZE
#define IO_BUFFER_SIZE 1024 * 4096
#endif

#define REQUIRED_PT_NULLS 2

#define LONE_TOOLS_EMBED_EXIT_INVALID_ELF           8
#define LONE_TOOLS_EMBED_EXIT_MISSING_NULL_ENTRY    9
#define LONE_TOOLS_EMBED_EXIT_MULTIPLE_PHDR_ENTRIES 10
#define LONE_TOOLS_EMBED_EXIT_MISSING_PHDR_ENTRY    11
#define LONE_TOOLS_EMBED_EXIT_OVERFLOW              250

struct elf {
	struct lone_bytes header;
	enum lone_elf_ident_class class;
	size_t page_size;

	struct {
		struct {
			lone_elf_umax file;
			lone_elf_umax virtual;
			lone_elf_umax physical;
		} start;
		struct {
			lone_elf_umax file;
			lone_elf_umax virtual;
			lone_elf_umax physical;
		} end;
	} limits;

	struct {
		struct lone_bytes memory;
		struct lone_elf_segments table;
		lone_elf_umax offset;
		lone_u16 nulls_count;
	} segments;

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

static size_t pht_size_for(struct elf *elf, size_t entry_count)
{
	return elf->segments.table.segment.size * entry_count;
}

static size_t pht_size(struct elf *elf)
{
	return pht_size_for(elf, elf->segments.table.segment.count);
}

static void *pht_end(struct elf *elf)
{
	return elf->segments.table.segments + pht_size(elf);
}

static bool has_required_null_segments(struct elf *elf)
{
	return elf->segments.nulls_count >= REQUIRED_PT_NULLS;
}

static size_t align(size_t n, size_t a) { return ((size_t) ((n + (a - 1)) / a)) * a; }
static size_t align_to_page(struct elf *elf, size_t n) { return align(n, elf->page_size); }
static lone_elf_umax min(lone_elf_umax x, lone_elf_umax y) { return x < y? x : y; }
static lone_elf_umax max(lone_elf_umax x, lone_elf_umax y) { return x > y? x : y; }

static void invalid_elf(void)
{
	linux_exit(LONE_TOOLS_EMBED_EXIT_INVALID_ELF);
}

static void overflow(void)
{
	linux_exit(LONE_TOOLS_EMBED_EXIT_OVERFLOW);
}

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

static void read_elf_header(struct elf *elf)
{
	read_bytes(elf->file.descriptor, elf->header);
}

static void validate_elf_header(struct elf *elf)
{
	elf->class = LONE_ELF_IDENT_CLASS_INVALID;

	if (lone_elf_header_is_valid(elf->header.pointer)) {
		elf->class = elf->header.pointer[LONE_ELF_IDENT_INDEX_CLASS];
	} else {
		invalid_elf();
	}
}

static void load_program_header_table(struct elf *elf)
{
	struct lone_elf_optional_umax offset;
	struct lone_optional_u16 entry_size, entry_count;
	lone_elf_umax size;
	void *address;

	offset = lone_elf_header_read_segments_offset(elf->header.pointer);
	if (!offset.present) { invalid_elf(); }

	entry_size = lone_elf_header_read_segment_size(elf->header.pointer);
	if (!entry_size.present) { invalid_elf(); }

	entry_count = lone_elf_header_read_segment_count(elf->header.pointer);
	if (!entry_count.present) { invalid_elf(); }

	elf->segments.nulls_count = 0;
	elf->segments.offset = offset.value;
	elf->segments.table.segment.size = entry_size.value;
	elf->segments.table.segment.count = entry_count.value;

	size = pht_size_for(elf, entry_count.value + REQUIRED_PT_NULLS + 1);
	address = map(size);

	elf->segments.table.segments = address;
	elf->segments.memory.pointer = address;
	elf->segments.memory.count = size;

	seek_to(elf->file.descriptor, offset.value);
	read_bytes(elf->file.descriptor, elf->segments.memory);
}

static lone_u32 segment_read_u32(struct lone_elf_header *header,
		struct lone_elf_segment *segment,
		struct lone_optional_u32 (*reader)(struct lone_elf_header *, struct lone_elf_segment *))
{
	struct lone_optional_u32 read;
	read = reader(header, segment);
	if (!read.present) { invalid_elf(); }
	return read.value;
}

static void segment_write_u32(struct lone_elf_header *header,
		struct lone_elf_segment *segment,
		bool (*writer)(struct lone_elf_header *, struct lone_elf_segment *, lone_u32),
		lone_u32 value)
{
	if (!writer(header, segment, value)) { invalid_elf(); }
}

static lone_elf_umax segment_read_umax(struct lone_elf_header *header,
		struct lone_elf_segment *segment,
		struct lone_elf_optional_umax (*reader)(struct lone_elf_header *, struct lone_elf_segment *))
{
	struct lone_elf_optional_umax read;
	read = reader(header, segment);
	if (!read.present) { invalid_elf(); }
	return read.value;
}

static void segment_write_umax(struct lone_elf_header *header,
		struct lone_elf_segment *segment,
		bool (*writer)(struct lone_elf_header *, struct lone_elf_segment *, lone_elf_umax),
		lone_elf_umax value)
{
	if (!writer(header, segment, value)) { invalid_elf(); }
}

static void set_start_end(lone_elf_umax *start, lone_elf_umax *end,
		lone_elf_umax address_or_offset, lone_elf_umax size,
		int exit_code)
{
	lone_elf_umax limit;

	if (!start || !end || __builtin_add_overflow(address_or_offset, size, &limit)) {
		linux_exit(exit_code);
	}

	*start = min(*start, address_or_offset);
	*end = max(*end, limit);
}

static void analyze(struct elf *elf)
{
	struct lone_elf_header *header = elf->header.pointer;
	lone_u16 entry_count = elf->segments.table.segment.count;
	struct lone_elf_segment *segment;
	struct lone_optional_u32 type;
	lone_u16 i;

	elf->limits.start.file = -1ULL;
	elf->limits.start.virtual = -1ULL;
	elf->limits.start.physical = -1ULL;
	elf->limits.end.file = 0;
	elf->limits.end.virtual = 0;
	elf->limits.end.physical = 0;
	elf->segments.nulls_count = 0;

	for (i = 0; i < entry_count; ++i) {
		segment = lone_elf_segment_at(elf->segments.table, i);
		if (!segment) { overflow(); }

		type = lone_elf_segment_read_type(header, segment);
		if (!type.present) { invalid_elf(); }

		if (type.value == LONE_ELF_SEGMENT_TYPE_LOADABLE) {

			set_start_end(&elf->limits.start.file, &elf->limits.end.file,
			              segment_read_umax(header, segment, lone_elf_segment_read_file_offset),
			              segment_read_umax(header, segment, lone_elf_segment_read_size_in_file),
			              8);

			set_start_end(&elf->limits.start.virtual, &elf->limits.end.virtual,
			              segment_read_umax(header, segment, lone_elf_segment_read_virtual_address),
			              segment_read_umax(header, segment, lone_elf_segment_read_size_in_memory),
			              8);

			set_start_end(&elf->limits.start.physical, &elf->limits.end.physical,
			              segment_read_umax(header, segment, lone_elf_segment_read_physical_address),
			              segment_read_umax(header, segment, lone_elf_segment_read_size_in_memory),
			              8);

		} else if (type.value == LONE_ELF_SEGMENT_TYPE_NULL) {
			++elf->segments.nulls_count;
		}
	}

	elf->data.offset = align_to_page(elf, elf->file.size);
}

static void adjust_phdr_entry(struct elf *elf)
{
	size_t entry_count, i;
	struct lone_elf_header *header;
	struct lone_elf_segment *segments, *segment, *phdr, *load;
	lone_elf_umax alignment, size, size_aligned, offset, virtual, physical;
	lone_u32 type;

	header = elf->header.pointer;
	segments = elf->segments.table.segments;
	entry_count = elf->segments.table.segment.count;
	phdr = 0;
	load = 0;

	for (i = 0; i < entry_count; ++i) {
		segment = lone_elf_segment_at(elf->segments.table, i);
		if (!segment) { overflow(); }

		type = segment_read_u32(header, segment, lone_elf_segment_read_type);

		if (type == LONE_ELF_SEGMENT_TYPE_PHDR) {
			if (phdr) {
				linux_exit(LONE_TOOLS_EMBED_EXIT_MULTIPLE_PHDR_ENTRIES);
			} else {
				phdr = segment;
			}
		} else if (type == LONE_ELF_SEGMENT_TYPE_NULL) {
			load = segment;
			break;
		}
	}

	if (!phdr) {
		linux_exit(LONE_TOOLS_EMBED_EXIT_MISSING_PHDR_ENTRY);
	}

	if (!load) {
		linux_exit(LONE_TOOLS_EMBED_EXIT_MISSING_NULL_ENTRY);
	}

	alignment = elf->page_size;
	size = pht_size(elf);
	size_aligned = align_to_page(elf, size);
	offset = elf->segments.offset;
	virtual = align_to_page(elf, elf->limits.end.virtual);
	physical = align_to_page(elf, elf->limits.end.physical);

	segment_write_umax(header, phdr, lone_elf_segment_write_file_offset, offset);
	segment_write_umax(header, phdr, lone_elf_segment_write_virtual_address, virtual);
	segment_write_umax(header, phdr, lone_elf_segment_write_physical_address, physical);
	segment_write_umax(header, phdr, lone_elf_segment_write_size_in_file, size);
	segment_write_umax(header, phdr, lone_elf_segment_write_size_in_memory, size);

	segment_write_u32(header,  load, lone_elf_segment_write_type, LONE_ELF_SEGMENT_TYPE_LOAD);
	segment_write_u32(header,  load, lone_elf_segment_write_flags, LONE_ELF_SEGMENT_FLAGS_R);
	segment_write_umax(header, load, lone_elf_segment_write_file_offset, offset);
	segment_write_umax(header, load, lone_elf_segment_write_virtual_address, virtual);
	segment_write_umax(header, load, lone_elf_segment_write_physical_address, physical);
	segment_write_umax(header, load, lone_elf_segment_write_size_in_file, size_aligned);
	segment_write_umax(header, load, lone_elf_segment_write_size_in_memory, size_aligned);
	segment_write_umax(header, load, lone_elf_segment_write_alignment, alignment);
}

static void move_and_expand_pht_if_needed(struct elf *elf)
{
	void *new_nulls;

	if (has_required_null_segments(elf)) { return; }

	new_nulls = pht_end(elf);

	elf->segments.offset = elf->data.offset;
	elf->segments.table.segment.count += REQUIRED_PT_NULLS + 1;
	elf->file.size = elf->segments.offset + pht_size(elf);

	lone_memory_zero(new_nulls, pht_size_for(elf, REQUIRED_PT_NULLS + 1));
	adjust_phdr_entry(elf);
	analyze(elf);
}

static void set_lone_segments(struct elf *elf)
{
	size_t entry_count = elf->segments.table.segment.count;
	void *table = elf->segments.table.segments;
	bool set_load_segment = false, set_lone_segment = false;
	Elf32_Phdr *phdr32;
	Elf64_Phdr *phdr64;
	size_t i;

	if (!has_required_null_segments(elf)) {
		/* Not enough null segments to patch this ELF */ linux_exit(9);
	}

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

			__attribute__((fallthrough));
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
					phdr32->p_type = PT_LONE;

					phdr32->p_offset = elf->data.offset;
					phdr32->p_vaddr = phdr32->p_paddr = align_to_page(elf, elf->limits.end.virtual);
					phdr32->p_filesz = phdr32->p_memsz = elf->data.size;
					phdr32->p_align = 1;
					phdr32->p_flags = PF_R;

					set_lone_segment = true;

				} else {
					break;
				}

			__attribute__((fallthrough));
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
	seek_to(elf->file.descriptor, elf->segments.offset);
	write_bytes(elf->file.descriptor, elf->segments.memory);
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

static void patch_ehdr_if_needed(struct elf *elf)
{
	bool patching_required;
	Elf32_Ehdr *elf32;
	Elf64_Ehdr *elf64;

	patching_required = false;

	switch (elf->class) {
	case ELFCLASS64:
		elf64 = (Elf64_Ehdr *) elf->header.pointer;

		if (elf64->e_phoff != elf->segments.offset) {
			elf64->e_phoff = elf->segments.offset;
			elf64->e_phnum = elf->segments.table.segment.count;
			patching_required = true;
		}

		break;
	case ELFCLASS32:
		elf32 = (Elf32_Ehdr *) elf->header.pointer;

		if (elf32->e_phoff != elf->segments.offset) {
			elf32->e_phoff = elf->segments.offset;
			elf32->e_phnum = elf->segments.table.segment.count;
			patching_required = true;
		}

		break;
	default:
		/* Invalid ELF class but somehow made it here? */ linux_exit(8);
	}

	if (patching_required) {
		seek_to_start(elf->file.descriptor);
		write_bytes(elf->file.descriptor, elf->header);
	}
}

static void patch(struct elf *elf)
{
	patch_program_header_table(elf);
	append_data(elf);
	patch_ehdr_if_needed(elf);
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxvec)
{
	static unsigned char elf_header_buffer[0x40];
	struct elf elf = { .header = { sizeof(elf_header_buffer), elf_header_buffer } };

	check_arguments(argc, argv);
	open_files(&elf, argv[1], argv[2]);

	read_elf_header(&elf);
	validate_elf_header(&elf);

	set_page_size(&elf, auxvec);
	query_file_sizes(&elf);
	load_program_header_table(&elf);
	analyze(&elf);

	move_and_expand_pht_if_needed(&elf);
	set_lone_segments(&elf);

	patch(&elf);

	return 0;
}

#include <lone/architecture/linux/entry_point.c>
