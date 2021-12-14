; Hello World
(define-prologue
	"push {r11, lr}"
	"mov r11, sp")

(define-epilogue
	"mov sp, r11"
	"pop {r11,lr}")


; Instructions


(define-insn "ret"
	[(kInsn_Return)]
	"bx lr")

(define-insn "mov_i32"
	[(kInsn_Or
		(const_int:i32 0)
		(match_operand:i32 0 "int")
		(match_operand:i32 1 "register"))]
	"mov {1}, #{0}")
