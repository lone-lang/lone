/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_ELF_HEADER
#define LONE_ELF_HEADER

#include <linux/elf.h>

#include <lone/definitions.h>
#include <lone/types.h>

/* Latest ELF specification as of this writing:
 * https://www.sco.com/developers/gabi/latest/contents.html
 */

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The ELF identification data, or ident for short,                    │
   │    is a byte array containing the metadata needed for                  │
   │    handling the ELF correctly. It contains the ELF's                   │
   │    magic numbers as well as its class, data encoding                   │
   │    and targeted binary interfaces. The class refers                    │
   │    to the word size of the target architecture.                        │
   │    The data encoding refers to its endianness.                         │
   │                                                                        │
   │    The ELF data structures were designed to use                        │
   │    the natural types and byte order of the target                      │
   │    allowing programs to read/write the data directly.                  │
   │    Since bytes do not depend on endianness, an array                   │
   │    of bytes serves as the ELF's metadata, making it                    │
   │    easy to work with.                                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_elf_ident_index {
	LONE_ELF_IDENT_INDEX_MAGIC_0        = 0,
	LONE_ELF_IDENT_INDEX_MAGIC_1        = 1,
	LONE_ELF_IDENT_INDEX_MAGIC_2        = 2,
	LONE_ELF_IDENT_INDEX_MAGIC_3        = 3,
	LONE_ELF_IDENT_INDEX_CLASS          = 4,
	LONE_ELF_IDENT_INDEX_DATA_ENCODING  = 5,
	LONE_ELF_IDENT_INDEX_VERSION        = 6,
	LONE_ELF_IDENT_INDEX_OS_ABI         = 7,
	LONE_ELF_IDENT_INDEX_OS_ABI_VERSION = 8,

	LONE_ELF_IDENT_INDEX_PADDING_0      = 9,
	LONE_ELF_IDENT_INDEX_PADDING_1      = 10,
	LONE_ELF_IDENT_INDEX_PADDING_2      = 11,
	LONE_ELF_IDENT_INDEX_PADDING_3      = 12,
	LONE_ELF_IDENT_INDEX_PADDING_4      = 13,
	LONE_ELF_IDENT_INDEX_PADDING_5      = 14,
	LONE_ELF_IDENT_INDEX_PADDING_6      = 15,

	LONE_ELF_IDENT_INDEX_DATA           = LONE_ELF_IDENT_INDEX_MAGIC_0,
	LONE_ELF_IDENT_INDEX_PADDING        = LONE_ELF_IDENT_INDEX_PADDING_0,
	LONE_ELF_IDENT_INDEX_MAGIC          = LONE_ELF_IDENT_INDEX_MAGIC_0,
};

enum lone_elf_ident_magic_number {
	LONE_ELF_IDENT_MAGIC_0 = 0x7f,
	LONE_ELF_IDENT_MAGIC_1 = 'E',
	LONE_ELF_IDENT_MAGIC_2 = 'L',
	LONE_ELF_IDENT_MAGIC_3 = 'F',
};

enum lone_elf_ident_class {
	LONE_ELF_IDENT_CLASS_INVALID = 0,
	LONE_ELF_IDENT_CLASS_32BIT   = 1,
	LONE_ELF_IDENT_CLASS_64BIT   = 2,
};

enum lone_elf_ident_data_encoding {
	LONE_ELF_IDENT_DATA_ENCODING_INVALID = 0,
	LONE_ELF_IDENT_DATA_ENCODING_2_LSB   = 1, /* two's complement, least significant byte lowest */
	LONE_ELF_IDENT_DATA_ENCODING_2_MSB   = 2, /* two's complement, most significant byte lowest */

	LONE_ELF_IDENT_DATA_ENCODING_LITTLE_ENDIAN = LONE_ELF_IDENT_DATA_ENCODING_2_LSB,
	LONE_ELF_IDENT_DATA_ENCODING_BIG_ENDIAN    = LONE_ELF_IDENT_DATA_ENCODING_2_MSB,
};

enum lone_elf_ident_version {
	LONE_ELF_IDENT_VERSION_INVALID = 0,
	LONE_ELF_IDENT_VERSION_CURRENT = 1,
};

enum lone_elf_ident_os_abi {
	LONE_ELF_IDENT_OS_ABI_UNSPECIFIED = 0,  /* No extensions or unspecified */
	LONE_ELF_IDENT_OS_ABI_HPUX        = 1,  /* Hewlett-Packard HP-UX */
	LONE_ELF_IDENT_OS_ABI_NETBSD      = 2,  /* NetBSD */
	LONE_ELF_IDENT_OS_ABI_GNU         = 3,  /* GNU */
	LONE_ELF_IDENT_OS_ABI_SOLARIS     = 6,  /* Sun Solaris */
	LONE_ELF_IDENT_OS_ABI_AIX         = 7,  /* AIX */
	LONE_ELF_IDENT_OS_ABI_IRIX        = 8,  /* IRIX */
	LONE_ELF_IDENT_OS_ABI_FREEBSD     = 9,  /* FreeBSD */
	LONE_ELF_IDENT_OS_ABI_TRU64       = 10, /* Compaq TRU64 UNIX */
	LONE_ELF_IDENT_OS_ABI_MODESTO     = 11, /* Novell Modesto */
	LONE_ELF_IDENT_OS_ABI_OPENBSD     = 12, /* Open BSD */
	LONE_ELF_IDENT_OS_ABI_OPENVMS     = 13, /* Open VMS */
	LONE_ELF_IDENT_OS_ABI_NSK         = 14, /* Hewlett-Packard Non-Stop Kernel */
	LONE_ELF_IDENT_OS_ABI_AROS        = 15, /* Amiga Research OS */
	LONE_ELF_IDENT_OS_ABI_FENIXOS     = 16, /* The FenixOS highly scalable multi-core OS */
	LONE_ELF_IDENT_OS_ABI_CLOUDABI    = 17, /* Nuxi CloudABI */
	LONE_ELF_IDENT_OS_ABI_OPENVOS     = 18, /* Stratus Technologies OpenVOS */

	LONE_ELF_IDENT_OS_ABI_LINUX       = LONE_ELF_IDENT_OS_ABI_GNU, /* historical - alias for GNU */
};

enum lone_elf_ident_os_abi_version {
	LONE_ELF_IDENT_OS_ABI_VERSION_UNSPECIFIED = 0,
};

#endif /* LONE_ELF_HEADER */
