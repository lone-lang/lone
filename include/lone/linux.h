/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LINUX_HEADER
#define LONE_LINUX_HEADER

#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fcntl.h>

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
linux_close(int fd);

ssize_t
__attribute__((fd_arg_read(1), tainted_args))
linux_read(int fd, const void *buffer, size_t count);

ssize_t
__attribute__((fd_arg_write(1), tainted_args))
linux_write(int fd, const void *buffer, size_t count);

off_t
__attribute__((tainted_args))
linux_lseek(int fd, off_t offset, int origin);

#endif /* LONE_LINUX_HEADER */
