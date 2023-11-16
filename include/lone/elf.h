#ifndef LONE_ELF_HEADER
#define LONE_ELF_HEADER

#include <linux/elf.h>

#ifndef PT_LONE
//      PT_LONE   l o n e
#define PT_LONE 0x6c6f6e65
#endif

#if PT_LONE < PT_LOOS || PT_LONE > PT_HIOS
	#warning "PT_LONE outside reserved operating system specific range"
#endif

#endif /* LONE_ELF_HEADER */
