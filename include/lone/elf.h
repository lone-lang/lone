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

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The ELF type folds multiple distinct data types                     │
   │    into one unsigned 16 bit integer value:                             │
   │                                                                        │
   │        ◦ General object file type                                      │
   │        ◦ Operating system-specific object file type                    │
   │        ◦ Processor-specific object file type                           │
   │                                                                        │
   │    Values within [0x0000, 0xFDFF] are general object file types.       │
   │    Values within [0x0000, 0x0004] are defined by the specification.    │
   │    Values within [0x0005, 0xFDFF] are reserved for future use.         │
   │    Values within [0xFE00, 0xFEFF] are OS-specific file types.          │
   │    Values within [0xFF00, 0xFFFF] are processor-specific file types.   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_elf_type {
	LONE_ELF_TYPE_NONE        = 0,
	LONE_ELF_TYPE_RELOCATABLE = 1,
	LONE_ELF_TYPE_EXECUTABLE  = 2,
	LONE_ELF_TYPE_DYNAMIC     = 3, /* Shared object file */
	LONE_ELF_TYPE_CORE        = 4,

	LONE_ELF_TYPE_SHARED_OBJECT = LONE_ELF_TYPE_DYNAMIC,
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The ELF specification allocates from the unsigned 16 bit integer    │
   │    range numbers for a large amount of machine types. The allocated    │
   │    segments are not contiguous, there are gaps with reserved values.   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_elf_machine {
	LONE_ELF_MACHINE_NONE        = 0,  /* No machine requirements */
	LONE_ELF_MACHINE_M32         = 1,  /* AT&T WE 32100 */
	LONE_ELF_MACHINE_SPARC       = 2,  /* SPARC */
	LONE_ELF_MACHINE_386         = 3,  /* Intel 80386 */
	LONE_ELF_MACHINE_68K         = 4,  /* Motorola 68000 */
	LONE_ELF_MACHINE_88K         = 5,  /* Motorola 88000 */
	LONE_ELF_MACHINE_IAMCU       = 6,  /* Intel MCU */
	LONE_ELF_MACHINE_860         = 7,  /* Intel 80860 */
	LONE_ELF_MACHINE_MIPS        = 8,  /* MIPS I Architecture */
	LONE_ELF_MACHINE_S370        = 9,  /* IBM System/370 Processor */
	LONE_ELF_MACHINE_MIPS_RS3_LE = 10, /* MIPS RS3000 Little-endian */

	LONE_ELF_MACHINE_PARISC = 15, /* Hewlett-Packard PA-RISC */

	LONE_ELF_MACHINE_VPP500      = 17, /* Fujitsu VPP500 */
	LONE_ELF_MACHINE_SPARC32PLUS = 18, /* Enhanced instruction set SPARC */
	LONE_ELF_MACHINE_960         = 19, /* Intel 80960 */
	LONE_ELF_MACHINE_PPC         = 20, /* PowerPC */
	LONE_ELF_MACHINE_PPC64       = 21, /* 64-bit PowerPC */
	LONE_ELF_MACHINE_S390        = 22, /* IBM System/390 Processor */
	LONE_ELF_MACHINE_SPU         = 23, /* IBM SPU/SPC */

	LONE_ELF_MACHINE_V800         = 36,  /* NEC V800 */
	LONE_ELF_MACHINE_FR20         = 37,  /* Fujitsu FR20 */
	LONE_ELF_MACHINE_RH32         = 38,  /* TRW RH-32 */
	LONE_ELF_MACHINE_RCE          = 39,  /* Motorola RCE */
	LONE_ELF_MACHINE_ARM          = 40,  /* ARM 32-bit architecture (AARCH32) */
	LONE_ELF_MACHINE_ALPHA        = 41,  /* Digital Alpha */
	LONE_ELF_MACHINE_SH           = 42,  /* Hitachi SH */
	LONE_ELF_MACHINE_SPARCV9      = 43,  /* SPARC Version 9 */
	LONE_ELF_MACHINE_TRICORE      = 44,  /* Siemens TriCore embedded processor */
	LONE_ELF_MACHINE_ARC          = 45,  /* Argonaut RISC Core, Argonaut Technologies Inc. */
	LONE_ELF_MACHINE_H8_300       = 46,  /* Hitachi H8/300 */
	LONE_ELF_MACHINE_H8_300H      = 47,  /* Hitachi H8/300H */
	LONE_ELF_MACHINE_H8S          = 48,  /* Hitachi H8S */
	LONE_ELF_MACHINE_H8_500       = 49,  /* Hitachi H8/500 */
	LONE_ELF_MACHINE_IA_64        = 50,  /* Intel IA-64 processor architecture */
	LONE_ELF_MACHINE_MIPS_X       = 51,  /* Stanford MIPS-X */
	LONE_ELF_MACHINE_COLDFIRE     = 52,  /* Motorola ColdFire */
	LONE_ELF_MACHINE_68HC12       = 53,  /* Motorola M68HC12 */
	LONE_ELF_MACHINE_MMA          = 54,  /* Fujitsu MMA Multimedia Accelerator */
	LONE_ELF_MACHINE_PCP          = 55,  /* Siemens PCP */
	LONE_ELF_MACHINE_NCPU         = 56,  /* Sony nCPU embedded RISC processor */
	LONE_ELF_MACHINE_NDR1         = 57,  /* Denso NDR1 microprocessor */
	LONE_ELF_MACHINE_STARCORE     = 58,  /* Motorola Star*Core processor  */
	LONE_ELF_MACHINE_ME16         = 59,  /* Toyota ME16 processor */
	LONE_ELF_MACHINE_ST100        = 60,  /* STMicroelectronics ST100 processor */
	LONE_ELF_MACHINE_TINYJ        = 61,  /* Advanced Logic Corp. TinyJ embedded processor family */
	LONE_ELF_MACHINE_X86_64       = 62,  /* AMD x86-64 architecture */
	LONE_ELF_MACHINE_PDSP         = 63,  /* Sony DSP Processor */
	LONE_ELF_MACHINE_PDP10        = 64,  /* Digital Equipment Corp. PDP-10 */
	LONE_ELF_MACHINE_PDP11        = 65,  /* Digital Equipment Corp. PDP-11 */
	LONE_ELF_MACHINE_FX66         = 66,  /* Siemens FX66 microcontroller */
	LONE_ELF_MACHINE_ST9PLUS      = 67,  /* STMicroelectronics ST9+ 8/16 bit microcontroller */
	LONE_ELF_MACHINE_ST7          = 68,  /* STMicroelectronics ST7 8-bit microcontroller */
	LONE_ELF_MACHINE_68HC16       = 69,  /* Motorola MC68HC16 Microcontroller */
	LONE_ELF_MACHINE_68HC11       = 70,  /* Motorola MC68HC11 Microcontroller */
	LONE_ELF_MACHINE_68HC08       = 71,  /* Motorola MC68HC08 Microcontroller */
	LONE_ELF_MACHINE_68HC05       = 72,  /* Motorola MC68HC05 Microcontroller */
	LONE_ELF_MACHINE_SVX          = 73,  /* Silicon Graphics SVx */
	LONE_ELF_MACHINE_ST19         = 74,  /* STMicroelectronics ST19 8-bit microcontroller */
	LONE_ELF_MACHINE_VAX          = 75,  /* Digital VAX */
	LONE_ELF_MACHINE_CRIS         = 76,  /* Axis Communications 32-bit embedded processor */
	LONE_ELF_MACHINE_JAVELIN      = 77,  /* Infineon Technologies 32-bit embedded processor */
	LONE_ELF_MACHINE_FIREPATH     = 78,  /* Element 14 64-bit DSP Processor */
	LONE_ELF_MACHINE_ZSP          = 79,  /* LSI Logic 16-bit DSP Processor */
	LONE_ELF_MACHINE_MMIX         = 80,  /* Donald Knuth's educational 64-bit processor */
	LONE_ELF_MACHINE_HUANY        = 81,  /* Harvard University machine-independent object files */
	LONE_ELF_MACHINE_PRISM        = 82,  /* SiTera Prism */
	LONE_ELF_MACHINE_AVR          = 83,  /* Atmel AVR 8-bit microcontroller */
	LONE_ELF_MACHINE_FR30         = 84,  /* Fujitsu FR30 */
	LONE_ELF_MACHINE_D10V         = 85,  /* Mitsubishi D10V */
	LONE_ELF_MACHINE_D30V         = 86,  /* Mitsubishi D30V */
	LONE_ELF_MACHINE_V850         = 87,  /* NEC v850 */
	LONE_ELF_MACHINE_M32R         = 88,  /* Mitsubishi M32R */
	LONE_ELF_MACHINE_MN10300      = 89,  /* Matsushita MN10300 */
	LONE_ELF_MACHINE_MN10200      = 90,  /* Matsushita MN10200 */
	LONE_ELF_MACHINE_PJ           = 91,  /* picoJava */
	LONE_ELF_MACHINE_OPENRISC     = 92,  /* OpenRISC 32-bit embedded processor */
	LONE_ELF_MACHINE_ARC_COMPACT  = 93,  /* ARC International ARCompact processor (old spelling/synonym: EM_ARC_A5) */
	LONE_ELF_MACHINE_XTENSA       = 94,  /* Tensilica Xtensa Architecture */
	LONE_ELF_MACHINE_VIDEOCORE    = 95,  /* Alphamosaic VideoCore processor */
	LONE_ELF_MACHINE_TMM_GPP      = 96,  /* Thompson Multimedia General Purpose Processor */
	LONE_ELF_MACHINE_NS32K        = 97,  /* National Semiconductor 32000 series */
	LONE_ELF_MACHINE_TPC          = 98,  /* Tenor Network TPC processor */
	LONE_ELF_MACHINE_SNP1K        = 99,  /* Trebia SNP 1000 processor */
	LONE_ELF_MACHINE_ST200        = 100, /* STMicroelectronics ST200 microcontroller */
	LONE_ELF_MACHINE_IP2K         = 101, /* Ubicom IP2xxx microcontroller family */
	LONE_ELF_MACHINE_MAX          = 102, /* MAX Processor */
	LONE_ELF_MACHINE_CR           = 103, /* National Semiconductor CompactRISC microprocessor */
	LONE_ELF_MACHINE_F2MC16       = 104, /* Fujitsu F2MC16 */
	LONE_ELF_MACHINE_MSP430       = 105, /* Texas Instruments embedded microcontroller msp430 */
	LONE_ELF_MACHINE_BLACKFIN     = 106, /* Analog Devices Blackfin (DSP) processor */
	LONE_ELF_MACHINE_SE_C33       = 107, /* S1C33 Family of Seiko Epson processors */
	LONE_ELF_MACHINE_SEP          = 108, /* Sharp embedded microprocessor */
	LONE_ELF_MACHINE_ARCA         = 109, /* Arca RISC Microprocessor */
	LONE_ELF_MACHINE_UNICORE      = 110, /* Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University */
	LONE_ELF_MACHINE_EXCESS       = 111, /* eXcess: 16/32/64-bit configurable embedded CPU */
	LONE_ELF_MACHINE_DXP          = 112, /* Icera Semiconductor Inc. Deep Execution Processor */
	LONE_ELF_MACHINE_ALTERA_NIOS2 = 113, /* Altera Nios II soft-core processor */
	LONE_ELF_MACHINE_CRX          = 114, /* National Semiconductor CompactRISC CRX microprocessor */
	LONE_ELF_MACHINE_XGATE        = 115, /* Motorola XGATE embedded processor */
	LONE_ELF_MACHINE_C166         = 116, /* Infineon C16x/XC16x processor */
	LONE_ELF_MACHINE_M16C         = 117, /* Renesas M16C series microprocessors */
	LONE_ELF_MACHINE_DSPIC30F     = 118, /* Microchip Technology dsPIC30F Digital Signal Controller */
	LONE_ELF_MACHINE_CE           = 119, /* Freescale Communication Engine RISC core */
	LONE_ELF_MACHINE_M32C         = 120, /* Renesas M32C series microprocessors */

	LONE_ELF_MACHINE_TSK3000       = 131, /* Altium TSK3000 core */
	LONE_ELF_MACHINE_RS08          = 132, /* Freescale RS08 embedded processor */
	LONE_ELF_MACHINE_SHARC         = 133, /* Analog Devices SHARC family of 32-bit DSP processors */
	LONE_ELF_MACHINE_ECOG2         = 134, /* Cyan Technology eCOG2 microprocessor */
	LONE_ELF_MACHINE_SCORE7        = 135, /* Sunplus S+core7 RISC processor */
	LONE_ELF_MACHINE_DSP24         = 136, /* New Japan Radio (NJR) 24-bit DSP Processor */
	LONE_ELF_MACHINE_VIDEOCORE3    = 137, /* Broadcom VideoCore III processor */
	LONE_ELF_MACHINE_LATTICEMICO32 = 138, /* RISC processor for Lattice FPGA architecture */
	LONE_ELF_MACHINE_SE_C17        = 139, /* Seiko Epson C17 family */
	LONE_ELF_MACHINE_TI_C6000      = 140, /* The Texas Instruments TMS320C6000 DSP family */
	LONE_ELF_MACHINE_TI_C2000      = 141, /* The Texas Instruments TMS320C2000 DSP family */
	LONE_ELF_MACHINE_TI_C5500      = 142, /* The Texas Instruments TMS320C55x DSP family */
	LONE_ELF_MACHINE_TI_ARP32      = 143, /* Texas Instruments Application Specific RISC Processor, 32bit fetch */
	LONE_ELF_MACHINE_TI_PRU        = 144, /* Texas Instruments Programmable Realtime Unit */

	LONE_ELF_MACHINE_MMDSP_PLUS  = 160, /* STMicroelectronics 64bit VLIW Data Signal Processor */
	LONE_ELF_MACHINE_CYPRESS_M8C = 161, /* Cypress M8C microprocessor */
	LONE_ELF_MACHINE_R32C        = 162, /* Renesas R32C series microprocessors */
	LONE_ELF_MACHINE_TRIMEDIA    = 163, /* NXP Semiconductors TriMedia architecture family */
	LONE_ELF_MACHINE_QDSP6       = 164, /* QUALCOMM DSP6 Processor */
	LONE_ELF_MACHINE_8051        = 165, /* Intel 8051 and variants */
	LONE_ELF_MACHINE_STXP7X      = 166, /* STMicroelectronics STxP7x family of configurable and extensible RISC processors */
	LONE_ELF_MACHINE_NDS32       = 167, /* Andes Technology compact code size embedded RISC processor family */
	LONE_ELF_MACHINE_ECOG1c      = 168, /* Cyan Technology eCOG1X family */
	LONE_ELF_MACHINE_ECOG1X      = 168, /* Cyan Technology eCOG1X family */
	LONE_ELF_MACHINE_MAXQ30      = 169, /* Dallas Semiconductor MAXQ30 Core Micro-controllers */
	LONE_ELF_MACHINE_XIMO16      = 170, /* New Japan Radio (NJR) 16-bit DSP Processor */
	LONE_ELF_MACHINE_MANIK       = 171, /* M2000 Reconfigurable RISC Microprocessor */
	LONE_ELF_MACHINE_CRAYNV2     = 172, /* Cray Inc. NV2 vector architecture */
	LONE_ELF_MACHINE_RX          = 173, /* Renesas RX family */
	LONE_ELF_MACHINE_METAG       = 174, /* Imagination Technologies META processor architecture */
	LONE_ELF_MACHINE_MCST_ELBRUS = 175, /* MCST Elbrus general purpose hardware architecture */
	LONE_ELF_MACHINE_ECOG16      = 176, /* Cyan Technology eCOG16 family */
	LONE_ELF_MACHINE_CR16        = 177, /* National Semiconductor CompactRISC CR16 16-bit microprocessor */
	LONE_ELF_MACHINE_ETPU        = 178, /* Freescale Extended Time Processing Unit */
	LONE_ELF_MACHINE_SLE9X       = 179, /* Infineon Technologies SLE9X core */
	LONE_ELF_MACHINE_L10M        = 180, /* Intel L10M */
	LONE_ELF_MACHINE_K10M        = 181, /* Intel K10M */

	/* 182 - Reserved for future Intel use */

	LONE_ELF_MACHINE_AARCH64 = 183, /* ARM 64-bit architecture (AARCH64) */

	/* 184 - Reserved for future ARM use */

	LONE_ELF_MACHINE_AVR32        = 185, /* Atmel Corporation 32-bit microprocessor family */
	LONE_ELF_MACHINE_STM8         = 186, /* STMicroeletronics STM8 8-bit microcontroller */
	LONE_ELF_MACHINE_TILE64       = 187, /* Tilera TILE64 multicore architecture family */
	LONE_ELF_MACHINE_TILEPRO      = 188, /* Tilera TILEPro multicore architecture family */
	LONE_ELF_MACHINE_MICROBLAZE   = 189, /* Xilinx MicroBlaze 32-bit RISC soft processor core */
	LONE_ELF_MACHINE_CUDA         = 190, /* NVIDIA CUDA architecture */
	LONE_ELF_MACHINE_TILEGX       = 191, /* Tilera TILE-Gx multicore architecture family */
	LONE_ELF_MACHINE_CLOUDSHIELD  = 192, /* CloudShield architecture family */
	LONE_ELF_MACHINE_COREA_1ST    = 193, /* KIPO-KAIST Core-A 1st generation processor family */
	LONE_ELF_MACHINE_COREA_2ND    = 194, /* KIPO-KAIST Core-A 2nd generation processor family */
	LONE_ELF_MACHINE_ARC_COMPACT2 = 195, /* Synopsys ARCompact V2 */
	LONE_ELF_MACHINE_OPEN8        = 196, /* Open8 8-bit RISC soft processor core */
	LONE_ELF_MACHINE_RL78         = 197, /* Renesas RL78 family */
	LONE_ELF_MACHINE_VIDEOCORE5   = 198, /* Broadcom VideoCore V processor */
	LONE_ELF_MACHINE_78KOR        = 199, /* Renesas 78KOR family */
	LONE_ELF_MACHINE_56800EX      = 200, /* Freescale 56800EX Digital Signal Controller (DSC) */
	LONE_ELF_MACHINE_BA1          = 201, /* Beyond BA1 CPU architecture */
	LONE_ELF_MACHINE_BA2          = 202, /* Beyond BA2 CPU architecture */
	LONE_ELF_MACHINE_XCORE        = 203, /* XMOS xCORE processor family */
	LONE_ELF_MACHINE_MCHP_PIC     = 204, /* Microchip 8-bit PIC(r) family */
	LONE_ELF_MACHINE_INTEL205     = 205, /* Reserved by Intel */
	LONE_ELF_MACHINE_INTEL206     = 206, /* Reserved by Intel */
	LONE_ELF_MACHINE_INTEL207     = 207, /* Reserved by Intel */
	LONE_ELF_MACHINE_INTEL208     = 208, /* Reserved by Intel */
	LONE_ELF_MACHINE_INTEL209     = 209, /* Reserved by Intel */
	LONE_ELF_MACHINE_KM32         = 210, /* KM211 KM32 32-bit processor */
	LONE_ELF_MACHINE_KMX32        = 211, /* KM211 KMX32 32-bit processor */
	LONE_ELF_MACHINE_KMX16        = 212, /* KM211 KMX16 16-bit processor */
	LONE_ELF_MACHINE_KMX8         = 213, /* KM211 KMX8 8-bit processor */
	LONE_ELF_MACHINE_KVARC        = 214, /* KM211 KVARC processor */
	LONE_ELF_MACHINE_CDP          = 215, /* Paneve CDP architecture family */
	LONE_ELF_MACHINE_COGE         = 216, /* Cognitive Smart Memory Processor */
	LONE_ELF_MACHINE_COOL         = 217, /* Bluechip Systems CoolEngine */
	LONE_ELF_MACHINE_NORC         = 218, /* Nanoradio Optimized RISC */
	LONE_ELF_MACHINE_CSR_KALIMBA  = 219, /* CSR Kalimba architecture family */
	LONE_ELF_MACHINE_Z80          = 220, /* Zilog Z80 */
	LONE_ELF_MACHINE_VISIUM       = 221, /* Controls and Data Services VISIUMcore processor */
	LONE_ELF_MACHINE_FT32         = 222, /* FTDI Chip FT32 high performance 32-bit RISC architecture */
	LONE_ELF_MACHINE_MOXIE        = 223, /* Moxie processor family */
	LONE_ELF_MACHINE_AMDGPU       = 224, /* AMD GPU architecture */

	LONE_ELF_MACHINE_RISCV = 243, /* RISC-V */
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    As of this writing, there is only one version of the ELF format.    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_elf_version {
	LONE_ELF_VERSION_INVALID = LONE_ELF_IDENT_VERSION_INVALID,
	LONE_ELF_VERSION_CURRENT = LONE_ELF_IDENT_VERSION_CURRENT,
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Useful ranges and sizes of various ELF structures and types.        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_elf_ranges_ident_index {
	LONE_ELF_RANGES_IDENT_INDEX_DATA_MIN = LONE_ELF_IDENT_INDEX_MAGIC_0,
	LONE_ELF_RANGES_IDENT_INDEX_DATA_MAX = LONE_ELF_IDENT_INDEX_OS_ABI_VERSION,

	LONE_ELF_RANGES_IDENT_INDEX_PADDING_MIN = LONE_ELF_IDENT_INDEX_PADDING_0,
	LONE_ELF_RANGES_IDENT_INDEX_PADDING_MAX = LONE_ELF_IDENT_INDEX_PADDING_6,

	LONE_ELF_RANGES_IDENT_INDEX_MIN = LONE_ELF_RANGES_IDENT_INDEX_DATA_MIN,
	LONE_ELF_RANGES_IDENT_INDEX_MAX = LONE_ELF_RANGES_IDENT_INDEX_PADDING_MAX,

	LONE_ELF_RANGES_IDENT_INDEX_MAGIC_MIN = LONE_ELF_IDENT_INDEX_MAGIC_0,
	LONE_ELF_RANGES_IDENT_INDEX_MAGIC_MAX = LONE_ELF_IDENT_INDEX_MAGIC_3,
};

enum lone_elf_sizes_ident {
	LONE_ELF_SIZES_IDENT         = LONE_ELF_RANGES_IDENT_INDEX_MAX + 1,
	LONE_ELF_SIZES_IDENT_DATA    = LONE_ELF_RANGES_IDENT_INDEX_DATA_MAX + 1,
	LONE_ELF_SIZES_IDENT_PADDING = LONE_ELF_SIZES_IDENT - LONE_ELF_SIZES_IDENT_DATA,
	LONE_ELF_SIZES_IDENT_MAGIC   = LONE_ELF_RANGES_IDENT_INDEX_MAGIC_MAX + 1,
};

enum lone_elf_ranges_ident_class {
	LONE_ELF_RANGES_IDENT_CLASS_MIN = LONE_ELF_IDENT_CLASS_32BIT,
	LONE_ELF_RANGES_IDENT_CLASS_MAX = LONE_ELF_IDENT_CLASS_64BIT,
};

enum lone_elf_ranges_ident_data_encoding {
	LONE_ELF_RANGES_IDENT_DATA_ENCODING_MIN = LONE_ELF_IDENT_DATA_ENCODING_2_LSB,
	LONE_ELF_RANGES_IDENT_DATA_ENCODING_MAX = LONE_ELF_IDENT_DATA_ENCODING_2_MSB,
};

enum lone_elf_ranges_ident_version {
	LONE_ELF_RANGES_IDENT_VERSION_MIN = LONE_ELF_IDENT_VERSION_CURRENT,
	LONE_ELF_RANGES_IDENT_VERSION_MAX = LONE_ELF_IDENT_VERSION_CURRENT,
};

enum lone_elf_ranges_ident_os_abi {
	LONE_ELF_RANGES_IDENT_OS_ABI_MIN = LONE_ELF_IDENT_OS_ABI_UNSPECIFIED,
	LONE_ELF_RANGES_IDENT_OS_ABI_MAX = LONE_ELF_IDENT_OS_ABI_OPENVOS,
};

enum lone_elf_ranges_ident_os_abi_arch {
	LONE_ELF_RANGES_IDENT_OS_ABI_ARCH_MIN = 64,
	LONE_ELF_RANGES_IDENT_OS_ABI_ARCH_MAX = 255,
};

enum lone_elf_ranges_ident_os_abi_version {
	LONE_ELF_RANGES_IDENT_OS_ABI_VERSION_MIN = LONE_ELF_IDENT_OS_ABI_VERSION_UNSPECIFIED,
	LONE_ELF_RANGES_IDENT_OS_ABI_VERSION_MAX = 255,
};

enum lone_elf_ranges_type {
	LONE_ELF_RANGES_TYPE_GENERAL_MIN = LONE_ELF_TYPE_NONE,
	LONE_ELF_RANGES_TYPE_GENERAL_MAX = LONE_ELF_TYPE_CORE,

	LONE_ELF_RANGES_TYPE_OS_MIN = 0xfe00,
	LONE_ELF_RANGES_TYPE_OS_MAX = 0xfeff,

	LONE_ELF_RANGES_TYPE_PROC_MIN = 0xff00,
	LONE_ELF_RANGES_TYPE_PROC_MAX = 0xffff,
};

enum lone_elf_ranges_machine {
	LONE_ELF_RANGES_MACHINE_MIN = LONE_ELF_MACHINE_NONE,
	LONE_ELF_RANGES_MACHINE_MAX = LONE_ELF_MACHINE_RISCV,
};

enum lone_elf_ranges_version {
	LONE_ELF_RANGES_VERSION_MIN = LONE_ELF_VERSION_CURRENT,
	LONE_ELF_RANGES_VERSION_MAX = LONE_ELF_VERSION_CURRENT,
};

#define LONE_ELF_IDENT_MAGIC_INIT(...)                                                             \
	{                                                                                          \
		LONE_ELF_IDENT_MAGIC_0,                                                            \
		LONE_ELF_IDENT_MAGIC_1,                                                            \
		LONE_ELF_IDENT_MAGIC_2,                                                            \
		LONE_ELF_IDENT_MAGIC_3,                                                            \
		__VA_ARGS__                                                                        \
	}

#define LONE_ELF_IDENT_MAGIC()                                                                     \
	((unsigned char[LONE_ELF_SIZES_IDENT_MAGIC]) LONE_ELF_IDENT_MAGIC_INIT())

#define LONE_ELF_IDENT_MAGIC_C_STRING()                                                            \
	((char[LONE_ELF_SIZES_IDENT_MAGIC]) LONE_ELF_IDENT_MAGIC_INIT('\0'))

#define LONE_ELF_IDENT_MAGIC_BYTES()                                                               \
	((struct lone_bytes) {                                                                     \
		.count = LONE_ELF_SIZES_IDENT_MAGIC,                                               \
		.pointer = LONE_ELF_IDENT_MAGIC(),                                                 \
	})

struct lone_elf_header {
	unsigned char ident[LONE_ELF_SIZES_IDENT];
	lone_u16 type;
	lone_u16 machine;
	lone_u32 version;

	union {
		struct {
			lone_u32 entry_point;

			lone_u32 segments_offset;
			lone_u32 sections_offset;

			lone_u32 flags;

			lone_u16 header_size;

			lone_u16 segment_size;
			lone_u16 segment_count;

			lone_u16 section_size;
			lone_u16 section_count;
			lone_u16 section_names_index;
		} elf32;

		struct {
			lone_u64 entry_point;

			lone_u64 segments_offset;
			lone_u64 sections_offset;

			lone_u32 flags;

			lone_u16 header_size;

			lone_u16 segment_size;
			lone_u16 segment_count;

			lone_u16 section_size;
			lone_u16 section_count;
			lone_u16 section_names_index;
		} elf64;
	} as;
};

struct lone_elf_value {
	enum lone_elf_ident_class class;
	union {
		lone_u32 u32;
		lone_u64 u64;
	} as;
};

typedef lone_s64 lone_elf_smax;
typedef lone_u64 lone_elf_umax;

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The ELF identification information is a sixteen byte array.         │
   │    Values are independent of ELF class and endianness                  │
   │    and can be indexed and accessed directly.                           │
   │    Functions are provided to access logical sections of the array.     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_bytes lone_elf_header_read_ident_data(struct lone_elf_header *header);
struct lone_bytes lone_elf_header_read_ident_padding(struct lone_elf_header *header);
struct lone_bytes lone_elf_header_read_ident_magic(struct lone_elf_header *header);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The ELF data types have the same sizes and byte order               │
   │    as the platform the program is meant to run on.                     │
   │    This makes it convenient to load and link the ELF.                  │
   │    However, it also means extra logic is required                      │
   │    to process arbitrary ELF files.                                     │
   │                                                                        │
   │    These functions implement ELF class and endianness independent      │
   │    access to the values contained in the ELF data structures.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_optional_u16 lone_elf_header_read_type(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_machine(struct lone_elf_header *header);
struct lone_optional_u32 lone_elf_header_read_version(struct lone_elf_header *header);

struct lone_elf_value lone_elf_header_read_entry_point(struct lone_elf_header *header);
struct lone_elf_value lone_elf_header_read_segments_offset(struct lone_elf_header *header);
struct lone_elf_value lone_elf_header_read_sections_offset(struct lone_elf_header *header);

struct lone_optional_u32 lone_elf_header_read_flags(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_header_size(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_segment_size(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_segment_count(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_section_size(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_section_count(struct lone_elf_header *header);
struct lone_optional_u16 lone_elf_header_read_section_names_index(struct lone_elf_header *header);

bool lone_elf_header_ident_has_valid_magic_numbers(struct lone_elf_header *header);
bool lone_elf_header_ident_has_valid_class(struct lone_elf_header *header);
bool lone_elf_header_ident_has_valid_data_encoding(struct lone_elf_header *header);
bool lone_elf_header_ident_has_valid_version(struct lone_elf_header *header);
bool lone_elf_header_ident_has_valid_os_abi(struct lone_elf_header *header);
bool lone_elf_header_ident_has_zero_filled_padding(struct lone_elf_header *header);
bool lone_elf_header_has_valid_ident(struct lone_elf_header *header);
bool lone_elf_header_has_valid_type(struct lone_elf_header *header);
bool lone_elf_header_has_valid_machine(struct lone_elf_header *header);
bool lone_elf_header_has_valid_version(struct lone_elf_header *header);
bool lone_elf_header_has_valid_header_size(struct lone_elf_header *header);

bool lone_elf_header_ident_is_linux_os_abi(struct lone_elf_header *header);

bool lone_elf_header_type_is_os(lone_u16 type);
bool lone_elf_header_type_is_proc(lone_u16 type);
bool lone_elf_header_type_is_general(lone_u16 type);
bool lone_elf_header_type_is_specific(lone_u16 type);

bool lone_elf_header_machine_is_reserved(lone_u16 machine);

#endif /* LONE_ELF_HEADER */
