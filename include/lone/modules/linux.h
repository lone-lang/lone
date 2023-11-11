/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_LINUX_HEADER
#define LONE_MODULES_LINUX_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

#include <lone/struct/auxiliary.h>

/* ╭────────────────────────┨ LONE / MODULE / LINUX ┠───────────────────────╮
   │                                                                        │
   │    Linux system calls and process parameters.                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_module_linux_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv);

LONE_PRIMITIVE(linux_system_call);

#endif /* LONE_MODULES_LINUX_HEADER */
