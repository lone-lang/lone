#ifndef LONE_MODULES_HEADER
#define LONE_MODULES_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Module importing, exporting and loading operations.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_module_null(struct lone_lisp *lone);
struct lone_value *lone_module_for_name(struct lone_lisp *lone, struct lone_value *name);
struct lone_value *lone_module_load(struct lone_lisp *lone, struct lone_value *name);
void lone_module_load_null_from_file_descriptor(struct lone_lisp *lone, int file_descriptor);
void lone_module_load_null_from_standard_input(struct lone_lisp *lone);

void lone_export(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *symbol
);

void lone_set_and_export(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *symbol,
	struct lone_value *value
);

struct lone_value *lone_primitive_export(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *environment,
	struct lone_value *arguments,
	struct lone_value *closure
);

struct lone_value *lone_primitive_import(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *environment,
	struct lone_value *arguments,
	struct lone_value *closure
);

#endif /* LONE_MODULES_HEADER */
