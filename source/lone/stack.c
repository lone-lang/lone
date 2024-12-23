#include <lone/stack.h>

bool lone_stack_is_empty(void *top, void *base)
{
	return ((unsigned char *) top) <= base;
}

bool lone_stack_is_full(void *top, void *limit)
{
	return ((unsigned char *) top) >= limit;
}

bool lone_stack_can_peek(void *top, void *base, lone_size bytes)
{
	return lone_stack_can_pop(top, base, bytes);
}

bool lone_stack_can_pop(void *top, void *base, lone_size bytes)
{
	return (((unsigned char *) top) - bytes) >= base;
}

bool lone_stack_can_push(void *top, void *limit, lone_size bytes)
{
	return (((unsigned char *) top) + bytes) <= limit;
}

#define LONE_STACK_PUSH(__name, __type)                 \
void lone_stack_push_##__name(void **top, __type value) \
{                                                       \
	__type *pointer;                                \
	pointer = *top;                                 \
	*pointer = value;                               \
	++pointer;                                      \
	*top = pointer;                                 \
}

LONE_STACK_PUSH(         char,          char)
LONE_STACK_PUSH(  signed_char,   signed char)
LONE_STACK_PUSH(unsigned_char, unsigned char)

LONE_STACK_PUSH(         short,       signed short)
LONE_STACK_PUSH(unsigned_short,     unsigned short)
LONE_STACK_PUSH(         int,         signed int)
LONE_STACK_PUSH(unsigned_int,       unsigned int)
LONE_STACK_PUSH(         long,        signed long)
LONE_STACK_PUSH(unsigned_long,      unsigned long)
LONE_STACK_PUSH(         long_long,   signed long long)
LONE_STACK_PUSH(unsigned_long_long, unsigned long long)

LONE_STACK_PUSH(float,       float)
LONE_STACK_PUSH(double,      double)
LONE_STACK_PUSH(long_double, long double)

LONE_STACK_PUSH(boolean, bool)

LONE_STACK_PUSH(pointer,          void  *)
LONE_STACK_PUSH(function_pointer, lone_function_pointer)

LONE_STACK_PUSH(u8,  lone_u8)
LONE_STACK_PUSH(s8,  lone_s8)
LONE_STACK_PUSH(u16, lone_u16)
LONE_STACK_PUSH(s16, lone_s16)
LONE_STACK_PUSH(u32, lone_u32)
LONE_STACK_PUSH(s32, lone_s32)
LONE_STACK_PUSH(u64, lone_u64)
LONE_STACK_PUSH(s64, lone_s64)

#undef LONE_STACK_PUSH

#define LONE_STACK_POP(__name, __type)     \
__type lone_stack_pop_##__name(void **top) \
{                                          \
	__type *pointer;                   \
	__type value;                      \
	pointer = *top;                    \
	--pointer;                         \
	value = *pointer;                  \
	*top = pointer;                    \
	return value;                      \
}

LONE_STACK_POP(         char,          char)
LONE_STACK_POP(  signed_char,   signed char)
LONE_STACK_POP(unsigned_char, unsigned char)

LONE_STACK_POP(         short,       signed short)
LONE_STACK_POP(unsigned_short,     unsigned short)
LONE_STACK_POP(         int,         signed int)
LONE_STACK_POP(unsigned_int,       unsigned int)
LONE_STACK_POP(         long,        signed long)
LONE_STACK_POP(unsigned_long,      unsigned long)
LONE_STACK_POP(         long_long,   signed long long)
LONE_STACK_POP(unsigned_long_long, unsigned long long)

LONE_STACK_POP(float,       float)
LONE_STACK_POP(double,      double)
LONE_STACK_POP(long_double, long double)

LONE_STACK_POP(boolean, bool)

LONE_STACK_POP(pointer,          void  *)
LONE_STACK_POP(function_pointer, lone_function_pointer)

LONE_STACK_POP(u8,  lone_u8)
LONE_STACK_POP(s8,  lone_s8)
LONE_STACK_POP(u16, lone_u16)
LONE_STACK_POP(s16, lone_s16)
LONE_STACK_POP(u32, lone_u32)
LONE_STACK_POP(s32, lone_s32)
LONE_STACK_POP(u64, lone_u64)
LONE_STACK_POP(s64, lone_s64)

#undef LONE_STACK_POP

#define LONE_STACK_PEEK(__name, __type)    \
__type lone_stack_peek_##__name(void *top) \
{                                          \
	__type *pointer;                   \
	__type value;                      \
	pointer = top;                     \
	value = *--pointer;                \
	return value;                      \
}

LONE_STACK_PEEK(         char,          char)
LONE_STACK_PEEK(  signed_char,   signed char)
LONE_STACK_PEEK(unsigned_char, unsigned char)

LONE_STACK_PEEK(         short,       signed short)
LONE_STACK_PEEK(unsigned_short,     unsigned short)
LONE_STACK_PEEK(         int,         signed int)
LONE_STACK_PEEK(unsigned_int,       unsigned int)
LONE_STACK_PEEK(         long,        signed long)
LONE_STACK_PEEK(unsigned_long,      unsigned long)
LONE_STACK_PEEK(         long_long,   signed long long)
LONE_STACK_PEEK(unsigned_long_long, unsigned long long)

LONE_STACK_PEEK(float,       float)
LONE_STACK_PEEK(double,      double)
LONE_STACK_PEEK(long_double, long double)

LONE_STACK_PEEK(boolean, bool)

LONE_STACK_PEEK(pointer,          void  *)
LONE_STACK_PEEK(function_pointer, lone_function_pointer)

LONE_STACK_PEEK(u8,  lone_u8)
LONE_STACK_PEEK(s8,  lone_s8)
LONE_STACK_PEEK(u16, lone_u16)
LONE_STACK_PEEK(s16, lone_s16)
LONE_STACK_PEEK(u32, lone_u32)
LONE_STACK_PEEK(s32, lone_s32)
LONE_STACK_PEEK(u64, lone_u64)
LONE_STACK_PEEK(s64, lone_s64)

#undef LONE_STACK_PEEK
