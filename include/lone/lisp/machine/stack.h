/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MACHINE_STACK_HEADER
#define LONE_LISP_MACHINE_STACK_HEADER

#include <lone/lisp/types.h>

struct lone_lisp_machine_stack lone_lisp_machine_allocate_stack(struct lone_lisp *lone, size_t count);
void lone_lisp_machine_deallocate_stack(struct lone_lisp *lone, struct lone_lisp_machine_stack stack);

void lone_lisp_machine_shrink_stack(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_machine_stack_frame frame);
void lone_lisp_machine_push_frames(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames);
struct lone_lisp_machine_stack_frame lone_lisp_machine_pop(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_value(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_machine_pop_value(struct lone_lisp *lone, struct lone_lisp_machine *machine);
struct lone_lisp_value lone_lisp_machine_peek_value(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		size_t depth);

void lone_lisp_machine_push_integer(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		lone_lisp_integer integer);
lone_lisp_integer lone_lisp_machine_pop_integer(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_step(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		enum lone_lisp_machine_step step);
enum lone_lisp_machine_step lone_lisp_machine_pop_step(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_push_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		long primitive_step);
long lone_lisp_machine_pop_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_save_step(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_restore_step(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_save_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_restore_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value function);
void lone_lisp_machine_pop_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_primitive_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_pop_primitive_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_continuation_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_pop_continuation_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_pop_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);

bool lone_lisp_machine_top_is_tail_return(struct lone_lisp_machine *machine);

void lone_lisp_machine_unwind_to(struct lone_lisp *lone, struct lone_lisp_machine *machine, enum lone_lisp_tag tag);
bool lone_lisp_machine_unwind_to_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_unwind_to_primitive_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_unwind_to_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);

#endif /* LONE_LISP_MACHINE_STACK_HEADER */
