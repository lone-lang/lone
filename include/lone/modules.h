/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_HEADER
#define LONE_MODULES_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭───────────────────────────┨ LONE / MODULES ┠───────────────────────────╮
   │                                                                        │
   │    Module importing, exporting and loading operations.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_module_null(struct lone_lisp *lone);
struct lone_value lone_module_for_name(struct lone_lisp *lone, struct lone_value name);
struct lone_value lone_module_load(struct lone_lisp *lone, struct lone_value name);
void lone_module_load_from_bytes(struct lone_lisp *lone, struct lone_value module, struct lone_bytes bytes);
void lone_module_load_null_from_file_descriptor(struct lone_lisp *lone, int file_descriptor);
void lone_module_load_null_from_standard_input(struct lone_lisp *lone);
void lone_module_path_push(struct lone_lisp *lone, struct lone_value directory);
void lone_module_path_push_c_string(struct lone_lisp *lone, char *directory);
void lone_module_path_push_va_list(struct lone_lisp *lone, size_t count, va_list directories);
void lone_module_path_push_all(struct lone_lisp *lone, size_t count, ...);

void lone_export(
	struct lone_lisp *lone,
	struct lone_value module,
	struct lone_value symbol
);

void lone_set_and_export(
	struct lone_lisp *lone,
	struct lone_value module,
	struct lone_value symbol,
	struct lone_value value
);

LONE_PRIMITIVE(import);
LONE_PRIMITIVE(export);

#endif /* LONE_MODULES_HEADER */
