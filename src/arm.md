; -*- Clojure -*-
;
; @file arm.md
; @author Barney Wilks
;
; This file defines various patterns for matching IR instructions
; against the equivilant/target ARM assembly.

; *****************************************************************************
;                             Binary Operations
; *****************************************************************************

; ******************************
;      Register/Immediate
; ******************************

; 32 bit Register/Immediate Addition
;
; #FIXME: This presumes that the immediate integral value
;         can fit/is suitible to use as an immediate operand to
;         add. We might need additional logic to split the immediate
;         if required?
(define-insn "add_r32i32"
	[(kInsn_IAdd
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "int")
		(match_operand:i32 2 "register"))]
	"add {2}, {0}, #{1}")

; 32 bit Register/Immediate Subtraction
;
; #FIXME: Same problem with assuming that the constant int is suitable
;         to use as an immediate here. See comment on "add_r32i32" for
;         more details.
(define-insn "sub_r32i32"
	[(kInsn_ISub
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "int")
		(match_operand:i32 2 "register"))]
	"sub {2}, {0}, #{1}")

; ******************************
;      Register/Register
; ******************************

; 32 bit Register/Register Addition
(define-insn "add_r32r32"
	[(kInsn_IAdd
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"add {2}, {0}, {1}")

; 32 bit Register/Register Signed Division
(define-insn "div_r32r32"
	[(kInsn_IDiv
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"sdiv {2}, {0}, {1}")

; 32 bit Register/Register Subtraction
(define-insn "sub_r32r32"
	[(kInsn_ISub
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"sub {2}, {0}, {1}")

; 32 bit Register/Register Multiplication
(define-insn "mul_r32r32"
	[(kInsn_IMul
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"mul {2}, {0}, {1}")

; 32 bit Register/Register Bitwise Or
(define-insn "or_r32r32"
	[(kInsn_Or
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"orr {2}, {0}, {1}")

; 32 bit Register/Register Bitwise And
(define-insn "and_r32r32"
	[(kInsn_And
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"and {2}, {0}, {1}")

; 32 bit Register/Register Bitwise Exclusive Or
(define-insn "xor_r32r32"
	[(kInsn_Xor
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"eor {2}, {0}, {1}")

; *****************************************************************************
;                             Comparison Operations
; These could probably be handled better in the IR, without having to expand
; to multiple MC instructions here.
; *****************************************************************************

(define-insn "icmpeq_r32r32r32"
	[(kInsn_ICmp_Eq
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"cmp {0}, {1}"
	"mov {2}, #0"
	"movweq {2}, #1")

(define-insn "icmpneq_r32r32r32"
	[(kInsn_ICmp_Neq
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"cmp {0}, {1}"
	"mov {2}, #0"
	"movwne {2}, #1")

(define-insn "icmplt_r32r32r32"
	[(kInsn_ICmp_Lt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"cmp {0}, {1}"
	"mov {2}, #0"
	"movwlt {2}, #1")

(define-insn "icmplte_r32r32r32"
	[(kInsn_ICmp_Lte
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"cmp {0}, {1}"
	"mov {2}, #0"
	"movwle {2}, #1")	

(define-insn "icmpgt_r32r32r32"
	[(kInsn_ICmp_Gt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"cmp {0}, {1}"
	"mov {2}, #0"
	"movwgt {2}, #1")

(define-insn "icmpgte_r32r32r32"
	[(kInsn_ICmp_Gte
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"cmp {0}, {1}"
	"mov {2}, #0"
	"movwge {2}, #1")	

; *****************************************************************************
;                             Memory Operations
; *****************************************************************************

;
; The movw/movt pair is what GCC likes to emit, and clang sometimes (clang also sometimes
; defines another global/label that just contains the address
; and uses ldr to load the address from that. Choosing to do it this way for now, since
; it can be done entirely here & doesn't require extra support in the code generator to
; emit extra labels.
;
; Example:
;
;   > main:
;   >    ldr r0, .LC0
;   >    bx lr
;   >
;   > .LC0:
;   >     .long .LC1
;   >
;   > .LC1:
;   >     .asciz "Hello World!"

; Load a 32 bit global variable into a register
(define-insn "load_gptr"
	[(kInsn_Load
		(match_operand:ptr 0 "global")
		(match_operand:i32 1 "register"))]
	"movw {1}, :lower16:{0}"
	"movt {1}, :upper16:{0}"
	"ldr {1}, [{1}]")

; Load a 32 bit value pointed at by a register into another register
(define-insn "load_r32"
	[(kInsn_Load
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"ldr {1}, [{0}]")

; Store a 32 bit value into the address specified by another register
(define-insn "store"
	[(kInsn_Store
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"str {0}, [{1}]")

; Store the pointer of a global variable into the memory address
; given by a register (note that it doesn't load the value pointed to
; by the global, but the pointer of the global itself!)
(define-insn "store_gr32"
	[(kInsn_Store
		(match_operand:ptr 0 "global")
		(match_operand:i32 1 "register"))]
	"movw r7, :lower16:{0}"
	"movt r7, :upper16:{0}"
	"str r7, [{1}]")

(define-insn "store_r32g"
	[(kInsn_Store
		(match_operand:i32 0 "register")
		(match_operand:ptr 1 "global"))]
	"movw r7, :lower16:{1}"
	"movt r7, :upper16:{1}"
	"str {0}, [r7]")

; *****************************************************************************
;                             Branching Operations
; *****************************************************************************

; Unconditional branch to a internal basic block/label.
(define-insn "br"
	[(kInsn_UnconditionalBranch
		(match_operand:lbl 0 "basic_block"))]
	"b {0}")

(define-insn "cbr"
	[(kInsn_ConditionalBranch
		(match_operand:lbl 0 "basic_block")
		(match_operand:lbl 1 "basic_block")
		(match_operand:i32 2 "register")
		)]
	"cmp {2}, #1"
	"bge {0}"
	"b   {1}")

; 'ret' here implements the function prologue
; This acts as a return from the current instruction, since
; in the prologue we pushed onto the stack the link register (LR)
; which is the address to which the function should return.
; Here we just pop the link register directly into the program counter (PC)
; instead of going through a branch instruction
(define-insn "ret"
	[(kInsn_Return)]
	"@pop {r11, pc}")

; *****************************************************************************
;                             Cast Operations
; *****************************************************************************

; 32 bit integer -> 32 bit pointer conversion
;
; #FIXME: This is actually a noop if both the pointer
;         and register are 32 bits (different story if the integer
;         is not the same as the native pointer size).
;         Ideally this should be removed by an earlier pass in the compiler
;         and should never get this far.
(define-insn "inttoptr"
	[(kInsn_IntToPtr
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"mov {1}, {0}")

(define-insn "ptrtoint"
	[(kInsn_PtrToInt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"mov {1}, {0}")
