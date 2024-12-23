/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STACK_HEADER
#define LONE_STACK_HEADER

#include <lone/types.h>

/* ╭───────────────────────────┨ LONE STACK ┠───────────────────────────────╮
   │                                                                        │
   │    A stack consists of a memory block and the stack top.               │
   │    The memory block is described by base and limit pointers.           │
   │    The stack top is given by a pointer to the start of the             │
   │    next element. It points to the stack base when empty                │
   │    and to the stack limit when full. The stack can store               │
   │                                                                        │
   │    The stack can store elements of variable and arbitrary length.      │
   │    Users are responsible for maintaining the proper alignment of       │
   │    the stack pointer: for performance reasons, all stack functions     │
   │    access data via native instructions which may or may not have       │
   │    alignment requirements. Native endianness is also assumed           │
   │    unless otherwise stated.                                            │
   │                                                                        │
   │                                            out of bounds               │
   │                                    base + size ──┼── limit             │
   │                                                  ▼                     │
   │        base[size] ──► [.........................].                     │
   │                        ▲           ▲           ▲                       │
   │                 base ──┘           │           └── base + size - 1     │
   │                                   top                                  │
   │                                                                        │
   │        top <= base ─────────────────────────────────────► empty        │
   │        top >= limit ────────────────────────────────────► full         │
   │                                                                        │
   │        base[16] ──► [................].                                │
   │                      ▲                                                 │
   │                      └── top                                           │
   │                                                                        │
   │        lone_stack_push_u64(&top, a);                                   │
   │                                                                        │
   │        base[16] ──► [aaaaaaaa........].                                │
   │                              ▲                                         │
   │                              └── top                                   │
   │                                                                        │
   │        lone_stack_push_u32(&top, b);                                   │
   │                                                                        │
   │        base[16] ──► [aaaaaaaabbbb....].                                │
   │                                  ▲                                     │
   │                                  └── top                               │
   │                                                                        │
   │        lone_stack_push_u16(&top, c);                                   │
   │                                                                        │
   │        base[16] ──► [aaaaaaaabbbbcc..].                                │
   │                                    ▲                                   │
   │                                    └── top                             │
   │        lone_stack_push_u8(&top, d);                                    │
   │        lone_stack_push_u8(&top, e);                                    │
   │                                                                        │
   │        base[16] ──► [aaaaaaaabbbbccde].                                │
   │                                       ▲                                │
   │                                       └── top                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

bool lone_stack_is_empty(void *top, void *base);
bool lone_stack_is_full(void  *top, void *limit);
bool lone_stack_can_peek(void *top, void *base,  lone_size size);
bool lone_stack_can_pop(void  *top, void *base,  lone_size size);
bool lone_stack_can_push(void *top, void *limit, lone_size size);

void lone_stack_push_char(         void **top,          char value);
void lone_stack_push_signed_char(  void **top,   signed char value);
void lone_stack_push_unsigned_char(void **top, unsigned char value);

void lone_stack_push_short(             void **top,   signed short     value);
void lone_stack_push_unsigned_short(    void **top, unsigned short     value);
void lone_stack_push_int(               void **top,   signed int       value);
void lone_stack_push_unsigned_int(      void **top, unsigned int       value);
void lone_stack_push_long(              void **top,   signed long      value);
void lone_stack_push_unsigned_long(     void **top, unsigned long      value);
void lone_stack_push_long_long(         void **top,   signed long long value);
void lone_stack_push_unsigned_long_long(void **top, unsigned long long value);

void lone_stack_push_float(      void **top, float       value);
void lone_stack_push_double(     void **top, double      value);
void lone_stack_push_long_double(void **top, long double value);

void lone_stack_push_boolean(void **top, bool value);

void lone_stack_push_pointer(         void **top, void *                value);
void lone_stack_push_function_pointer(void **top, lone_function_pointer value);

void lone_stack_push_u8( void **top, lone_u8  value);
void lone_stack_push_s8( void **top, lone_s8  value);
void lone_stack_push_u16(void **top, lone_u16 value);
void lone_stack_push_s16(void **top, lone_s16 value);
void lone_stack_push_u32(void **top, lone_u32 value);
void lone_stack_push_s32(void **top, lone_s32 value);
void lone_stack_push_u64(void **top, lone_u64 value);
void lone_stack_push_s64(void **top, lone_s64 value);

         char lone_stack_pop_char(         void **top);
  signed char lone_stack_pop_signed_char(  void **top);
unsigned char lone_stack_pop_unsigned_char(void **top);

  signed short     lone_stack_pop_short(             void **top);
unsigned short     lone_stack_pop_unsigned_short(    void **top);
  signed int       lone_stack_pop_int(               void **top);
unsigned int       lone_stack_pop_unsigned_int(      void **top);
  signed long      lone_stack_pop_long(              void **top);
unsigned long      lone_stack_pop_unsigned_long(     void **top);
  signed long long lone_stack_pop_long_long(         void **top);
unsigned long long lone_stack_pop_unsigned_long_long(void **top);

float       lone_stack_pop_float(      void **top);
double      lone_stack_pop_double(     void **top);
long double lone_stack_pop_long_double(void **top);

bool lone_stack_pop_boolean(void **top);

void *                lone_stack_pop_pointer(         void **top);
lone_function_pointer lone_stack_pop_function_pointer(void **top);

lone_u8  lone_stack_pop_u8( void **top);
lone_s8  lone_stack_pop_s8( void **top);
lone_u16 lone_stack_pop_u16(void **top);
lone_s16 lone_stack_pop_s16(void **top);
lone_u32 lone_stack_pop_u32(void **top);
lone_s32 lone_stack_pop_s32(void **top);
lone_u64 lone_stack_pop_u64(void **top);
lone_s64 lone_stack_pop_s64(void **top);

         char lone_stack_peek_char(         void *top);
  signed char lone_stack_peek_signed_char(  void *top);
unsigned char lone_stack_peek_unsigned_char(void *top);

  signed short     lone_stack_peek_short(             void *top);
unsigned short     lone_stack_peek_unsigned_short(    void *top);
  signed int       lone_stack_peek_int(               void *top);
unsigned int       lone_stack_peek_unsigned_int(      void *top);
  signed long      lone_stack_peek_long(              void *top);
unsigned long      lone_stack_peek_unsigned_long(     void *top);
  signed long long lone_stack_peek_long_long(         void *top);
unsigned long long lone_stack_peek_unsigned_long_long(void *top);

float       lone_stack_peek_float(      void *top);
double      lone_stack_peek_double(     void *top);
long double lone_stack_peek_long_double(void *top);

bool lone_stack_peek_boolean(void *top);

void *                lone_stack_peek_pointer(         void *top);
lone_function_pointer lone_stack_peek_function_pointer(void *top);

lone_u8  lone_stack_peek_u8( void *top);
lone_s8  lone_stack_peek_s8( void *top);
lone_u16 lone_stack_peek_u16(void *top);
lone_s16 lone_stack_peek_s16(void *top);
lone_u32 lone_stack_peek_u32(void *top);
lone_s32 lone_stack_peek_s32(void *top);
lone_u64 lone_stack_peek_u64(void *top);
lone_s64 lone_stack_peek_s64(void *top);

#endif /* LONE_STACK_HEADER */
