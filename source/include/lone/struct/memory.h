#ifndef LONE_STRUCT_MEMORY_SOURCE_HEADER
#define LONE_STRUCT_MEMORY_SOURCE_HEADER

#include <lone/types.h>
#include <lone/struct/value.h>

/* ╭────────────────────┨ LONE LISP MEMORY ALLOCATION ┠─────────────────────╮
   │                                                                        │
   │    Lone is designed to work without any dependencies except Linux,     │
   │    so it does not make use of even the system's C library.             │
   │    In order to bootstrap itself in such harsh conditions,              │
   │    it must be given some memory to work with.                          │
   │                                                                        │
   │    Lone manages its own memory with a block-based allocator.           │
   │    Memory blocks are allocated on a first fit basis.                   │
   │    They will be split into smaller units when allocated                │
   │    and merged together with free neighbors when deallocated.           │
   │                                                                        │
   │    Memory blocks are segments prefixed by a block descriptor           │
   │    that tracks its size, allocation status as well as pointers         │
   │    to surrounding memory blocks.                                       │
   │                                                                        │
   │    Lone employs a very simple mark-and-sweep garbage collector.        │
   │    It starts by marking all values reachable by the interpreter        │
   │    in its current state. Then it walks the list of all values,         │
   │    deallocating any unmarked object it finds as well as any            │
   │    memory they happen own based on the value's type.                   │
   │                                                                        │
   │    Like memory blocks, lone values are prefixed by a header            │
   │    structure containing metadata such as its marked state              │
   │    as well as a pointer to the next object in the list.                │
   │                                                                        │
   │    Since these headers are prefixed to pointers returned by            │
   │    allocation and value creation functions, it is simple to            │
   │    calculate their locations given a pointer to a memory block         │
   │    or lone value: simply subtract the header's size from it.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_memory {
	struct lone_memory *prev, *next;
	int free;
	size_t size;
	unsigned char pointer[];
};

struct lone_heap {
	struct lone_heap *next;
	size_t count;
	struct lone_value values[];
};

#endif /* LONE_STRUCT_MEMORY_SOURCE_HEADER */
