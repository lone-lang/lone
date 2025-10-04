/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MACHINE_HEADER
#define LONE_LISP_MACHINE_HEADER

#include <lone/lisp/types.h>

void lone_lisp_machine_reset(struct lone_lisp *lone, struct lone_lisp_value module, struct lone_lisp_value expression);
bool lone_lisp_machine_cycle(struct lone_lisp *lone);

void lone_lisp_machine_push(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_machine_stack_frame frame);
void lone_lisp_machine_push_frames(struct lone_lisp *lone,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames);
struct lone_lisp_machine_stack_frame lone_lisp_machine_pop(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_value(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_machine_pop_value(struct lone_lisp *lone, struct lone_lisp_machine *machine);

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

void lone_lisp_machine_push_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);
void lone_lisp_machine_pop_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine);

void lone_lisp_machine_push_continuation_delimiter(struct lone_lisp *lone);
void lone_lisp_machine_pop_continuation_delimiter(struct lone_lisp *lone);

void lone_lisp_machine_unwind_to(struct lone_lisp *lone, enum lone_lisp_machine_stack_frame_type type);
void lone_lisp_machine_unwind_to_function_delimiter(struct lone_lisp *lone);

#endif /* LONE_LISP_MACHINE_HEADER */
