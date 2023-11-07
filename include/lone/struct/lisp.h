#ifndef LONE_STRUCT_LISP_HEADER
#define LONE_STRUCT_LISP_HEADER

#include <lone/types.h>

/* ╭───────────────────────┨ LONE LISP INTERPRETER ┠────────────────────────╮
   │                                                                        │
   │    The lone lisp interpreter is composed of all internal state         │
   │    necessary to process useful programs. It includes memory,           │
   │    references to all allocated objects, a table of interned            │
   │    symbols, references to constant values such as nil and              │
   │    a table of loaded modules and the top level null module.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_memory;
struct lone_heap;

struct lone_lisp {
	struct {
		void *stack;
		struct lone_memory *general;
		struct lone_heap *heaps;
	} memory;
	struct lone_value *symbol_table;
	struct {
		struct lone_value *nil;
		struct lone_value *truth;
	} constants;
	struct {
		struct lone_value *loaded;
		struct lone_value *null;
		struct lone_value *top_level_environment;
		struct lone_value *path;
	} modules;
	struct {
		struct {
			unsigned long offset_basis;
		} fnv_1a;
	} hash;
};

#endif /* LONE_STRUCT_LISP_HEADER */
