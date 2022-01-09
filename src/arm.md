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
(define-insn "sdiv_r32r32"
	[(kInsn_ISDiv
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"sdiv {2}, {0}, {1}")

(define-insn "udiv_r32r32"
	[(kInsn_IUDiv
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"udiv {2}, {0}, {1}")

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

(define-insn "cmp"  [] "cmp {0}, {1}")
(define-insn "cmpi" [] "cmp {0}, #{1}")

(define-insn "$icmpeq"
	[(kInsn_ICmp_Eq
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmpneq"
	[(kInsn_ICmp_Neq
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmplt"
	[(kInsn_ICmp_Lt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmplte"
	[(kInsn_ICmp_Lte
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmpgt"
	[(kInsn_ICmp_Gt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmpgte"
	[(kInsn_ICmp_Gte
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

; *****************************************************************************
;                             Memory Operations
; *****************************************************************************

(define-insn "ldr" [] "ldr {0}, [{1}]")

(define-insn "$ldr_g32"
	[(kInsn_Load
		(match_operand:ptr 0 "global")
		(match_operand:i32 1 "register"))]
	"*expand_load")

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

; Load a 32 bit value pointed at by a register into another register
(define-insn "ldrw_r32"
	[(kInsn_Load
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"ldr {1}, [{0}]")

; Load a 16 bit unsigned value pointed at by a register into another register
(define-insn "ldrh_r32"
	[(kInsn_Load
		(match_operand:i32 0 "register")
		(match_operand:i16 1 "register"))]
	"ldrh {1}, [{0}]")

; Load a 32 bit value pointed at by a register into another register
(define-insn "ldrb_r32"
	[(kInsn_Load
		(match_operand:i32 0 "register")
		(match_operand:i8 1 "register"))]
	"ldrb {1}, [{0}]")

; Store a 32 bit value into the address specified by another register
(define-insn "strw"
	[(kInsn_Store
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"str {0}, [{1}]")

; Store a 8 bit value into the address specified by another register
(define-insn "strb"
	[(kInsn_Store
		(match_operand:i8 0 "register")
		(match_operand:i32 1 "register"))]
	"strb {0}, [{1}]")

; Store a 16 bit value into the address specified by another register
(define-insn "strh"
	[(kInsn_Store
		(match_operand:i16 0 "register")
		(match_operand:i32 1 "register"))]
	"strh {0}, [{1}]")

; *****************************************************************************
;                             Branching Operations
; *****************************************************************************

(define-insn "bge" [] "bge {0}")

(define-insn "$cbr"
	[(kInsn_ConditionalBranch
		(match_operand:lbl 0 "basic_block")
		(match_operand:lbl 1 "basic_block")
		(match_operand:i32 2 "register"))]
	"*expand_conditional_branch")

; Unconditional branch to a internal basic block/label.
(define-insn "br"
	[(kInsn_UnconditionalBranch
		(match_operand:lbl 0 "basic_block"))]
	"b {0}")

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

; *****************************************************************************
;                             Move Operations
; *****************************************************************************

(define-insn "movi" [] "mov {0}, #{1}")
(define-insn "mov" [] "mov {0}, {1}")

(define-insn "movweqi" [] "movweq {0}, #{1}")
(define-insn "movwnei" [] "movwne {0}, #{1}")
(define-insn "movwlti" [] "movwlt {0}, #{1}")
(define-insn "movwlei" [] "movwle {0}, #{1}")
(define-insn "movwgti" [] "movwgt {0}, #{1}")
(define-insn "movwgei" [] "movwge {0}, #{1}")

(define-insn "movweq" [] "movweq {0}, {1}")
(define-insn "movwne" [] "movwne {0}, {1}")
(define-insn "movwlt" [] "movwlt {0}, {1}")
(define-insn "movwle" [] "movwle {0}, {1}")
(define-insn "movwgt" [] "movwgt {0}, {1}")
(define-insn "movwge" [] "movwge {0}, {1}")

(define-insn "movw_gl16" [] "movw {0}, :lower16:{1}")
(define-insn "movt_gu16" [] "movt {0}, :upper16:{1}")