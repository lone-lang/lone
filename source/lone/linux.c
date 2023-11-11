/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/linux.h>

#include <lone/architecture/linux/system_calls.c>

void linux_exit(int code)
{
	linux_system_call_1(__NR_exit, code);
	__builtin_unreachable();
}

long linux_openat(int dirfd, unsigned char *path, int flags)
{
	return linux_system_call_4(__NR_openat, dirfd, (long) path, flags, 0);
}

long linux_close(int fd)
{
	return linux_system_call_1(__NR_close, fd);
}

ssize_t linux_read(int fd, const void *buffer, size_t count)
{
	return linux_system_call_3(__NR_read, fd, (long) buffer, (long) count);
}

ssize_t linux_write(int fd, const void *buffer, size_t count)
{
	return linux_system_call_3(__NR_write, fd, (long) buffer, (long) count);
}
