/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_START_HEADER
#define LONE_START_HEADER

#include <lone/auxiliary_vector.h>

__attribute__((externally_visible))
long lone_start(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv);

#endif /* LONE_START_HEADER */
