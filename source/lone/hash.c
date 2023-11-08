#include <lone/hash.h>
#include <lone/linux.h>

#include <lone/struct/lisp.h>
#include <lone/struct/value.h>

void lone_hash_initialize(struct lone_lisp *lone, struct lone_bytes random)
{
	lone_hash_fnv_1a_initialize(lone, random);
}

static size_t lone_hash_recursively(struct lone_value *key, unsigned long hash)
{
	struct lone_bytes bytes;

	if (!key) { /* a null key is probably a bug */ linux_exit(-1); }

	bytes.pointer = (unsigned char *) &key->type;
	bytes.count = sizeof(key->type);
	hash = lone_hash_fnv_1a(bytes, hash);

	if (lone_is_nil(key)) { return hash; }

	switch (key->type) {
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_VECTOR:
	case LONE_TABLE:
		linux_exit(-1);
	case LONE_LIST:
		hash = lone_hash_recursively(key->list.first, hash);
		hash = lone_hash_recursively(key->list.rest, hash);
		return hash;
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
		bytes = key->bytes;
		break;
	case LONE_INTEGER:
		bytes.pointer = (unsigned char *) &key->integer;
		bytes.count = sizeof(key->integer);
		break;
	case LONE_POINTER:
		bytes.pointer = (unsigned char *) &key->pointer;
		bytes.count = sizeof(key->pointer);
		break;
	}

	hash = lone_hash_fnv_1a(bytes, hash);

	return hash;
}

size_t lone_hash(struct lone_lisp *lone, struct lone_value *value)
{
	return lone_hash_recursively(value, lone->hash.fnv_1a.offset_basis);
}
