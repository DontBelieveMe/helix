# [Transform][ARM] Constant Hoisting

## Abstract

`MOV` in the ARMv7-A architecture only supports 8 bit immediate operands
(in Thumb mode) or 12 bit (in AArch32 mode).
Because of this we can't rely on expanding constant integers in the IR to a
generic `mov r?, #<constant>`.

Instead there are multiple ways you can get an constant integer into a
register:
 - Split it into instructions that each load different parts of the integer
   into the register via offsets and shifting.
 - Place the integer in a constant pool and load it into the register with
   `ldr`. 

As an initial implementation, I think implementing loading immediates as always
loading from memory makes sense - the approach is simpler is consistent for
all immediate values, wheras choosing how to split the load into multiple
instructions takes a bit more work.

This can always be improved on later by taking a bit more of a heuristic
approach to deciding which to do (something that can probably be
mentioned in the report).

## Algorithm

Iterate every reference to a `ConstantInt` in the module. For each one, check
if a global variable has been created for this integer, if it has then use that,
if it hasn't then create a constant int with the init value of this `ConstantInt`
(then cache the global variable so a new one won't be created for the next
instance of this constant int). This global variable will be known as `G`.

Create a new virtual register with the type of the constant integer (`V`),
and inject a load (before the instruction that uses the constant int) that loads
the value from global `G` into the new virtual register `V`.

Replace the constant int reference operand in the current instruction value
with a reference to the vreg `V`.

## Examples

Given this C:
```c
int c() {
	return 12345678;
}
```

Something like this IR will get emitted by the frontend:

```
function c(): i32 {
	ret 12345678
}
```

instead given this transform, it should look something like:

```
@c0:ptr = global i32 12345678

function c(): i32 {
	load @c0:ptr, %0:i32
	return %0:i32
}
```

## References

[Here](https://godbolt.org/z/3Trq5Gc3r) is a Godbolt link comparing the
approaches that clang and GCC use (at `-O3`), for a large 32 bit immediate
value.

(Given this C code)

```c
int a() {
    return 123456789;
}
```

### GCC
---
With optimisations enabled (`-O3`)

```arm
a:
  movw r0, #52501
  movt r0, 1883
  bx lr
```

Here GCC uses the `movw` and `movt` instructions (the first approach in the 
abstract).  
`movw` loads the lower 16 bits of the instruction and `movt` loads the upper 16
bits of the, basically forming the C bitwise OR operation `r0 = lower | upper`.

---

Without optimisations enabled (no `-O3`)

```arm
a:
  push {r7}
  add r7, sp, #0
  movw r3, #52501
  movt r3, 1883
  mov r0, r3
  mov sp, r7
  ldr r7, [sp], #4
  bx lr
```

Here the actual code that loads the constant into a register is the same
(a `movw`/`movt` pair), but there is a lot of unnessary boilerplate code
around it that the optimiser obviously removes later (e.g. the constant
actually gets loaded into `r3` but has to be copied to `r0` to be the return
value. The rest of the code is the function epilogue/prologue setting up
and then cleaning up the stack, as per AAPCS32).

---

### Clang

Same output is produced regardless of optimisation flags (e.g. `-O3`/no `-O3`)

```arm
a:
  ldr r0, .LCPI0_0
  bx lr
.LCPI0_0:
  .long 123456789 @ 0x75bcd15
```

Clang instead takes the second described approach here and loads the constant
from memory instead of encoding it in instructions.

---