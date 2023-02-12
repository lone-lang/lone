#include <linux/types.h>
#include <linux/unistd.h>

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
	static const char hello[] = "Hello world, from lone\n";
	system_call(__NR_write, 1, (long) hello, sizeof(hello) - 1, 0, 0, 0);
	return 0;
}
