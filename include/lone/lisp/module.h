/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULE_HEADER
#define LONE_LISP_MODULE_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭───────────────────────────┨ LONE / MODULES ┠───────────────────────────╮
   │                                                                        │
   │    Module importing, exporting and loading operations.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_module_null(struct lone_lisp *lone);
struct lone_lisp_value lone_lisp_module_for_name(struct lone_lisp *lone, struct lone_lisp_value name);
struct lone_lisp_value lone_lisp_module_load(struct lone_lisp *lone, struct lone_lisp_value name);
void lone_lisp_module_load_from_bytes(struct lone_lisp *lone, struct lone_lisp_value module, struct lone_bytes bytes);
void lone_lisp_module_load_null_from_file_descriptor(struct lone_lisp *lone, int file_descriptor);
void lone_lisp_module_load_null_from_standard_input(struct lone_lisp *lone);
void lone_lisp_module_path_push(struct lone_lisp *lone, struct lone_lisp_value directory);
void lone_lisp_module_path_push_c_string(struct lone_lisp *lone, char *directory);
void lone_lisp_module_path_push_va_list(struct lone_lisp *lone, size_t count, va_list directories);
void lone_lisp_module_path_push_all(struct lone_lisp *lone, size_t count, ...);

void lone_lisp_module_export(
	struct lone_lisp *lone,
	struct lone_lisp_value module,
	struct lone_lisp_value symbol
);

void lone_lisp_module_set_and_export(
	struct lone_lisp *lone,
	struct lone_lisp_value module,
	struct lone_lisp_value symbol,
	struct lone_lisp_value value
);

void lone_lisp_module_set_and_export_c_string(struct lone_lisp *lone,
		struct lone_lisp_value module, char *symbol, struct lone_lisp_value value);

LONE_LISP_PRIMITIVE(module_import);
LONE_LISP_PRIMITIVE(module_export);

#endif /* LONE_LISP_MODULE_HEADER */
