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

; #FIXME(bwilks):
;
;    Note how the read/write flags for binary operations is often done in
;    the order:
;        (0 read) (1 read) (2 write)
;    when in actual fact the ARM instructions operands are in the order
;        write, read, read.
;
;   we have to do it this way due to a percularity in the generator where
;   the operands are added into the LLIR instruction in the same order as
;   the HLIR and doesn't take into account reordering that might be in the
;   assembly string (e.g. ARM add {2}, {0}, {1}).
;
;   This is _really_ confusing and probably going to cause more issues down the
;   line so heed this & hopefully fix this at some point.

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
	[(HLIR::IAdd
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "int")
		(match_operand:i32 2 "register"))]
	"add {2}, {0}, #{1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Immediate Subtraction
;
; #FIXME: Same problem with assuming that the constant int is suitable
;         to use as an immediate here. See comment on "add_r32i32" for
;         more details.
(define-insn "sub_r32i32"
	[(HLIR::ISub
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "int")
		(match_operand:i32 2 "register"))]
	"sub {2}, {0}, #{1}"
	[(0 read) (1 read) (2 write)])

; ******************************
;      Register/Register
; ******************************

; 32 bit Register/Register Addition
(define-insn "add_r32r32"
	[(HLIR::IAdd
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"add {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Register Signed Division
(define-insn "sdiv_r32r32"
	[(HLIR::ISDiv
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"sdiv {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

(define-insn "udiv_r32r32"
	[(HLIR::IUDiv
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"udiv {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Register Subtraction
(define-insn "sub_r32r32"
	[(HLIR::ISub
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"sub {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Register Multiplication
(define-insn "mul_r32r32"
	[(HLIR::IMul
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"mul {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Register Bitwise Or
(define-insn "or_r32r32"
	[(HLIR::Or
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"orr {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Register Bitwise And
(define-insn "and_r32r32"
	[(HLIR::And
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"and {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; 32 bit Register/Register Bitwise Exclusive Or
(define-insn "xor_r32r32"
	[(HLIR::Xor
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"eor {2}, {0}, {1}"
	[(0 read) (1 read) (2 write)])

; *****************************************************************************
;                             Comparison Operations
; *****************************************************************************

(define-insn "cmp"  [] "cmp {0}, {1}"  [(0 read) (1 read)])
(define-insn "cmpi" [] "cmp {0}, #{1}" [(0 read) (1 read)])

(define-insn "$icmpeq"
	[(HLIR::ICmp_Eq
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmpneq"
	[(HLIR::ICmp_Neq
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmplt"
	[(HLIR::ICmp_Lt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmplte"
	[(HLIR::ICmp_Lte
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmpgt"
	[(HLIR::ICmp_Gt
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

(define-insn "$icmpgte"
	[(HLIR::ICmp_Gte
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"*expand_icmp")

; *****************************************************************************
;                             Memory Operations
; *****************************************************************************

(define-insn "ldr"    [] "ldr {0}, [{1}]"         [(0 write) (1 read)])   ; Load (32 bits)

(define-insn "ldrsh"  [] "ldrsh {0}, [{1}]"       [(0 write) (1 read)]) ; Load & Sign Extend (16 bits)
(define-insn "ldrh"   [] "ldrh {0}, [{1}]"        [(0 write) (1 read)])  ; Load & Zero Extend (16 bits)

(define-insn "ldrb"   [] "ldrb {0}, [{1}]"        [(0 write) (1 read)])  ; Load & Zero Extend (8 bits)
(define-insn "ldrsb"  [] "ldrsb {0}, [{1}]"       [(0 write) (1 read)]) ; Load & Sign Extend (8 bits)

(define-insn "str"    [] "str {0}, [{1}]"         [(0 read) (1 read)])   ; Store (32 bit)
(define-insn "strh"   [] "strh {0}, [{1}]"        [(0 read) (1 read)])  ; Store (16 bit)
(define-insn "strb"   [] "strb {0}, [{1}]"        [(0 read) (1 read)])  ; Store (16 bit)

(define-insn "ldri"   [] "ldr {0}, [{1}, #{2}]"   [(0 write) (1 read) (2 read)]) ; Load (Word, Immediate Offset)
(define-insn "ldrbi"  [] "ldrb {0}, [{1}, #{2}]"  [(0 write) (1 read) (2 read)]) ; Load (Byte, Immediate Offset)
(define-insn "ldrhi"  [] "ldrh {0}, [{1}, #{2}]"  [(0 write) (1 read) (2 read)]) ; Load (Halfword, Immediate Offset)

(define-insn "ldrsbi" [] "ldrsb {0}, [{1}, #{2}]" [(0 write) (1 read) (2 read)]) ; Load & Sign Extend (Byte, Immediate Offset)
(define-insn "ldrshi" [] "ldrsh {0}, [{1}, #{2}]" [(0 write) (1 read) (2 read)]) ; Load & Sign Extend (Halfword, Immediate Offset)

(define-insn "stri"   [] "str {0}, [{1}, #{2}]"   [(0 read) (1 read) (2 read)]) ; Store (Word, Immediate Offset)
(define-insn "strbi"  [] "strb {0}, [{1}, #{2}]"  [(0 read) (1 read) (2 read)]) ; Store (Byte, Immediate Offset)
(define-insn "strhi"  [] "strh {0}, [{1}, #{2}]"  [(0 read) (1 read) (2 read)]) ; Store (Halfword, Immediate Offset)

; ******************************
;      Store (To Memory)
; ******************************

; Store the address of a global variable (operand 0) to the memory address
; specified in the destination register (operand 1).
;
; This is a bit of a special case & is a bit funky, sorry about that :-(
(define-insn "$store_global_address_to_memory"
	[(HLIR::Store
		(match_operand:ptr 0 "global")
		(match_operand:ptr 1 "register"))]
	"*expand_store")

; Store 32 bit value in the source register (operand 0) into
; a global variable (operand 1).
(define-insn "$store_global"
	[(HLIR::Store
		(match_operand:i32 0 "register")
		(match_operand:ptr 1 "global"))]
	"*expand_store")

; Store the 8/16/32 bit value from the source register (operand 0) to
; the memory address specified in the destination register (operand 1).
(define-insn "$store_register"
	[(HLIR::Store
		(match_operand:*   0 "register")
		(match_operand:ptr 1 "register"))]
	"*expand_store")

; ******************************
;      Load (From Memory)
; ******************************

; Loading a 8/16/32 bit value from a global variable (operand 0) into the destination
; register (operand 1).
(define-insn "$load_global"
	[(HLIR::Load
		(match_operand:ptr 0 "global")
		(match_operand:*   1 "register"))]
	"*expand_load")

; Load the 8/16/32 bit value stored at the address given in the source
; register (operand 0) to the destination register (operand 1).
(define-insn "$load_register"
	[(HLIR::Load
		(match_operand:ptr 0 "register")
		(match_operand:*   1 "register"))]
	"*expand_load")

; *****************************************************************************
;                             Branching Operations
; *****************************************************************************

(define-insn "bge" [] "bge {0}" [(0 read)])
(define-insn "bgt" [] "bgt {0}" [(0 read)])

(define-insn "ble" [] "ble {0}" [(0 read)])
(define-insn "blt" [] "blt {0}" [(0 read)])

(define-insn "bne" [] "bne {0}" [(0 read)])
(define-insn "beq" [] "beq {0}" [(0 read)])

(define-insn "bl"  [] "bl {0}" [(0 read)])

(define-insn "$cbr"
	[(HLIR::ConditionalBranch
		(match_operand:lbl 0 "basic_block")
		(match_operand:lbl 1 "basic_block")
		(match_operand:i32 2 "register"))]
	"*expand_conditional_branch")

; Unconditional branch to a internal basic block/label.
(define-insn "br"
	[(HLIR::UnconditionalBranch
		(match_operand:lbl 0 "basic_block"))]
	"b {0}"
	[(0 read)])

; 'ret' here implements the function prologue
; This acts as a return from the current instruction, since
; in the prologue we pushed onto the stack the link register (LR)
; which is the address to which the function should return.
; Here we just pop the link register directly into the program counter (PC)
; instead of going through a branch instruction
(define-insn "ret"
	[(HLIR::Return)]
	"@mov sp, r11
	pop {r4, r5, r6, r7, r8, r10, r11, lr}
	bx lr"
	[])

(define-insn "$call_void"
	[(HLIR::Call
		(match_operand:void 0 "any")
		(match_operand:*    1 "function"))]
	"*expand_void_call")

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
;
;        NB: See note on "Binary Instructions" on why the read/write flags
;            are the wrong way round (e.g. don't represent the ARM operands
;            but the order of operands in the IR)
(define-insn "inttoptr"
	[(HLIR::IntToPtr
		(match_operand:i32 0 "register")
		(match_operand:ptr 1 "register"))]
	"mov {1}, {0}"
	[(0 read) (1 write)])

(define-insn "ptrtoint"
	[(HLIR::PtrToInt
		(match_operand:ptr 0 "register")
		(match_operand:i32 1 "register"))]
	"mov {1}, {0}"
	[(0 read) (1 write)])

(define-insn "$ptrtoint"
	[(HLIR::PtrToInt
		(match_operand:ptr 0 "global")
		(match_operand:i32 1 "register"))]
	"*expand_global_address_to_register")

(define-insn "sxth"
	[(HLIR::SExt
		(match_operand:i16 0 "register")
		(match_operand:i32 1 "register"))]
	"sxth {1}, {0}"
	[(0 read) (1 write)])

(define-insn "sxtb"
	[(HLIR::SExt
		(match_operand:i8  0 "register")
		(match_operand:i32 1 "register"))]
	"sxtb {1}, {0}"
	[(0 read) (1 write)])

(define-insn "uxth"
	[(HLIR::ZExt
		(match_operand:i16 0 "register")
		(match_operand:i32 1 "register"))]
	"uxth {1}, {0}"
	[(0 read) (1 write)])

(define-insn "uxtb"
	[(HLIR::ZExt
		(match_operand:i8  0 "register")
		(match_operand:i32 1 "register"))]
	"uxtb {1}, {0}"
	[(0 read) (1 write)])

; *****************************************************************************
;                             Move Operations
; *****************************************************************************

(define-insn "set"
	[(HLIR::Set
		(match_operand:* 0 "register")
		(match_operand:* 1 "register"))]
	"mov {0}, {1}"
	[(0 write) (1 read)])

(define-insn "$set-address"
	[(HLIR::Set
		(match_operand:ptr 0 "register")
		(match_operand:ptr 1 "global"))]
	"*expand_global_address_to_register")

(define-insn "movi" [] "mov {0}, #{1}" [(0 write) (1 read)])
(define-insn "mov"  [] "mov {0}, {1}"  [(0 write) (1 read)])

(define-insn "movweqi" [] "movweq {0}, #{1}" [(0 write) (1 read)])
(define-insn "movwnei" [] "movwne {0}, #{1}" [(0 write) (1 read)])
(define-insn "movwlti" [] "movwlt {0}, #{1}" [(0 write) (1 read)])
(define-insn "movwlei" [] "movwle {0}, #{1}" [(0 write) (1 read)])
(define-insn "movwgti" [] "movwgt {0}, #{1}" [(0 write) (1 read)])
(define-insn "movwgei" [] "movwge {0}, #{1}" [(0 write) (1 read)])

(define-insn "movweq" [] "movweq {0}, {1}"   [(0 write) (1 read)])
(define-insn "movwne" [] "movwne {0}, {1}"   [(0 write) (1 read)])
(define-insn "movwlt" [] "movwlt {0}, {1}"   [(0 write) (1 read)])
(define-insn "movwle" [] "movwle {0}, {1}"   [(0 write) (1 read)])
(define-insn "movwgt" [] "movwgt {0}, {1}"   [(0 write) (1 read)])
(define-insn "movwge" [] "movwge {0}, {1}"   [(0 write) (1 read)])

(define-insn "movw_gl16" [] "movw {0}, :lower16:{1}" [(0 write) (1 read)])
(define-insn "movt_gu16" [] "movt {0}, :upper16:{1}" [(0 write) (1 read)])
(define-insn "movwi"     [] "movw {0}, #{1}" [(0 write) (1 read)])
(define-insn "movti"     [] "movt {0}, #{1}" [(0 write) (1 read)])

