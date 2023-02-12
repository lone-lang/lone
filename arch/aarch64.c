/**
 *
 * architecture:    aarch64
 * register-size:   64 bits
 * stack-alignment: 16 bytes
 * system-call:     x0 = "svc 0" [x8] x0 x1 x2 x3 x4 x5
 *
 **/

/**
 *
 * initial stack layout - logical
 *
 * sp → 0                         | argc
 *      1                         | argv
 *      argv + *argc + 1          | envp
 *      &(*envp++ == 0) + 1       | auxv
 *
 * initial stack layout - bytes
 *
 * sp → 0                         | argc
 *      8                         | argv
 *      argv + 8 * (*argc + 1)    | envp
 *      &(*envp++ == 0) + 8       | auxv
 *
 **/
