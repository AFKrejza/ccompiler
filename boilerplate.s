.intel_syntax noprefix
.global main

.text
main:
	push    rbp
	mov     rbp, rsp
	mov     eax, 5
	pop     rbp
	ret
	
.section .note.GNU-stack,"",@progbits
