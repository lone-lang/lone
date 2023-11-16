#ifndef LONE_STRUCT_ELF_HEADER
#define LONE_STRUCT_ELF_HEADER

#include <lone/types.h>

struct lone_elf_program_header_table {
	void *address;
	size_t entry_size;
	size_t entry_count;
};

#endif /* LONE_STRUCT_ELF_HEADER */
