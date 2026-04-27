/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone.h>
#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/elf.h>
#include <lone/linux.h>
#include <lone/auxiliary_vector.h>

#define EXIT_OK                       0
#define EXIT_USAGE                    1
#define EXIT_INVALID_ELF              2
#define EXIT_NO_FIRST_LOAD            3
#define EXIT_LOAD_ADDRESS_UNALIGNED   4
#define EXIT_AT_PHDR_UNMAPPED         5
#define EXIT_INCORRECT_LOAD_BIAS      6

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	struct lone_elf_header *header;
	struct lone_elf_segments segments;
	struct lone_elf_segment *segment;
	struct lone_optional_u32 type;
	struct lone_elf_optional_umax optional;
	lone_elf_umax e_phoff, vaddr, offset, memsz, load_address, kernel_at_phdr;
	size_t page_size;
	intptr_t map;
	off_t size;
	int fd;
	lone_u16 i;
	bool found_first_load, found_covering;

	if (argc < 2) { linux_exit(EXIT_USAGE); }

	fd = linux_openat(AT_FDCWD, (unsigned char *) argv[1], O_RDONLY | O_CLOEXEC);
	if (fd < 0) { linux_exit(EXIT_USAGE); }

	size = linux_lseek(fd, 0, SEEK_END);
	if (size <= 0) { linux_exit(EXIT_INVALID_ELF); }

	map = linux_mmap(0, (size_t) size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map < 0) { linux_exit(EXIT_INVALID_ELF); }
	linux_close(fd);

	header = (struct lone_elf_header *) map;
	if (!lone_elf_header_is_valid(header)) { linux_exit(EXIT_INVALID_ELF); }

	optional = lone_elf_header_read_segments_offset(header);
	if (!optional.present) { linux_exit(EXIT_INVALID_ELF); }
	e_phoff = optional.value;

	segments = lone_elf_header_read_segments(header);

	found_first_load = false;
	load_address = 0;

	for (i = 0; i < segments.segment.count; ++i) {
		segment = lone_elf_segment_at(segments, i);
		if (!segment) { linux_exit(EXIT_INVALID_ELF); }

		type = lone_elf_segment_read_type(header, segment);
		if (!type.present) { linux_exit(EXIT_INVALID_ELF); }
		if (type.value != LONE_ELF_SEGMENT_TYPE_LOADABLE) { continue; }

		optional = lone_elf_segment_read_virtual_address(header, segment);
		if (!optional.present) { linux_exit(EXIT_INVALID_ELF); }
		vaddr = optional.value;

		optional = lone_elf_segment_read_file_offset(header, segment);
		if (!optional.present) { linux_exit(EXIT_INVALID_ELF); }
		offset = optional.value;

		load_address = vaddr - offset;
		found_first_load = true;
		break;
	}

	if (!found_first_load) { linux_exit(EXIT_NO_FIRST_LOAD); }

	page_size = lone_auxiliary_vector_page_size(auxv);
	if (page_size == 0) { page_size = 4096; }
	if (load_address % page_size != 0) { linux_exit(EXIT_LOAD_ADDRESS_UNALIGNED); }

	kernel_at_phdr = load_address + e_phoff;

	found_covering = false;

	for (i = 0; i < segments.segment.count; ++i) {
		segment = lone_elf_segment_at(segments, i);
		if (!segment) { linux_exit(EXIT_INVALID_ELF); }

		type = lone_elf_segment_read_type(header, segment);
		if (!type.present) { linux_exit(EXIT_INVALID_ELF); }
		if (type.value != LONE_ELF_SEGMENT_TYPE_LOADABLE) { continue; }

		optional = lone_elf_segment_read_virtual_address(header, segment);
		if (!optional.present) { linux_exit(EXIT_INVALID_ELF); }
		vaddr = optional.value;

		optional = lone_elf_segment_read_size_in_memory(header, segment);
		if (!optional.present) { linux_exit(EXIT_INVALID_ELF); }
		memsz = optional.value;

		optional = lone_elf_segment_read_file_offset(header, segment);
		if (!optional.present) { linux_exit(EXIT_INVALID_ELF); }
		offset = optional.value;

		if (kernel_at_phdr >= vaddr && kernel_at_phdr < vaddr + memsz) {
			if (offset + (kernel_at_phdr - vaddr) != e_phoff) {
				linux_exit(EXIT_INCORRECT_LOAD_BIAS);
			}
			found_covering = true;
			break;
		}
	}

	if (!found_covering) { linux_exit(EXIT_AT_PHDR_UNMAPPED); }

	return EXIT_OK;
}

#include <lone/architecture/linux/entry.c>
