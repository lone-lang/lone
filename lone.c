#include <linux/types.h>
#include <linux/unistd.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;

static void __attribute__((noreturn)) linux_exit(int code)
{
	system_call_1(__NR_exit, code);
	__builtin_unreachable();
}

static ssize_t linux_read(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_read, fd, (long) buffer, count);
}

static ssize_t linux_write(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_write, fd, (long) buffer, count);
}

struct lone_lisp {
	unsigned char *memory;
	size_t capacity;
	size_t allocated;
};

enum lone_type {
	LONE_BYTES = 0,
};

struct lone_bytes {
	size_t count;
	unsigned char *pointer;
};

struct lone_value {
	enum lone_type type;
	union {
		struct lone_bytes bytes;
	};
};

static void *lone_allocate(struct lone_lisp *lone, size_t size)
{
	if (lone->allocated + size > lone->capacity)
		linux_exit(-1);
	void *allocation = lone->memory + lone->allocated;
	lone->allocated += size;
	return allocation;
}

static struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	return lone_allocate(lone, sizeof(struct lone_value));
}

static struct lone_value *lone_bytes_create(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_BYTES;
	value->bytes.count = count;
	value->bytes.pointer = pointer;
	return value;
}

static int lone_is_space(char c)
{
	switch (c) {
	case ' ':
	case '\t':
	case '\n':
		return 1;
	default:
		return 0;
	}
}

static struct lone_value *lone_read(char *buffer, size_t size)
{
	size = linux_read(0, buffer, size);
	if (size == 0) {
		linux_exit(0);
	}

	static struct lone_value static_value;

	static_value.type = LONE_BYTES;
	static_value.bytes = (struct lone_bytes) { size, buffer };

	return &static_value;
}

static struct lone_value *lone_evaluate(struct lone_value *value)
{
	switch (value->type) {
	case LONE_BYTES:
		return value;
		break;
	default:
		linux_exit(-1);
	}
}

static void lone_print(struct lone_value *value)
{
	switch (value->type) {
	case LONE_BYTES:
		linux_write(1, value->bytes.pointer, value->bytes.count);
		break;
	default:
		linux_exit(-1);
	}
}

#if __BITS_PER_LONG == 64
typedef __u64 auxiliary_value;
#elif __BITS_PER_LONG == 32
typedef __u32 auxiliary_value;
#else
#error "Unsupported architecture"
#endif

struct auxiliary {
	auxiliary_value type;
	auxiliary_value value;
};

long lone(int count, char **arguments, char **environment, struct auxiliary *values)
{
	static char buffer[4096];

	while (1) {
		lone_print(lone_evaluate(lone_read(buffer, sizeof(buffer))));
	}

	return 0;
}
