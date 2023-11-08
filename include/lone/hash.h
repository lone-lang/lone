#ifndef LONE_HASH_HEADER
#define LONE_HASH_HEADER

#include <lone/types.h>
#include <lone/hash/fnv_1a.h>
#include <lone/struct/bytes.h>

void lone_hash_initialize(struct lone_lisp *lone, struct lone_bytes random);

#endif /* LONE_HASH_HEADER */
