(define-insn "ret"
	[(kInsn_Return)]
	"bx lr")
 
(define-insn "add_r32i32"
	[(kInsn_IAdd
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "int")
		(match_operand:i32 2 "register"))]
	"add {2}, {0}, #{1}")

(define-insn "add_r32r32"
	[(kInsn_IAdd
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register")
		(match_operand:i32 2 "register"))]
	"add {2}, {0}, {1}")

(define-insn "sub_r32i32"
	[(kInsn_ISub
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "int")
		(match_operand:i32 2 "register"))]
	"sub {2}, {0}, #{1}")

(define-insn "inttoptr"
	[(kInsn_IntToPtr
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"mov {1}, {0}")

(define-insn "br"
	[(kInsn_UnconditionalBranch
		(match_operand:lbl 0 "basic_block"))]
	"b {0}")

(define-insn "load_gptr"
	[(kInsn_Load
		(match_operand:ptr 0 "global")
		(match_operand:i32 1 "register"))]
	"ldr {1}, {0}")

(define-insn "load_r32"
	[(kInsn_Load
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"ldr {1}, [{0}]")

(define-insn "store"
	[(kInsn_Store
		(match_operand:i32 0 "register")
		(match_operand:i32 1 "register"))]
	"str {0}, [{1}]")

(define-insn "mov_i32"
	[(kInsn_Or
		(const_int:i32 0)
		(match_operand:i32 0 "int")
		(match_operand:i32 1 "register"))]
	"mov {1}, #{0}")
