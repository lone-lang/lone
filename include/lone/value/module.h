/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_MODULE_HEADER
#define LONE_VALUE_MODULE_HEADER

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone modules are named isolated environments for evaluation.        │
   │                                                                        │
   │    The lisp interpreter contains a top level environment.              │
   │    This environment contains only the import/export primitives.        │
   │    New modules have clean environments which inherit from it.          │
   │    This allows complete control over the symbols and the namespace.    │
   │                                                                        │
   │    Each module corresponds roughly to one .ln file on disk.            │
   │    These module files are text files containing lone lisp code         │
   │    which may import or export symbols from or to other modules.        │
   │    The lone interpreter's import primitive will search for files       │
   │    to load in conventional locations, enabling library development.    │
   │                                                                        │
   │    A special nameless module known as the null module                  │
   │    contains code read in from standard input.                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_value lone_module_create(struct lone_lisp *lone, struct lone_value name);

#endif /* LONE_VALUE_MODULE_HEADER */
