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
	LONE_LIST = 1,
};

struct lone_bytes {
	size_t count;
	unsigned char *pointer;
};

struct lone_list {
	struct lone_value *first;
	struct lone_value *rest;
};

struct lone_value {
	enum lone_type type;
	union {
		struct lone_bytes bytes;
		struct lone_list list;
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

static struct lone_value *lone_list_create(struct lone_lisp *lone, struct lone_value *first, struct lone_value *rest)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_LIST;
	value->list.first = first;
	value->list.rest = rest;
	return value;
}

static struct lone_value *lone_list_create_nil(struct lone_lisp *lone)
{
	return lone_list_create(lone, 0, 0);
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

static ssize_t lone_bytes_find(char byte, unsigned char *bytes, size_t size)
{
	size_t i = 0;

	do {
		if (i >= size) { return -1; }
	} while (bytes[i++] != byte);

	return i;
}

static struct lone_value *lone_list_set(struct lone_value *list, struct lone_value *value)
{
	return list->list.first = value;
}

static struct lone_value *lone_list_append(struct lone_value *list, struct lone_value *rest)
{
	return list->list.rest = rest;
}

static struct lone_value *lone_parse(struct lone_lisp *lone, struct lone_value *value)
{
	unsigned char *input = value->bytes.pointer;
	size_t size = value->bytes.count;

	for (size_t i = 0; i < size; ++i) {
		if (lone_is_space(input[i])) {
			continue;
		} else {
			size_t start = i, remaining = size - start, end = 0;
			unsigned char *position = input + start;
			switch (*position) {
			case '"':
				end = lone_bytes_find('"', position + 1, remaining - 1);
				if (end < 0) { goto parse_failed; }
				return lone_bytes_create(lone, position + 1, end - 1);
			default:
				goto parse_failed;
			}
		}
	}

parse_failed:
	linux_exit(-1);
}

static struct lone_value *lone_read_all_input(struct lone_lisp *lone, int fd)
{
	#define LONE_BUFFER_SIZE 4096
	unsigned char *input = lone_allocate(lone, LONE_BUFFER_SIZE);
	size_t bytes_read = 0, total_read = 0;

	while (1) {
		bytes_read = linux_read(fd, input + total_read, LONE_BUFFER_SIZE);

		if (bytes_read < 0) {
			linux_exit(-1);
		}

		total_read += bytes_read;

		if (bytes_read == LONE_BUFFER_SIZE) {
			lone_allocate(lone, LONE_BUFFER_SIZE);
		} else {
			break;
		}
	}

	return lone_bytes_create(lone, input, total_read);
}

static struct lone_value *lone_read(struct lone_lisp *lone, int fd)
{
	return lone_parse(lone, lone_read_all_input(lone, fd));
}

static struct lone_value *lone_evaluate(struct lone_lisp *lone, struct lone_value *value)
{
	switch (value->type) {
	case LONE_BYTES:
	case LONE_LIST:
		return value;
		break;
	default:
		linux_exit(-1);
	}
}

static void lone_print(struct lone_lisp *lone, struct lone_value *value)
{
	if (value == 0)
		return;

	switch (value->type) {
	case LONE_LIST:
		lone_print(lone, value->list.first);
		lone_print(lone, value->list.rest);
		break;
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
	#define LONE_MEMORY_SIZE 65536
	static unsigned char memory[LONE_MEMORY_SIZE];
	struct lone_lisp lone = { memory, sizeof(memory), 0 };

	lone_print(&lone, lone_evaluate(&lone, lone_read(&lone, 0)));

	return 0;
}
