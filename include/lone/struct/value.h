/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STRUCT_VALUE_HEADER
#define LONE_STRUCT_VALUE_HEADER

#include <lone/types.h>

#include <lone/struct/module.h>
#include <lone/struct/function.h>
#include <lone/struct/primitive.h>
#include <lone/struct/list.h>
#include <lone/struct/vector.h>
#include <lone/struct/table.h>
#include <lone/struct/bytes.h>
#include <lone/struct/pointer.h>

struct lone_value {
	struct {
		bool live: 1;
		bool marked: 1;
		bool should_deallocate_bytes: 1;
	};

	enum lone_type type;

	union {
		struct lone_module module;
		struct lone_function function;
		struct lone_primitive primitive;
		struct lone_list list;
		struct lone_vector vector;
		struct lone_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
		struct lone_pointer pointer;
		long integer;
	};
};

#endif /* LONE_STRUCT_VALUE_HEADER */
