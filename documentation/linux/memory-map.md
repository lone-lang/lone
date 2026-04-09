# Linux user space memory maps

All actively maintained 64-bit Linux architectures.
Only `long` is used as the value type in lone.
`sizeof(long) == 8` on every 64-bit Linux platform (LP64 data model).

## Address space summary

| Architecture   | End                | VA bits | Page size | Page table levels |
| :------------: | :----------------- | :-----: | :-------: | :---------------: |
| `x86_64`       | `00007fffffffffff` |   47    |    4 KB   |         4         |
| `x86_64`       | `00ffffffffffffff` |   56    |    4 KB   |         5         |
| `aarch64`      | `0000003fffffffff` |   36    |   16 KB   |     2 (EXPERT)    |
| `aarch64`      | `0000007fffffffff` |   39    |    4 KB   |         3         |
| `aarch64`      | `000003ffffffffff` |   42    |   64 KB   |         2         |
| `aarch64`      | `00003fffffffffff` |   47    |   16 KB   |         3         |
| `aarch64`      | `0000ffffffffffff` |   48    |  4/16 KB  |        3/4        |
| `aarch64`      | `000fffffffffffff` |   52    |   64 KB   |      3 (LVA)      |
| `riscv64`      | `0000003fffffffff` |   39    |    4 KB   |      3 (Sv39)     |
| `riscv64`      | `00007fffffffffff` |   48    |    4 KB   |      4 (Sv48)     |
| `riscv64`      | `00ffffffffffffff` |   57    |    4 KB   |      5 (Sv57)     |
| `ppc64`        | `00003fffffffffff` |   46    |    4 KB   |         4         |
| `ppc64`        | `00007fffffffffff` |   47    |   64 KB   |         4         |
| `ppc64`        | `0001ffffffffffff` |   49    |   64 KB   |         4         |
| `ppc64`        | `000fffffffffffff` |   52    |   64 KB   |         4         |
| `loongarch64`  | `0000003fffffffff` |   39    |    4 KB   |         3         |
| `loongarch64`  | `000003ffffffffff` |   42    |   64 KB   |         2         |
| `loongarch64`  | `00003fffffffffff` |   47    |   16 KB   |         3         |
| `loongarch64`  | `0000ffffffffffff` |   48    |    4 KB   |         4         |
| `loongarch64`  | `007fffffffffffff` |   55    |   64 KB   |         3         |
| `s390x`        | `fffffffffffff000` |  ~64    |    4 KB   |         5         |
| `mips64`       | `000000ffffffffff` |   40    |    4 KB   |         3         |
| `mips64`       | `0000ffffffffffff` |   48    |    4 KB   |         4         |
| `sparc64`      | `00000fffffffffff` |   44    |    8 KB   |         4         |

## Canonical form and address space structure

Most architectures enforce a canonical address structure
where unused high bits must replicate the sign
of the virtual address space range, creating a "hole"
in the middle of the address space that separates
user space from kernel space. User space addresses
always have the high bits set to zero.

`s390x` is the notable exception: it has a flat 64-bit address space
with no canonical hole: `TASK_SIZE_MAX = 0xFFFFFFFFFFFFF000`.
The entire range minus one page is theoretically available
to user space. In practice, the address space grows dynamically:
new processes start with a 4 TB (42-bit) space which the kernel
upgrades as needed. However, correctness demands accounting
for the theoretical maximum.

`aarch64` uses bit 55 for TTBR selection, not bit 63.
Top Byte Ignore (TBI) may allow bits 63:56 for software use,
but this is a kernel/hardware configuration choice.

# References

 - [`x86_64`]
 - [`arm64`]
 - [`riscv`]
 - [`s390`]
 - [`powerpc`]
 - [`loongarch`]
 - [`mips`]

[`x86_64`]: https://www.kernel.org/doc/html/latest/arch/x86/x86_64/mm.html
[`arm64`]: https://www.kernel.org/doc/html/latest/arch/arm64/memory.html
[`riscv`]: https://www.kernel.org/doc/html/latest/arch/riscv/vm-layout.html
[`s390`]: https://www.kernel.org/doc/html/latest/arch/s390/index.html
[`powerpc`]: https://www.kernel.org/doc/html/latest/arch/powerpc/index.html
[`loongarch`]: https://www.kernel.org/doc/html/latest/arch/loongarch/index.html
[`mips`]: https://www.kernel.org/doc/html/latest/arch/mips/index.html
