# [Transform] Combine Returns

## Abstract

The purpose of this transform is to simplify multiple return 
instructions in a function to just one return instruction.

This makes it easier for later passes to inject code that always needs
to be executed on function exit (such as stack cleanup) - it wouldn't
need to be duplicated for each instance of the return.

This occurs during a new `retcomb` (Return Combine) pass. If too much time is spent doing this
pass alone it could be merged into tho `genlower` pass, or even try to get the frontend
to emit this code in the first place.

## Algorithm

An extra stack variable (`V`) is allocated to store the return value.  
A new "tail" basic block is created and inserted at the end of the 
function (`.tail`).

Each existing return in the function gets replaced with a sequence
that stores the return value (if present) in `V` and then branches to
newly created `.tail` BB.

Finally in the `.tail` BB the value stored in `V` is loaded to a
virtual register (`R`) and a return instruction is inserted to return that value (`R`).

For void functions the principle is mostly the same, except the stack
variable to hold the return value (`V`) doesn't need to be created,
and as such extra loads/stores do not need to be inserted for the return
value.

Whilst this may seem to create superfluous BBs in some instances,
a later BB combine/flatten pass could solve that.

## Examples

Given the following C code that demonstrates multiple return points
```c
int a(int px) {
  if (px > 3) {
    return 10;
  }

  return 20;
}
```

The frontend naively emits the following IR:

```
1  | function a(%0:i32): i32 {
2  | .0:
3  |     stack_alloc [i32 x 1], %1:ptr
4  |     store %0:i32, %1:ptr
5  |     load %1:ptr, %2:i32
6  |     icmp_gt %2:i32, 3:i32, %3:i32
7  |     cbr .1, .2, %3:i32
8  | .1:
9  |     ret 10:i32
10 | .2:
11 |     ret 20:i32
12 | }
```

Note that if a later pass wanted to insert function cleanup code it
would have to insert the same code for each return (on lines 9 and 11).

Given this transform, that IR should be as follows:

```
1  | function a(%0:i32): i32 {
2  | .0:
3  |     stack_alloc [i32 x 1], %1:ptr
4  |     stack_alloc [i32 x 1], %2:ptr ;
5  |     store %0:i32, %1:ptr
6  |     load %1:ptr, %3:i32
7  |     icmp_gt %3:i32, 3:i32, %4:i32
8  |     cbr .1, .2, %4:i32
9  | .1:
10 |     store 10:i32, %2:ptr
11 |     br .3
12 | .2:
13 |     store 20:i32, %2:ptr
14 |     br .3
15 | .3:
16 |     load %2:ptr, %5:i32
17 |     ret %5:i32
18 | }
```

Note the new stack allocation for the return value (line 4) and
how the return instructions in blocks `.1` and `.2` have been
transformed to sequences that load the return value to memory
and branch to the tail block.

The tail block (`.3` starting on line 15) then loads that value
from memory and actually returns it.

## References

Clang appears to emit code in this format (with optimisations
disabled) - [Example in Godbolt](https://godbolt.org/z/eTb6qq5Kr).

Given the same C code as specified in the examples above, clang
generates the following code: (some comments & annotations have
been stripped for clarity, see the godbolt link above for
full output)

```llvm
define i32 @a(i32 %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  %4 = load i32, i32* %3, align 4
  %5 = icmp sgt i32 %4, 3
  br i1 %5, label %6, label %7

6:
  store i32 10, i32* %2, align 4
  br label %8

7:
  store i32 20, i32* %2, align 4
  br label %8

8:
  %9 = load i32, i32* %2, align 4
  ret i32 %9
}
```