/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/architecture/linux/entry_point.c>
#include <lone/linux.h>

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxvec)
{
	char message[] = "Hello, world!\n";
	linux_system_call_3(__NR_write, 1, (long) message, sizeof(message) - 1);
	return 0;
}
