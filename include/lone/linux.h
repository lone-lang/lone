/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LINUX_HEADER
#define LONE_LINUX_HEADER

#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/mman.h>
#include <linux/stat.h>
#include <asm/stat.h>

#include <lone/types.h>

long linux_system_call_0(long n);
long linux_system_call_1(long n, long _1);
long linux_system_call_2(long n, long _1, long _2);
long linux_system_call_3(long n, long _1, long _2, long _3);
long linux_system_call_4(long n, long _1, long _2, long _3, long _4);
long linux_system_call_5(long n, long _1, long _2, long _3, long _4, long _5);
long linux_system_call_6(long n, long _1, long _2, long _3, long _4, long _5, long _6);

void
__attribute__((noreturn))
linux_exit(int code);

long
__attribute__((tainted_args))
linux_openat(int dirfd, unsigned char *path, int flags);

long
__attribute__((tainted_args))
linux_pipe2(int fds[2], int flags);

long
__attribute__((tainted_args))
linux_close(int fd);

long
__attribute__((fd_arg_read(1), tainted_args))
linux_fstat(int fd, struct stat *buffer);

ssize_t
__attribute__((fd_arg_read(1), tainted_args))
linux_read(int fd, const void *buffer, size_t count);

/* Performs exactly one read system call.
 * Loops on EINTR and EAGAIN. Returns the kernel's value:
 * bytes read (0 == end of input) or the negative errno on failure. */
ssize_t
__attribute__((fd_arg_read(1), tainted_args))
linux_read_once(int fd, struct lone_bytes buffer);

/* Fills a lone bytes structure by reading from a file descriptor.
 * Loops on EINTR and EAGAIN. Returns the total bytes read,
 * or the negative kernel errno on error. If the end of input
 * is reached, total bytes read may be less than buffer.count.
 */
ssize_t
__attribute__((fd_arg_read(1), tainted_args))
linux_read_bytes(int fd, struct lone_bytes buffer);

/* Fills a lone bytes structure by reading /dev/urandom.
 * Returns 0 on success or the error code from the failing system call. */
long linux_dev_urandom(struct lone_bytes buffer);

ssize_t
__attribute__((fd_arg_write(1), tainted_args))
linux_write(int fd, const void *buffer, size_t count);

/* Drains a lone bytes structure to a file descriptor.
 * Loops on EINTR and EAGAIN. Returns the total bytes
 * written, or the negative kernel errno on error.
 * Total bytes written may be less than buffer.count
 * if a zero byte write occurs.
 */
ssize_t
__attribute__((fd_arg_write(1), tainted_args))
linux_write_bytes(int fd, struct lone_bytes buffer);

off_t
__attribute__((tainted_args))
linux_lseek(int fd, off_t offset, int origin);

intptr_t
__attribute__((tainted_args))
linux_mmap(void *address, size_t length, int protections, int flags, int file_descriptor, off_t offset);

int
__attribute__((tainted_args))
linux_munmap(void *address, size_t length);

intptr_t
__attribute__((tainted_args))
linux_mremap(void *address, size_t old_length, size_t new_length, unsigned long flags, void *new_address);

#endif /* LONE_LINUX_HEADER */
