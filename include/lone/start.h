/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_START_HEADER
#define LONE_START_HEADER

#include <lone/auxiliary_vector.h>

/* lone_entry calls lone_start via basic inline asm. The LTO
 * optimizer cannot inspect asm operands, so the call is
 * invisible to whole-program analysis. Without the attributes
 * below the optimizer would internalize the symbol
 * (externally_visible), drop the function body (used), or let
 * --gc-sections strip its section (retain). */
__attribute__((externally_visible, used, retain))
long lone_start(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv);

#endif /* LONE_START_HEADER */
