/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone.h>
#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/elf.h>
#include <lone/linux.h>
#include <lone/auxiliary_vector.h>
#include <lone/memory/functions.h>

#ifndef IO_BUFFER_SIZE
#define IO_BUFFER_SIZE (1024 * 4096)
#endif

/* REQUIRED_PT_NULLS encodes the number of PT_NULL entries
 * that set_lone_segments later repurposes into the new
 * PT_LOAD and the PT_LONE describing the embedded segment.
 *
 * MAX_NEW_SEGMENTS encodes the upper bound on segments table
 * entries appended by move_and_expand_pht_if_needed:
 *
 *     REQUIRED_PT_NULLS - if missing
 *     PT_PHDR           - if missing
 *     PT_LOAD           - covering the relocated segments
 *
 * load_program_header_table allocates room for every original entry
 * plus this upper bound so pht_end and the null-zeroing write never
 * walk off the mapped buffer. If the expansion logic ever needs more
 * slots, these constants must be adjusted, or the runtime check in
 * move_and_expand_pht_if_needed will trip and abort.
 */
#define REQUIRED_PT_NULLS 2
#define MAX_NEW_SEGMENTS (REQUIRED_PT_NULLS + 2)

#define LONE_TOOLS_EMBED_EXIT_INVALID_ELF             8
#define LONE_TOOLS_EMBED_EXIT_MISSING_NULL_ENTRY      9
#define LONE_TOOLS_EMBED_EXIT_MULTIPLE_PHDR_ENTRIES  10
#define LONE_TOOLS_EMBED_EXIT_LOAD_ADDRESS_UNALIGNED 11
#define LONE_TOOLS_EMBED_EXIT_INCORRECT_LOAD_BIAS    12
#define LONE_TOOLS_EMBED_EXIT_OVERFLOW              250

struct elf {
	struct lone_bytes header;
	enum lone_elf_ident_class class;
	size_t page_size;
	lone_elf_umax load_address;

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
		lone_elf_umax file_offset;
		lone_u16 nulls_count;
		bool has_phdr;
	} segments;

	struct {
		int descriptor;
		size_t size;
	} file;

	struct {
		int file_descriptor;
		size_t file_offset;
		size_t size;
	} data;
};

static struct lone_elf_header *
hdr(struct elf *elf)
{
	return (struct lone_elf_header *) elf->header.pointer;
}

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

static void not_enough_nulls(void)
{
	linux_exit(LONE_TOOLS_EMBED_EXIT_MISSING_NULL_ENTRY);
}

static void check_arguments(int argc, char **argv)
{
	/* argv[1] is the ELF path, argv[2] the segment data path */
	if (argc <= 2) { linux_exit(1); }
}

static int open_path(char *path, int flags)
{
	int fd = linux_openat(AT_FDCWD, (unsigned char *) path, flags | O_CLOEXEC);
	if (fd < 0) { /* error opening file descriptor */ linux_exit(2); }
	return fd;
}

static size_t read_bytes(int fd, struct lone_bytes buffer)
{
	ssize_t result;

	result = linux_read_bytes(fd, buffer);
	if (result < 0) { /* error reading input file */ linux_exit(3); }

	return (size_t) result;
}

static size_t write_bytes(int fd, struct lone_bytes buffer)
{
	ssize_t result;

	result = linux_write_bytes(fd, buffer);
	if (result < 0 || (size_t) result != buffer.count) { /* error writing output file */ linux_exit(4); }

	return buffer.count;
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
	if (read_bytes(elf->file.descriptor, elf->header) != elf->header.count) {
		invalid_elf();
	}
}

static void validate_elf_header(struct elf *elf)
{
	struct lone_optional_u16 type;

	elf->class = LONE_ELF_IDENT_CLASS_INVALID;

	if (!lone_elf_header_is_valid(hdr(elf))) { invalid_elf(); }

	/* lone_elf_header_is_valid certifies
	   the ELF header against the ELF spec
	   which includes ET_REL or ET_CORE
	   lone-embed can only meaningfully
	   patch runnable binaries though        */
	type = lone_elf_header_read_type(hdr(elf));
	if (!type.present) { invalid_elf(); }
	if (   type.value != LONE_ELF_TYPE_EXECUTABLE
	    && type.value != LONE_ELF_TYPE_DYNAMIC) { invalid_elf(); }

	elf->class = elf->header.pointer[LONE_ELF_IDENT_INDEX_CLASS];
}

static void load_program_header_table(struct elf *elf)
{
	struct lone_elf_optional_umax file_offset;
	struct lone_optional_u16 entry_size, entry_count;
	struct lone_bytes target;
	lone_elf_umax size;
	void *address;

	file_offset = lone_elf_header_read_segments_offset(hdr(elf));
	if (!file_offset.present) { invalid_elf(); }

	entry_size = lone_elf_header_read_segment_size(hdr(elf));
	if (!entry_size.present) { invalid_elf(); }

	entry_count = lone_elf_header_read_segment_count(hdr(elf));
	if (!entry_count.present) { invalid_elf(); }

	elf->segments.nulls_count = 0;
	elf->segments.file_offset = file_offset.value;
	elf->segments.table.segment.size = entry_size.value;
	elf->segments.table.segment.count = entry_count.value;

	size = pht_size_for(elf, entry_count.value + MAX_NEW_SEGMENTS);
	address = map(size);

	elf->segments.table.segments = address;
	elf->segments.memory.pointer = address;
	elf->segments.memory.count = size;

	seek_to(elf->file.descriptor, file_offset.value);
	target = (struct lone_bytes) { pht_size(elf), elf->segments.memory.pointer };
	if (read_bytes(elf->file.descriptor, target) != target.count) { invalid_elf(); }
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
	struct lone_elf_header *header = hdr(elf);
	lone_u16 entry_count = elf->segments.table.segment.count;
	struct lone_elf_segment *segment;
	struct lone_optional_u32 type;
	bool found_first_load;
	lone_u16 i;

	elf->limits.start.file = -1ULL;
	elf->limits.start.virtual = -1ULL;
	elf->limits.start.physical = -1ULL;
	elf->limits.end.file = 0;
	elf->limits.end.virtual = 0;
	elf->limits.end.physical = 0;
	elf->segments.nulls_count = 0;
	elf->segments.has_phdr = false;
	elf->load_address = 0;
	found_first_load = false;

	for (i = 0; i < entry_count; ++i) {
		segment = lone_elf_segment_at(elf->segments.table, i);
		if (!segment) { overflow(); }

		type = lone_elf_segment_read_type(header, segment);
		if (!type.present) { invalid_elf(); }

		if (type.value == LONE_ELF_SEGMENT_TYPE_LOADABLE) {

			if (!found_first_load) {
				elf->load_address =
					  segment_read_umax(header, segment, lone_elf_segment_read_virtual_address)
					- segment_read_umax(header, segment, lone_elf_segment_read_file_offset);
				found_first_load = true;
			}

			set_start_end(&elf->limits.start.file, &elf->limits.end.file,
			              segment_read_umax(header, segment, lone_elf_segment_read_file_offset),
			              segment_read_umax(header, segment, lone_elf_segment_read_size_in_file),
			              LONE_TOOLS_EMBED_EXIT_INVALID_ELF);

			set_start_end(&elf->limits.start.virtual, &elf->limits.end.virtual,
			              segment_read_umax(header, segment, lone_elf_segment_read_virtual_address),
			              segment_read_umax(header, segment, lone_elf_segment_read_size_in_memory),
			              LONE_TOOLS_EMBED_EXIT_INVALID_ELF);

			set_start_end(&elf->limits.start.physical, &elf->limits.end.physical,
			              segment_read_umax(header, segment, lone_elf_segment_read_physical_address),
			              segment_read_umax(header, segment, lone_elf_segment_read_size_in_memory),
			              LONE_TOOLS_EMBED_EXIT_INVALID_ELF);

		} else if (type.value == LONE_ELF_SEGMENT_TYPE_PHDR) {
			elf->segments.has_phdr = true;
		} else if (type.value == LONE_ELF_SEGMENT_TYPE_NULL) {
			++elf->segments.nulls_count;
		}
	}

	elf->data.file_offset = align_to_page(elf, elf->file.size);
}

static void adjust_phdr_entry(struct elf *elf)
{
	size_t entry_count, i;
	struct lone_elf_header *header;
	struct lone_elf_segment *segment, *phdr, *load, *null_1, *null_2;
	lone_elf_umax file_offset, required_file_offset;
	lone_elf_umax alignment, size, size_aligned;
	lone_elf_umax loaded_vaddr, loaded_offset;
	lone_elf_umax virtual, physical;
	lone_u32 type;
	bool created_phdr;

	header = hdr(elf);
	entry_count = elf->segments.table.segment.count;
	phdr = 0;
	null_1 = 0;
	null_2 = 0;

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
			if (!null_1) {
				null_1 = segment;
			} else if (!null_2) {
				null_2 = segment;
			}
		}
	}

	if (phdr) {
		/* PT_PHDR exists, use first NULL for PT_LOAD */
		load = null_1;
		created_phdr = false;
	} else {
		/* no PHDR found so repurpose the first NULL entry
		 * as one and use the second NULL for the LOAD entry
		 * guarantees every patched ELF contains a PHDR entry
		 * pointing at the relocated program header table
		 * linux < 5.18 ignores it but newer kernels do not  */
		phdr = null_1;
		load = null_2;
		created_phdr = true;
	}

	if (!phdr || !load) { not_enough_nulls(); }

	if (elf->load_address % elf->page_size != 0) {
		linux_exit(LONE_TOOLS_EMBED_EXIT_LOAD_ADDRESS_UNALIGNED);
	}

	alignment = elf->page_size;
	size = pht_size(elf);
	size_aligned = align_to_page(elf, size);

	required_file_offset = align_to_page(elf, elf->limits.end.virtual - elf->load_address);
	file_offset = max(elf->segments.file_offset, required_file_offset);
	elf->segments.file_offset = file_offset;

	/* Required by linux < 5.18:
	 *
	 * 	AT_PHDR = (first_LOAD.p_vaddr - first_LOAD.p_offset) + e_phoff
	 *
	 * Which gives:
	 *
	 * 	new_LOAD.p_vaddr  = load_address + new_LOAD.p_offset
	 * 	new_LOAD.p_offset = e_phoff
	 *
	 */
	virtual = elf->load_address + file_offset;

	physical = virtual;

	if (created_phdr) {
		segment_write_u32(
			header,
			phdr,
			lone_elf_segment_write_type,
			LONE_ELF_SEGMENT_TYPE_PHDR
		);
		segment_write_u32(
			header,
			phdr,
			lone_elf_segment_write_flags,
			LONE_ELF_SEGMENT_FLAGS_R
		);
		segment_write_umax(
			header,
			phdr,
			lone_elf_segment_write_alignment,
			elf->class == LONE_ELF_IDENT_CLASS_64BIT? 8 : 4
		);
	}

	segment_write_umax(header, phdr, lone_elf_segment_write_file_offset, file_offset);
	segment_write_umax(header, phdr, lone_elf_segment_write_virtual_address, virtual);
	segment_write_umax(header, phdr, lone_elf_segment_write_physical_address, physical);
	segment_write_umax(header, phdr, lone_elf_segment_write_size_in_file, size);
	segment_write_umax(header, phdr, lone_elf_segment_write_size_in_memory, size);

	segment_write_u32(header,  load, lone_elf_segment_write_type, LONE_ELF_SEGMENT_TYPE_LOAD);
	segment_write_u32(header,  load, lone_elf_segment_write_flags, LONE_ELF_SEGMENT_FLAGS_R);
	segment_write_umax(header, load, lone_elf_segment_write_file_offset, file_offset);
	segment_write_umax(header, load, lone_elf_segment_write_virtual_address, virtual);
	segment_write_umax(header, load, lone_elf_segment_write_physical_address, physical);
	segment_write_umax(header, load, lone_elf_segment_write_size_in_file, size_aligned);
	segment_write_umax(header, load, lone_elf_segment_write_size_in_memory, size_aligned);
	segment_write_umax(header, load, lone_elf_segment_write_alignment, alignment);

	/* verify the linux < 5.18 invariant holds */
	loaded_vaddr  = segment_read_umax(header, load, lone_elf_segment_read_virtual_address);
	loaded_offset = segment_read_umax(header, load, lone_elf_segment_read_file_offset);
	if (loaded_vaddr != elf->load_address + loaded_offset) {
		linux_exit(LONE_TOOLS_EMBED_EXIT_INCORRECT_LOAD_BIAS);
	}
}

static void move_and_expand_pht_if_needed(struct elf *elf)
{
	void *new_nulls;
	lone_u16 extra;

	if (has_required_null_segments(elf)) { return; }

	extra = REQUIRED_PT_NULLS + 1;
	if (!elf->segments.has_phdr) { extra += 1; }
	if (extra > MAX_NEW_SEGMENTS) { overflow(); }

	new_nulls = pht_end(elf);

	elf->segments.file_offset = elf->data.file_offset;
	elf->segments.table.segment.count += extra;
	elf->file.size = elf->segments.file_offset + pht_size(elf);

	lone_memory_zero(new_nulls, pht_size_for(elf, extra));
	adjust_phdr_entry(elf);
	analyze(elf);
}

static void
set_segment(struct elf *elf, struct lone_elf_header *header,
		struct lone_elf_segment *segment, enum lone_elf_segment_type type)
{
	lone_elf_umax address, size;
	bool set_true_size;

	set_true_size = type == LONE_ELF_SEGMENT_TYPE_LONE;

	segment_write_u32(header, segment, lone_elf_segment_write_type, type);
	segment_write_umax(header, segment, lone_elf_segment_write_file_offset, elf->data.file_offset);

	address = align_to_page(elf, elf->limits.end.virtual);
	segment_write_umax(header, segment, lone_elf_segment_write_virtual_address, address);

	address = align_to_page(elf, elf->limits.end.physical);
	segment_write_umax(header, segment, lone_elf_segment_write_physical_address, address);

	size = elf->data.size;
	size = set_true_size? size : align_to_page(elf, size);
	segment_write_umax(header, segment, lone_elf_segment_write_size_in_file, size);
	segment_write_umax(header, segment, lone_elf_segment_write_size_in_memory, size);

	segment_write_umax(header, segment, lone_elf_segment_write_alignment, set_true_size? 1 : elf->page_size);
	segment_write_u32(header, segment, lone_elf_segment_write_flags, LONE_ELF_SEGMENT_FLAGS_READABLE);
}

static void set_lone_segments(struct elf *elf)
{
	struct lone_elf_header *header;
	struct lone_elf_segments segments;
	struct lone_elf_segment *segment;
	struct lone_optional_u32 type;
	bool set_load_segment, set_lone_segment;
	lone_u16 i;

	if (!has_required_null_segments(elf)) { not_enough_nulls(); }

	header = hdr(elf);
	segments = elf->segments.table;
	set_load_segment = false;
	set_lone_segment = false;

	for (i = 0; i < segments.segment.count; ++i) {
		if (set_load_segment && set_lone_segment) { break; }

		segment = lone_elf_segment_at(segments, i);
		if (!segment) { overflow(); }
		type = lone_elf_segment_read_type(header, segment);
		if (!type.present) { invalid_elf(); }

		if (type.value != LONE_ELF_SEGMENT_TYPE_NULL) { continue; }

		if (!set_load_segment) {
			set_segment(elf, header, segment, LONE_ELF_SEGMENT_TYPE_LOAD);
			set_load_segment = true;
		} else {
			set_segment(elf, header, segment, LONE_ELF_SEGMENT_TYPE_LONE);
			set_lone_segment = true;
		}
	}
}

static void set_page_size(struct elf *elf, struct lone_auxiliary_vector *auxvec)
{
	/* Linux has no syscall that reports the page size. The auxv
	 * AT_PAGESZ entry is the only runtime source. If it's absent
	 * or zero, fall back to the smallest Linux page size so that
	 * alignment math is never a division by zero. */
	elf->page_size = lone_auxiliary_vector_page_size(auxvec);
	if (elf->page_size == 0) { elf->page_size = 4096; }
}

static void open_files(struct elf *elf, char *elf_path, char *data_path)
{
	elf->file.descriptor = open_path(elf_path, O_RDWR);
	elf->data.file_descriptor = open_path(data_path, O_RDONLY);
}

static void query_file_sizes(struct elf *elf)
{
	elf->file.size = file_size(elf->file.descriptor);
	elf->data.size = file_size(elf->data.file_descriptor);
}

static void patch_program_header_table(struct elf *elf)
{
	struct lone_bytes pht = { pht_size(elf), elf->segments.memory.pointer };
	seek_to(elf->file.descriptor, elf->segments.file_offset);
	write_bytes(elf->file.descriptor, pht);
}

static void append_data(struct elf *elf)
{
	static unsigned char io_buffer[IO_BUFFER_SIZE];
	static struct lone_bytes input_buffer = { sizeof(io_buffer), io_buffer }, output_buffer = { 0, io_buffer };

	seek_to_start(elf->data.file_descriptor);
	seek_to(elf->file.descriptor, elf->data.file_offset);

	while ((output_buffer.count = read_bytes(elf->data.file_descriptor, input_buffer)) > 0) {
		write_bytes(elf->file.descriptor, output_buffer);
	}
}

static void patch_elf_header(struct elf *elf)
{
	seek_to_start(elf->file.descriptor);
	write_bytes(elf->file.descriptor, elf->header);
}

static void patch_ehdr_if_needed(struct elf *elf)
{
	struct lone_elf_header *header;
	struct lone_elf_optional_umax file_offset;

	header = hdr(elf);

	file_offset = lone_elf_header_read_segments_offset(header);
	if (!file_offset.present) { invalid_elf(); }

	if (file_offset.value != elf->segments.file_offset) {
		if (!lone_elf_header_write_segments_offset(header, elf->segments.file_offset)) { invalid_elf(); }
		if (!lone_elf_header_write_segment_count(header, elf->segments.table.segment.count)) { invalid_elf(); }
		patch_elf_header(elf);
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

#include <lone/architecture/linux/entry.c>
