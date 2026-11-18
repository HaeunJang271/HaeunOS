# R4.2 Kernel Integrity

Compared to R4.1 / R4.0 research baseline SHA-256 (working tree).

| File | Baseline | Current | Match |
|------|----------|---------|-------|
| proc_sched.s | f378293d24ef74fe9af42615959e40c5c1de038d49a38590adfa562bf200d7d7 | same | YES |
| proc_policy.s | 38a79c49d2f9f9d72589f2c13e63f96c1b279bb051e02ac8bb1328f7d81299de | same | YES |
| proc_trap.s | e6cb2a9823a5e6a4d3bcba1322fcaad4e4732fb3138972df7cef672ebe3bc020 | same | YES |
| proc_telem.s | 2d7dbeacdb4326838487a580f946de660a39c141c408e6767d529a89c06a705a | same | YES |
| proc_fork.s | d895e560f52a33610fb9f63271d854bb38d7941422bc9a4eb093f57ed60ca603 | same | YES |

**VERIFIED:** No R4.2 kernel edits.  
Note: files may still differ from last **git commit** (uncommitted prior research); integrity gate is the R4.1 hash baseline.
