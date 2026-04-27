# AT_PHDR invariant

`lone-embed` may relocate the program headers table
when the input binary lacks enough `PT_NULL` slots
to absorb the new segments. The relocated segments
table must remain reachable via the kernel's `AT_PHDR`
auxiliary vector entry, which the interpreter walks
at startup to find its embedded segment.

## The kernel's AT_PHDR computation

Two formulas exist, both of which patched binaries must work under:

  1. Linux < 5.18

     `AT_PHDR` is computed unconditionally as:

         AT_PHDR = (first_LOAD.p_vaddr - first_LOAD.p_offset) + e_phoff

     Where `first_LOAD` is the first `PT_LOAD` in table order.
     `PT_PHDR` is ignored entirely.

  2. Linux ≥ 5.18

     When `PT_PHDR` is present, `AT_PHDR` is read
     from `PT_PHDR.p_vaddr` directly. The formula
     above is the fallback when `PT_PHDR` is absent.

The patched lone binary must work on both versions.
The formula is the strict superset.

## The invariant

`load_address` is `first_LOAD.p_vaddr - first_LOAD.p_offset`.
`e_phoff` is the patched program header table's file offset.

The new `PT_LOAD` that maps the relocated segments table satisfies:

    new_LOAD.p_offset = e_phoff
    new_LOAD.p_vaddr  = load_address + e_phoff

`AT_PHDR` resolves to `new_LOAD.p_vaddr` under both kernel formulas.

## Page alignment of `load_address`

The ELF specification requires `p_vaddr ≡ p_offset (mod p_align)`
for `PT_LOAD` segments. Linkers set `p_align` to the page size,
so `load_address` is page-aligned by construction.

`adjust_phdr_entry` asserts this before patching.
Malformed binaries abort with
`LONE_TOOLS_EMBED_EXIT_LOAD_ADDRESS_UNALIGNED`.

## Non-overlap with existing LOADs

`load_address + e_phoff` may collide with an existing
`PT_LOAD`'s virtual range. `aarch64` layouts where
`limits.end.virtual > limits.end.file` exhibit this,
observed with lld and mold.

`adjust_phdr_entry` bumps `e_phoff` past the collision:

    required = align_to_page(limits.end.virtual - load_address)
    e_phoff  = max(segments.file_offset, required)

`load_address + required` equals
`align_to_page(limits.end.virtual)`,
past every existing `PT_LOAD`.
Either arm of the `max`
satisfies the invariant.

The bump may introduce a zero-filled sparse hole in the file.

## Enforcement

The invariant is enforced by assertions in `lone-embed`
as well as actual test cases in the test suite.

## References

  - Linux kernel commit
    [`0da1d5002745cdc721bc018b582a8a9704d56c42`](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=0da1d5002745cdc721bc018b582a8a9704d56c42)
    fixes the `AT_PHDR` computation by honoring `PT_PHDR`.
  - [Bugzilla 197921](https://bugzilla.kernel.org/show_bug.cgi?id=197921)
    is the original report.
  - [LKML thread](https://lore.kernel.org/r/20220127124014.338760-2-akirakawata1@gmail.com)
    is the upstream patch discussion.
