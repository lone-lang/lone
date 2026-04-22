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

off_t linux_lseek(int fd, off_t offset, int origin)
{
	return linux_system_call_3(__NR_lseek, fd, (long) offset, (long) origin);
}

intptr_t linux_mmap(void *address, size_t length, int protections, int flags, int file_descriptor, off_t offset)
{
	return linux_system_call_6(__NR_mmap, (long) address, (long) length, (long) protections, (long) flags, (long) file_descriptor, (long) offset);
}

int linux_munmap(void *address, size_t length)
{
	return linux_system_call_2(__NR_munmap, (long) address, (long) length);
}

intptr_t linux_mremap(void *address, size_t old_length, size_t new_length, unsigned long flags, void *new_address)
{
	return linux_system_call_5(
		__NR_mremap,
		(long) address,
		(long) old_length,
		(long) new_length,
		(long) flags,
		(long) new_address
	);
}

long linux_dev_urandom(struct lone_bytes buffer)
{
	int fd;
	ssize_t n;
	size_t total;

	fd = linux_openat(AT_FDCWD, (unsigned char *) "/dev/urandom", O_RDONLY | O_CLOEXEC);
	if (fd < 0) { return fd; }

	total = 0;
	while (total < buffer.count) {
		n = linux_read(fd, buffer.pointer + total, buffer.count - total);
		if (n <= 0) { linux_close(fd); return n == 0 ? -EIO : n; }
		total += (size_t) n;
	}

	linux_close(fd);
	return 0;
}
