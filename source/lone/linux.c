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

long linux_pipe2(int fds[2], int flags)
{
	return linux_system_call_2(__NR_pipe2, (long) fds, flags);
}

long linux_close(int fd)
{
	return linux_system_call_1(__NR_close, fd);
}

long linux_fstat(int fd, struct stat *buffer)
{
	return linux_system_call_2(__NR_fstat, fd, (long) buffer);
}

ssize_t linux_read(int fd, const void *buffer, size_t count)
{
	return linux_system_call_3(__NR_read, fd, (long) buffer, (long) count);
}

ssize_t linux_read_once(int fd, struct lone_bytes buffer)
{
	ssize_t result;

	do {
		result = linux_read(fd, buffer.pointer, buffer.count);
	} while (result == -EINTR || result == -EAGAIN);

	return result;
}

ssize_t linux_read_bytes(int fd, struct lone_bytes buffer)
{
	size_t total;
	ssize_t result;

	total = 0;

	while (total < buffer.count) {
		result = linux_read(fd, buffer.pointer + total, buffer.count - total);

		if (result < 0) {
			if (result == -EINTR || result == -EAGAIN) { continue; }
			return result;
		}

		if (result == 0) { break; /* end of input */ }

		total += (size_t) result;
	}

	return (ssize_t) total;
}

ssize_t linux_write(int fd, const void *buffer, size_t count)
{
	return linux_system_call_3(__NR_write, fd, (long) buffer, (long) count);
}

ssize_t linux_write_bytes(int fd, struct lone_bytes buffer)
{
	size_t total;
	ssize_t result;

	total = 0;

	while (total < buffer.count) {
		result = linux_write(fd, buffer.pointer + total, buffer.count - total);

		if (result < 0) {
			if (result == -EINTR || result == -EAGAIN) { continue; }
			return result;
		}

		if (result == 0) { break; }

		total += (size_t) result;
	}

	return (ssize_t) total;
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
	ssize_t result;

	fd = linux_openat(AT_FDCWD, (unsigned char *) "/dev/urandom", O_RDONLY | O_CLOEXEC);
	if (fd < 0) { return fd; }

	result = linux_read_bytes(fd, buffer);
	linux_close(fd);

	if (result < 0) { return result; }
	if ((size_t) result < buffer.count) { return -EIO; }
	return 0;
}
