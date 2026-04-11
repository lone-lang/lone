# Tagged value representation

## Overview

Lone lisp values are a single 64-bit `long`.
The low byte is the tag. The upper 56 bits are the payload.


    63                                8 7          0
    ┌──────────────────────────────────┬────────────┐
    │          payload (56 bits)       │  tag (8)   │
    └──────────────────────────────────┴────────────┘

For heap-referencing values, the 56-bit payload
further splits into 16 bits of per-value metadata
and a 40-bit heap index:

    63                     24 23           8 7        0
    ┌────────────────────────┬──────────────┬──────────┐
    │    index (40 bits)     │ metadata (16)│ tag (8)  │
    └────────────────────────┴──────────────┴──────────┘

For non-heap values such as integers, singletons and inline data,
the full 56-bit payload carries data directly.

## Tags

Bit 0 is the register/heap value discriminator.

    bit 0 = 0  →  heap reference: index + metadata in payload
    bit 0 = 1  →  register value: data directly in payload

Allows discriminating between heap references and register values
with a single instruction:

```c
if (value.tagged & 1) {
    /* register value */
} else {
    /* heap value */
}
```

### Heap values

Words where bit 0 = 0.

Bits 1-4 encode the heap type.
16 slots, currently 11 are used.
Bits 5-7 are available for other
uses such as type group flags.

| Tag value | Bits `4321` | Type         |
|:---------:|------------:|--------------|
|   `0x00`  |      `0000` | Module       |
|   `0x02`  |      `0001` | Function     |
|   `0x04`  |      `0010` | Primitive    |
|   `0x06`  |      `0011` | Continuation |
|   `0x08`  |      `0100` | Generator    |
|   `0x0A`  |      `0101` | List         |
|   `0x0C`  |      `0110` | Vector       |
|   `0x0E`  |      `0111` | Table        |
|   `0x10`  |      `1000` | Symbol       |
|   `0x12`  |      `1001` | Text         |
|   `0x14`  |      `1010` | Bytes        |
|   `0x16`  |      `1011` | Reserved     |
|   `0x18`  |      `1100` | Reserved     |
|   `0x1A`  |      `1101` | Reserved     |
|   `0x1C`  |      `1110` | Reserved     |
|   `0x1E`  |      `1111` | Reserved     |

### Register values

Words where bit 0 = 1.
The 56 data bits directly
carry data.

| Tag value | Type    | Data bits                            |
|:---------:|---------|--------------------------------------|
|   `0x01`  | Integer | 56-bit 2's complement signed integer |
|   `0x03`  | Nil     | Zero                                 |
|   `0x05`  | True    | Zero                                 |
|   `0x07`  | False   | Zero                                 |

## Data

Register values directly encode data in their 56 data bits.
Heap values carry data and metadata in those same bits.

Interpretation of the 16 metadata bits
depend on the heap value's type:

 - **Symbols**
   - 8 hash bits
     Enables fast rejection during hash table lookups
     without heap access: 255/256 rejection probability
     for non-matching entries.
   - 8 free bits

 - **Functions**/**Primitives**
   - FEXPR flags
     - Evaluate arguments?
     - Evaluate result?
   - 6 arity bits
     - Not yet implemented
   - 8 free bits

### Stack frames

Some words are not normal lisp values,
they are metadata values that exist
only on the lisp machine stack.

Step words carry a step number in the data bits.
Delimiter words carry a heap reference which is
used for various purposes: prompt tag, enclosing
generator reference or other metadata. These heap
references use the same 16-bit metadata + 40-bit
index layout as normal heap values.

| Tag value | Type                   | Data bits                                 |
|:---------:|------------------------|-------------------------------------------|
|   `0x21`  | Lisp machine step      | Enumerated lisp machine step value        |
|   `0x23`  | Primitive step         | 56-bit 2's complement signed step value   |
|   `0x41`  | Function delimiter     | Heap reference                            |
|   `0x43`  | Continuation delimiter | Heap reference                            |
|   `0x60`  | Generator delimiter    | Heap reference                            |

Normal lone lisp values and integers keep their own tags
when pushed onto the stack. Only the frame types with no
corresponding lisp value need their own tags.

The generator delimiter is the only even-tagged frame type
as of the current implementation. It is used to delimit
generator functions and it also refers to the generator
itself. Bit 0 = 0 so that the GC traces the reference
to the generator. Function and continuation delimiters
are odd-tagged for now but may carry heap references
in the future, prompt tags for delimited continuations
for example.

### Inline small values

Remaining register value patterns encode type + length.
Data bits hold up to 7 bytes directly.

|  Bits |                   Value | Meaning                                      |
|------:|------------------------:|----------------------------------------------|
|   `0` |                     `1` | Register value flag                          |
| `123` |                   `0-7` | Inline value length                          |
| `456` | `000`<br>`001`<br>`010` | Inline symbol<br>Inline text<br>Inline bytes |
|   `7` |                     `1` | Inline value flag                            |

Full inline value tag enumeration:

| Tag value | Inline value type | Inline value length |
|----------:|:-----------------:|---------------------|
|    `0x81` |       Symbol      | `0`                 |
|    `0x83` |       Symbol      | `1`                 |
|    `0x85` |       Symbol      | `2`                 |
|    `0x87` |       Symbol      | `3`                 |
|    `0x89` |       Symbol      | `4`                 |
|    `0x8B` |       Symbol      | `5`                 |
|    `0x8D` |       Symbol      | `6`                 |
|    `0x8F` |       Symbol      | `7`                 |
|    `0x91` |        Text       | `0`                 |
|    `0x93` |        Text       | `1`                 |
|    `0x95` |        Text       | `2`                 |
|    `0x97` |        Text       | `3`                 |
|    `0x99` |        Text       | `4`                 |
|    `0x9B` |        Text       | `5`                 |
|    `0x9D` |        Text       | `6`                 |
|    `0x9F` |        Text       | `7`                 |
|    `0xA1` |       Bytes       | `0`                 |
|    `0xA3` |       Bytes       | `1`                 |
|    `0xA5` |       Bytes       | `2`                 |
|    `0xA7` |       Bytes       | `3`                 |
|    `0xA9` |       Bytes       | `4`                 |
|    `0xAB` |       Bytes       | `5`                 |
|    `0xAD` |       Bytes       | `6`                 |
|    `0xAF` |       Bytes       | `7`                 |

Type detection masks:

| Inline value type |          Test          |
|------------------:|:----------------------:|
|            Symbol | `(tag & 0xF1) == 0x81` |
|              Text | `(tag & 0xF1) == 0x91` |
|             Bytes | `(tag & 0xF1) == 0xA1` |
|               Any | `(tag & 0x81) == 0x81` |

Common data that fit inline within 7 bytes:

| Inline value type | Length | Examples                                                          |
|------------------:|:------:|-------------------------------------------------------------------|
|            Symbol |    1   | `+` `-` `*` `/` `<` `>` `=` `.`                                   |
|            Symbol |    2   | `if` `do` `fn` `or` `==` `<=` `>=` `->` `=>`                      |
|            Symbol |    3   | `def` `let` `set` `car` `cdr` `nil` `not` `and` `map` `===` `<=>` |
|            Symbol |    4   | `list` `cons` `rest` `true` `read` `sort`                         |
|            Symbol |    5   | `first` `false` `quote` `begin` `print` `apply`                   |
|            Symbol |    6   | `lambda` `define` `import` `export` `filter`                      |
|            Symbol |    7   | `println` `display` `unquote` `flatten` `require`                 |
|              Text |    0   | `""`                                                              |
|              Text |    1   | `"\n"` `" "` `"/"` `"."` `","` `";"` `"0"`                        |
|              Text |    2   | `"\r\n"` `", "` `": "`                                            |

## Design parameters

### Tag extraction

```c
value.tagged & 0xFF                  /* 8-bit tag */
((unsigned long) value.tagged) >> 8  /* 56-bit payload */
```

### Heap value index extraction

```c
((unsigned long) value.tagged) >> 24  /* 40-bit index */
```

### Heap value metadata extraction

```c
(((unsigned long) value.tagged) >> 8) & 0xFFFF  /* 16-bit metadata */
```

### Integer range

56-bit signed integers: -2^55 to 2^55-1.
Range exceeds +/- 3.6 x 10^16.
For the record, JavaScript's safe integer range is 2^53.

### Heap capacity

2^40 = 1,099,511,627,776 heap values.
Each heap value is 64 bytes.
That gives 64 TiB of heap values.
No machine exists with this much RAM.
Enforced by a bounds check in `lone_lisp_heap_grow`.

### Heap vs general-purpose memory

The heap object count limit applies only
to the interpreter's value heaps:
lists, symbols, functions, hash tables, etc.

Bulk data lives in separately allocated memory.
Users can process terabytes of data
with a modest number of heap objects.

### Garbage collection scanning

The conservative native stack scanner
applies two tests to each word
on the C stack:

 1. **Pointer check**
    - Does the raw value point into the heap array's memory range?
 2. **Tagged value check**
    - Is bit 0 = 0?
      - If so, is the 40-bit index within heap bounds?

```c
if (points_to_heap(word)) {
    /* raw pointer into heap array, pin and mark */
}

if ((word & 1) == 0) {
    size_t index = ((unsigned long) word) >> 24;
    if (index < heap->count) {
        /* potential tagged heap reference, pin and mark */
    }
}
```

The 8-bit tag provides stricter filtering than 3 bits,
reducing false positives from random stack data.
Nonetheless, values found by the conservative scanner
are pinned so that they are not moved during compaction.

The precise lisp machine stack scanner walks every frame
and exclusively checks bit 0: odd-tagged frames are
skipped, even-tagged frames are traced as heap references.
This naturally handles the generator delimiter (`0x60`)
without any special case logic.

### Forwarding indexes during compaction

During heap compaction, the compactor
rewrites the index in every reference
to a moved value. The tag and metadata
bits are preserved: type and metadata
don't change on move, only the upper
40 index bits are rewritten.

## Architecture support

See `documentation/linux/memory-map.md` for the full survey.

The 40-bit index supports 2^40
heap values which equals 64 TiB.
This exceeds the RAM of the largest machines
on every 64-bit Linux architecture including `s390x`.

The 8-bit tag with 40-bit index cap is enforced
by a bounds check in `lone_lisp_heap_grow`.
This makes the design safe on all architectures
without any platform-specific conditionals.
