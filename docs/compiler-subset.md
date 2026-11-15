# Initial C17 compiler subset

`tools/haeun_toolchain.py cc` compiles a small, explicitly limited C17 subset into HOBJ v4 (ISA v3). It is a tested compiler prototype, not a general C compiler.

## Accepted program shape

A translation unit contains zero or more `extern int name(...);` declarations and one or more definitions of the form `int name(int args...) { ... }`. An executable unit defines exactly one of `int main(void)` (emits `_start`) or `int kmain(void)` (for linking with an assembly `_start`). A library unit may omit both. Helpers accept up to 64 `int` arguments. `cc --isa N` selects the emitted `.isa` directive (default 3). Initialized local declarations (`int name = expression;`) come first in each function; parameters plus locals are limited to 128 slots. The first six parameter slots use registers and remaining parameters and locals use stack homes. The rest of a body may contain assignments, nested blocks, `if`/`else`, and `while`, followed by one final top-level `return expression;`. Function calls are permitted in expressions and may be recursive. Calls inside another call's argument expressions are rejected for now. `main` is an entry point and cannot be called.

Locals cannot be declared inside nested blocks and cannot be read before declaration/initialization. Prototypes, external functions, global variables, pointers, arrays, structs, uninitialized declarations, and returns inside control-flow blocks are rejected.

Expressions contain signed decimal/hex integer literals, local names, function calls, parentheses, unary `+`/`-`, binary `+`, `-`, `&`, `^`, `|`, equality (`==`, `!=`), and signed relational comparisons (`<`, `<=`, `>`, `>=`) with C precedence. Comparisons produce canonical integer 0 or 1 through ISA v3 `CMP`. Constant signed-32 overflow is rejected. Runtime arithmetic uses HAEUN modulo-2^32 integer instructions; signed overflow in source has C undefined behavior. Non-folded constants must fit the signed 18-bit ADDI immediate. Multiplication, division, shifts, and casts are unsupported.

## Tested calling convention subset

- `r0` is zero; `r1` carries integer return values.
- The first six integer parameters use `r2–r7`; later arguments are passed in a caller-allocated, 16-byte-rounded stack area. The callee copies stack arguments into its local stack homes during entry.
- `r8–r11` are callee-saved temporaries. Every helper saves them and `r15` in its stack frame.
- `r13` is a 16-byte-aligned downward-growing stack pointer. The VM CLI initializes it to the 256 MiB RAM limit; the top 16 MiB is reserved for stack. `r14` contains the inclusive stack lower bound `0x0FF00000`.
- Direct calls use `jal r15, function`; helpers return with `ret` (`jalr r0,r15,0`). `main` places its result in `r1` and halts.
- Each function uses a dynamically sized, 16-byte-aligned frame: `align16(80 + 4*local_spill_slots + 4*max_outgoing_stack_args + 4*temporary_spill_slots)`. Offsets 0–12 save `r8–r11`, 16 saves `r15`, 20–40 save caller `r2–r7`, 44–64 stage the first six outgoing arguments, 68 stages the return value, and 72–79 are reserved padding. Local spill homes start at offset 80; outgoing stack-argument staging follows local homes, then expression-temporary spill slots. On a call with more than six arguments, the caller temporarily allocates a 16-byte-rounded outgoing area below its frame; the callee sees stack argument zero at its entry stack pointer before its own frame allocation.

This ABI path is exercised by 6- and 8-argument calls, stack-passed parameters, stack-homed locals beyond the six argument registers, deep expressions that spill and reload temporaries, preserved caller locals, recursion, and VM execution. Expression evaluation uses four physical temporary registers and spills additional live values into frame slots. Generated C code checks every prospective function-frame and outgoing stack-argument allocation against the inclusive lower bound in `r14`; overflow stops through ECALL. This does not constrain arbitrary assembly or ordinary guest loads/stores, and there is no VM-enforced guard page. The full kernel calling convention remains provisional.

## Example

```c
int add(int left, int right) {
    return left + right;
}

int main(void) {
    return add(20, 22);
}
```

Compile and execute with:

```sh
python tools/haeun_toolchain.py cc examples/return_value.c build/return.hobj
python tools/haeun_toolchain.py ld -o build/return.hxf build/return.hobj
build/haeun-vm build/return.hxf
```

The VM CLI reports the result in `r1`. Unsupported syntax, argument-count mismatches, or unrepresentable immediates are compile/link errors; they are never silently dropped.
