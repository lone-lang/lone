#include <lone/value/bytes.h>

#include <lone/struct/value.h>

struct lone_value *lone_symbol_transfer(struct lone_lisp *lone, unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_value *value = lone_bytes_transfer(lone, text, length, should_deallocate);
	value->type = LONE_SYMBOL;
	return value;
}

struct lone_value *lone_symbol_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate)
{
	return lone_symbol_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_value *lone_symbol_create(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_create(lone, text, length);
	value->type = LONE_SYMBOL;
	return value;
}
