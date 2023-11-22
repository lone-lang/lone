/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_UTILITIES_HEADER
#define LONE_UTILITIES_HEADER

#include <lone/types.h>

#include <lone/struct/bytes.h>
#include <lone/struct/auxiliary.h>
#include <lone/struct/elf.h>

struct lone_value *lone_apply_predicate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate function);
struct lone_value *lone_apply_comparator(struct lone_lisp *lone, struct lone_value *arguments, lone_comparator function);
struct lone_bytes lone_join(struct lone_lisp *lone, struct lone_value *separator, struct lone_value *arguments, lone_predicate is_valid);
struct lone_bytes lone_concatenate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate is_valid);

struct auxiliary_value lone_auxiliary_vector_value(struct auxiliary_vector *values, long type);
size_t lone_auxiliary_vector_page_size(struct auxiliary_vector *values);
struct lone_bytes lone_auxiliary_vector_random(struct auxiliary_vector *values);
struct lone_elf_program_header_table lone_auxiliary_vector_elf_program_header_table(struct auxiliary_vector *values);

#endif /* LONE_UTILITIES_HEADER */

