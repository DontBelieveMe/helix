# [Transform] Host Stack Allocations

## Abstract

The purpose of this transform is to hoist all `stack_alloc` instructions
to be at the top of the first basic block in the function (head).

This transform isn't stricly nessesary, since `stack_alloc `isn't
actually a instruction that gets executed, but is more of a meta
instruction that tells the compiler that we need to reserve some
space on the stack.

Hoisting these to the top has a few benefits:
 - Removes clutter that could be deceptive in the body of the
   function.
   - For example if there was a `stack_alloc` in a loop, it looks
     like there is an allocation every iteration of the loop
     and not just one

 - Assuming all `stack_allocs` are in the same place in a function
   simplifies some later analysis.
   - For example when the `stack_allocs` get lowered to code that
     actually reserves the space on the stack (e.g. `SP += <n>` etc...)
     it only has to look in the head basic block to determine how much
     space to allocate and not the traverse the whole function to find each one.

This should happen during the `genlegal` pass since its platform agnostic
and just a matter of cleaning up the IR.

## Algorithm

 - For functions with more than one basic block:
 - Find each `stack_alloc` not in the first BB and store it in a
   worklist.
 - Iterate each recorded stack allocation, and move it from its current
   location to the head BB.

## Examples

Given the following C code:

```c
void a() {
  if (0 > 3) {
    int c;
  } else {
    int d;
  }
}
```

the following IR was being created

```
function a(): void {
.0:
    icmp_gt 0:i32, 3:i32, %0:i32
    cbr .1, .2, %0:i32
.1:
    stack_alloc [i32 x 1], %1:ptr
    br .3
.2:
    stack_alloc [i32 x 1], %2:ptr
    br .3
.3:
    ret
}
```

Note the `stack_alloc` instructions inside the if statement.

The IR should look something like as follows, given this transform:

```
function a(): void {
.0:
    stack_alloc [i32 x 1], %0:ptr
    stack_alloc [i32 x 1], %1:ptr
    icmp_gt 0:i32, 3:i32, %2:i32
    cbr .1, .2, %2:i32
.1:
    br .3
.2:
    br .3
.3:
    ret
}
```

(ignore the fact that this function should just be a single `ret` instruction and not a
whole 10 lines that essentially do nothing... we'll get there!)

## References

Clang also does this - [see this example in Godbolt](https://godbolt.org/z/jenrfnzoe).