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

static struct lone_value *lone_parse(struct lone_lisp *lone, struct lone_value *value)
{
	unsigned char *input = value->bytes.pointer;
	size_t size = value->bytes.count;

	for (size_t i = 0; i < size; ++i) {
		if (lone_is_space(input[i])) {
			continue;
		} else if (input[i] == '"') {
			size_t start = i + 1, end = start;
			do {
				if (end >= size) {
					goto parse_failed;
				}
			} while (input[end++] != '"');
			return lone_bytes_create(lone, input + start, end - start - 1);
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
	#define LONE_MEMORY_SIZE 65536
	static unsigned char memory[LONE_MEMORY_SIZE];
	struct lone_lisp lone = { memory, sizeof(memory), 0 };

	lone_print(lone_evaluate(lone_read(&lone, 0)));

	return 0;
}
