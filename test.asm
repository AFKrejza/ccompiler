; nasm -felf64 test.asm && ld test.o && ./a.out

; https://cs.lmu.edu/~ray/notes/nasmtutorial/
; includes sys write

	global _start

	section .text

_start:

	mov rax, 3
	imul rax, 4
	imul rax, 2
	add rax, 2
	mov rdi, rax
	
	mov rax, 60	
	; mov rdi, 50
	syscall














; nasm -felf64 test.asm && ld test.o && ./a.out

; https://cs.lmu.edu/~ray/notes/nasmtutorial/
; includes sys write

; 	global _start

; 	section .text

; _start:
; 	mov rax, 60		; exit
; 	mov rdi, 50		; exit code
; 	syscall
