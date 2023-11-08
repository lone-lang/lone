#include <lone/hash.h>

void lone_hash_initialize(struct lone_lisp *lone, struct lone_bytes random)
{
	lone_hash_fnv_1a_initialize(lone, random);
}
