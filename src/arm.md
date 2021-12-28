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


; *****************************************************************************
;                             Memory Operations
; *****************************************************************************

; Load a 32 bit global variable into a register
(define-insn "load_gptr"
	[(kInsn_Load
		(match_operand:ptr 0 "global")
		(match_operand:i32 1 "register"))]
	"ldr {1}, {0}")

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

; *****************************************************************************
;                             Branching Operations
; *****************************************************************************

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
