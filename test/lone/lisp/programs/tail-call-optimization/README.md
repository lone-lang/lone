# Tail call optimization regression tests

Each test recurses a fixed number of times. With tail call optimization
the machine reuses stack frame and the recursion runs in constant space.
Without it the stack grows until it overflows. Regressions in this
mechanism bring the stack overflows back.

The iteration count was determined empirically. A non-TCO recursion
overflows the machine stack at around 11000 iterations: 65536 frames
maximum, roughly six frames per call. Picking 20000 results in a nice
safety margin that's well past that point yet still executes quickly.
This count must be revisited if the frames per call change.

The two return-from-tail-call tests in modules/intrinsic/lone/return/
use the same count for the same reason.
