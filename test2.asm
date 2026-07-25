; debugging:
; nasm -felf64 -g -F dwarf main.asm && ld main.o
; gdb ./a.out
; in gdb: layout regs or layout asm
; set breakpoints e.g. break _start
; run
; stepi
; continue/c
; util/u, next/n, nexti/ni, return, finish/fin
; watch
; and so on

; run with:
; nasm -felf64 main.asm && ld main.o && ./a.out

; Very basic blackjack implementation in x86-64 assembly
; Dealer stands on 17
; Populates the cards array and then shuffles the cards into the deck array.
;
; this genuinely took a whole weekend :P

; send commands: stand = s, hit = h, replay = r
; if ace, set value to 11. if sum > 21, reduce by 10.
; once player stands, dealer draws until hits 17 or higher
; when replaying, reshuffle the cards
; 
; 
; 

%macro debug 0
	push rsi
	push rdx
	push rdi
	push rax
	mov rsi, debug_text
	mov rdx, debug_text_len
	mov rdi, 1
	mov rax, 1
	syscall
	pop rax
	pop rdi
	pop rdx
	pop rsi
%endmacro

%macro card_offset 2	; %1 = dest reg, %2 = index reg
	mov %1, %2
	imul %1, Card_size
%endmacro

%macro m_write_call 0
	push rax
	push rcx
	push rdx
	call write
	pop rdx
	pop rcx
	pop rax
%endmacro

MAX_TOTAL equ 21
DEALER_STANDS equ 17
DECK_SIZE equ 52
INPUT_BUFFER_SIZE equ 10

struc Card
	.value: resq 1
	.type: resq 1
	.suit: resq 1
endstruc


section .bss
	cards: resb Card_size * DECK_SIZE ; don't overwrite!
	deck: resb Card_size * DECK_SIZE ; current deck

	player_hand_start resq 1 ; start index
	player_hand_end resq 1 ; end index
	player_total resq 1

	dealer_hand_start resq 1 ; start index
	dealer_hand_end resq 1 ; end index
	dealer_total resq 1

	input_buffer: resb INPUT_BUFFER_SIZE ; only ever read first one
	total_str_buffer: resb 30 ; for printing player/dealer total
	total_str_buffer_len equ $ - total_str_buffer


section .data
	newline db 10
	comma_s db ', '
	debug_text db "DEBUG", 10, 0
	debug_text_len equ $ - debug_text - 1
	three_a db 'A', 'A', 'A', 10, 13

	text_welcome db "Welcome to blackjack. Dealer stands on 17. press enter to play.", 10, 0
	text_welcome_len equ $ - text_welcome - 1

	text_player_total db "Player total: ", 0
	text_player_total_len equ $ - text_player_total - 1
	text_player_hand db "Player hand:  ", 0
	text_player_hand_len equ $ - text_player_hand- 1

	text_dealer_total db "Dealer total: ", 0
	text_dealer_total_len equ $ - text_dealer_total - 1
	text_dealer_hand db "Dealer hand: ", 0
	text_dealer_hand_len equ $ - text_dealer_hand - 1

	text_turn db "press h to hit or s to stand", 10, 0
	text_turn_len equ $ - text_turn

	text_win db "You won!", 10, 0
	text_win_len equ $ - text_win - 1

	text_lose db "You lost!", 10, 0
	text_lose_len equ $ - text_lose - 1

	text_tie db "Tie!", 10, 0
	text_tie_len equ $ - text_tie - 1

	text_replay db "Play again? y/n", 10, 0
	text_replay_len equ $ - text_replay - 1

	text_invalid_input db "Invalid input.", 10, 0
	text_invalid_input_len equ $ - text_invalid_input - 1


global _start

section .text

_start:
	debug

	; mov rdi, three_a
	; mov rsi, 5
	; call write

	call generate_cards ; generate base cards

	; mov rdi, cards
	; call print_full_cards

	play_again:
	mov QWORD [player_total], 0
	mov QWORD [dealer_total], 0
	; randomize the order and insert them into deck
	call generate_deck

	; mov rdi, deck
	; call print_full_cards

	mov rdi, text_welcome
	mov rsi, text_welcome_len
	call write
	; call get_user_input ; press enter

	; logic: save the start and end indexes of the player's cards
	; and print them on each turn

	mov QWORD [player_hand_start], 0
	card_offset rcx, [player_hand_start]
	mov rbx, QWORD [deck + rcx + Card.value]
	add QWORD [player_total], rbx

	mov QWORD [player_hand_end], 1
	card_offset rcx, [player_hand_end]
	mov rbx, QWORD [deck + rcx + Card.value]
	add QWORD [player_total], rbx

	player_turn_start:
	
	call print_player_total
	call print_player_hand

	jmp check_player_total
	
	turn_input:
	mov rdi, text_turn
	mov rsi, text_turn_len
	call write
	call get_user_input

	; if h, increment player_hand_end, add it to player total, then call two print functions.
	cmp BYTE [input_buffer], 'h'
	je player_hit
	cmp BYTE [input_buffer], 's' ; if s, do dealer
	je player_stand
	jmp player_invalid_input
	
	exit:
	mov rax, 60
	mov rdi, 0
	syscall


player_hit:
; add the next card
inc QWORD [player_hand_end]
card_offset rcx, [player_hand_end]
mov rbx, QWORD [deck + rcx + Card.value]
add QWORD [player_total], rbx
jmp player_turn_start


player_stand:
	; set dealer_hand_start to player_hand_end + 1
	; set dealer_hand_end to dealer_hand_start + 1
	mov rax, QWORD [player_hand_end]
	
	inc rax
	mov QWORD [dealer_hand_start], rax
	card_offset rcx, [dealer_hand_start]
	mov rbx, QWORD [deck + rcx + Card.value]
	add QWORD [dealer_total], rbx

	.next_card:

	inc rax
	mov QWORD [dealer_hand_end], rax
	card_offset rcx, [dealer_hand_end]
	mov rbx, QWORD [deck + rcx + Card.value]
	add QWORD [dealer_total], rbx

	push rax
	call print_dealer_total
	call print_dealer_hand
	pop rax

	; check if dealer is <= 21
	cmp QWORD [dealer_total], 21
	jg player_win
	je player_tie

	; check if dealer is higher than player
	mov rdx, [dealer_total]
	cmp rdx, [player_total]
	jg player_lose

	; checks if dealer is 17 or higher but lower than player
	cmp rdx, 17
	jge player_win


	jmp .next_card



player_invalid_input:
	mov rdi, text_invalid_input
	mov rsi, text_invalid_input_len
	call write
	jmp turn_input

check_player_total:
	cmp QWORD [player_total], 21
	je player_win
	jg .ace_sub ; if there's an ace in the hand, subtract 10 and compare again


	jg player_lose
	jmp turn_input

	.ace_sub:
	go from here to add ace subtraction. i'm fucking done
		mov rax, [player_hand_start]
		mov rbx, [deck + player_hand_start]




player_win:
	mov rdi, text_win
	mov rsi, text_win_len
	call write
	jmp replay

player_lose:
	mov rdi, text_lose
	mov rsi, text_lose_len
	call write
	jmp replay

player_tie:
	mov rdi, text_tie
	mov rdi, text_tie_len
	call write
	jmp replay

replay:
	mov rdi, text_replay
	mov rsi, text_replay_len
	call write
	call get_user_input
	; y or n
	cmp BYTE [input_buffer], 'y'
	je play_again
	cmp BYTE [input_buffer], 'n'
	je exit
	mov rdi, text_invalid_input
	mov rsi, text_invalid_input_len
	call write
	jmp replay


write:
	; args:
		; rdi = address
		; rsi = length
	mov rdx, rsi ; rdx = length
	mov rsi, rdi ; rsi = address
	mov rdi, 1 ; file descriptor
	mov rax, 1 ; sys_write
	syscall
	ret


write_newln:
	mov rdi, newline
	mov rsi, 1
	call write
	ret

write_comma_s:
	mov rdi, comma_s
	mov rsi, 2
	call write
	ret

generate_cards:
	push rbp
	mov rbp, rsp
	sub rsp, 64

	; suits
	mov QWORD [rbp - 8], 'C' ; C
	mov QWORD [rbp - 16], 'D' ; D
	mov QWORD [rbp - 24], 'H' ; H
	mov QWORD [rbp - 32], 'S' ; S

	xor rax, rax 	; outer loop < 4
	xor rcx, rcx	; card index, 0 - 51
	card_offset r8, rcx
	mov rdx, -8		; suit offset, QWORD [rbp + rdx]

	.loop_outer:
		mov rbx, 2	; inner loop 2 - 10, card value
		.loop_num_inner:
			card_offset r8, rcx
			mov QWORD [cards + r8 + Card.value], rbx
			mov QWORD [cards + r8 + Card.type], 78 ; N
			mov r9, QWORD [rbp + rdx] ; suit
			mov QWORD [cards + r8 + Card.suit], r9
			inc rcx
			inc rbx
			cmp rbx, 10
			jle .loop_num_inner

		; set face cards
		card_offset r8, rcx
		mov QWORD [cards + r8 + Card.value], 10
		mov QWORD [cards + r8 + Card.type], 'J'
		mov QWORD [cards + r8 + Card.suit], r9
		inc rcx
		card_offset r8, rcx
		mov QWORD [cards + r8 + Card.value], 10
		mov QWORD [cards + r8 + Card.type], 'Q'
		mov QWORD [cards + r8 + Card.suit], r9
		inc rcx
		card_offset r8, rcx
		mov QWORD [cards + r8 + Card.value], 10
		mov QWORD [cards + r8 + Card.type], 'K'
		mov QWORD [cards + r8 + Card.suit], r9
		inc rcx
		card_offset r8, rcx
		mov QWORD [cards + r8 + Card.value], 11
		mov QWORD [cards + r8 + Card.type], 'A'
		mov QWORD [cards + r8 + Card.suit], r9
		inc rcx

		sub rdx, 8
		inc rax
		cmp rax, 4
		jl .loop_outer

	; lea rdi, [cards + Card.suit]
	; mov rsi, 1
	; call write

	mov rsp, rbp
	pop rbp
	ret


reset_deck:
	push rbp
	mov rbp, rsp

	xor eax, eax
	.loop:
		mov ebx, eax
		imul ebx, Card_size
		mov QWORD [deck + ebx], 0
		inc eax
		cmp eax, DECK_SIZE
		jl .loop

	mov rsp, rbp
	pop rbp
	ret


generate_deck:
	push rbp
	mov rbp, rsp

	call reset_deck

	; inc through the original cards array
	; generate an index and insert that card into deck

	; rax = 0
	; .loop:
	; if rax == DECK_SIZE, go to .break
	; generate random seed
	; modulo DECK_SIZE it, that's the index
	; if the value there is 0
	; 	copy the rax element in cards to that deck index
	; 	increment rax
	; else goto .loop

	xor rbx, rbx
	.loop:
		cmp rbx, DECK_SIZE
		jge .break

		rdseed rax
		xor rdx, rdx
		mov rcx, DECK_SIZE
		div rcx			; puts mod index of deck in rdx
		
		; check if blank
		mov rsi, rdx
		imul rsi, Card_size	; address of deck index
		cmp QWORD [deck + rsi], 0
		jne .loop
		
		; copy
		card_offset r8, rbx
		mov rdi, QWORD [cards + r8 + Card.value]
		mov QWORD [deck + rsi + Card.value], rdi
		mov rdi, QWORD [cards + r8 + Card.type]
		mov QWORD [deck + rsi + Card.type], rdi
		mov rdi, QWORD [cards + r8 + Card.suit]
		mov QWORD [deck + rsi + Card.suit], rdi
		
		inc rbx
		jmp .loop
	.break:
	mov rsp, rbp
	pop rbp
	ret

; TODO: make this a print_n_cards wrapper
print_full_cards:
	; store cards/deck in rdi
	push rbp
	mov rbp, rsp
	sub rsp, 8
	
	mov rdx, rdi	; cards/deck base
	xor rbx, rbx	; index
	.loop:
		card_offset rcx, rbx
		mov rax, QWORD [rdx + rcx + Card.value]
		add rax, '0'
		mov QWORD [rbp - 8], rax
		lea rdi, [rbp - 8]
		mov rsi, 1
		m_write_call
		lea rdi, [rdx + rcx + Card.type]
		mov rsi, 1
		m_write_call
		lea rdi, [rdx + rcx + Card.suit]
		mov rsi, 1
		m_write_call
		push rdx
		push rcx
		push rax
		call write_newln
		pop rax
		pop rcx
		pop rdx

		inc rbx
		cmp rbx, DECK_SIZE
		jl .loop
		
	mov rsp, rbp
	pop rbp
	ret


print_player_total:
	mov rdi, text_player_total
	mov rsi, text_player_total_len
	call write

	mov rdi, [player_total]
	call itoa_total_str_buffer	; returns str len in rax
	lea rdi, [total_str_buffer]
	mov rsi, rax
	call write
	call write_newln
	ret


print_dealer_total:
	mov rdi, text_dealer_total
	mov rsi, text_dealer_total_len
	call write

	mov rdi, [dealer_total]
	call itoa_total_str_buffer	; returns str len in rax
	lea rdi, [total_str_buffer]
	mov rsi, rax
	call write
	call write_newln
	ret


; converts a int to a string and places it in total_str_buffer
itoa_total_str_buffer:
	push rbp
	mov rbp, rsp
	sub rsp, 50
	; rdi = number
	mov r8, rdi ; number

	; wipe total_str_buffer
	xor rax, rax
	.clear_buf_loop:
		mov BYTE [total_str_buffer + rax], 0
		inc rax
		cmp rax, total_str_buffer_len - 1
		jne .clear_buf_loop

	
	xor rbx, rbx ; index increment
	; put lower 64 bits of dividend in rax, upper in rdx
	mov rax, r8	; dividend
	; put the string on stack in reverse order then copy to total_str_buffer in the correct order
	.loop_stack:
		xor rdx, rdx
		mov rcx, 10 ; divisor
		div rcx		; puts mod in rdx

		mov r11, rbx
		neg r11	; stack offset

		add dl, '0'
		mov [rbp + r11], dl ; while mod != 0, put on stack
		lea rdi, [rbp + r11]
		mov rsi, 1

		inc rbx

		cmp rax, 0 ; if num == 0
		jne .loop_stack
	.break:

	; then put them in total_str_buffer in correct order
	mov r12, r11 ; index
	xor rcx, rcx
	.loop_buffer:
		mov al, [rbp + r12] ; digit
		mov [total_str_buffer + rcx], al
		inc r12
		inc rcx
		cmp rcx, rbx
		jne .loop_buffer

	mov rax, rbx ; return length
	mov rsp, rbp
	pop rbp
	ret

get_user_input:
	xor eax, eax
	xor edi, edi
	mov rsi, input_buffer
	mov edx, INPUT_BUFFER_SIZE ; consume all of stdin
	syscall
	ret

print_n_cards:
	push rbp
	mov rbp, rsp
	sub rsp, 8
	
	mov r15, rdx	; end index (inclusive)
	mov rdx, rdi	; cards/deck base
	mov rbx, rsi	; start index

	.loop:
		card_offset rcx, rbx
		mov rax, [rdx + rcx + Card.value]
		cmp rax, 9
		jg .print_two_dig
		add rax, '0'
		mov QWORD [rbp - 8], rax
		lea rdi, [rbp - 8]
		mov rsi, 1
		m_write_call
		.skip_one:
		lea rdi, [rdx + rcx + Card.type]
		mov rsi, 1
		m_write_call
		lea rdi, [rdx + rcx + Card.suit]
		mov rsi, 1
		m_write_call
		push rdx
		push rcx
		push rax
		call write_comma_s
		pop rax
		pop rcx
		pop rdx

		inc rbx
		cmp rbx, r15
		jle .loop

	call write_newln	
	
	mov rsp, rbp
	pop rbp
	ret

	.print_two_dig:
		; print 1
		; subract 10 from rax
		; if rax == 1, print 1, else print 0
		mov QWORD [rbp - 8], '1'
		lea rdi, [rbp - 8]
		mov rsi, 1
		m_write_call
		sub eax, 10
		cmp eax, 0
		je .print_0
			mov QWORD [rbp - 8], '1'
			lea rdi, [rbp - 8]
			mov rsi, 1
			m_write_call
			jmp .skip_one
		.print_0:
			mov QWORD [rbp - 8], '0'
			lea rdi, [rbp - 8]
			mov rsi, 1
			m_write_call
			jmp .skip_one


print_player_hand:
	mov rdi, text_player_hand
	mov rsi, text_player_hand_len
	call write
	
	mov rdi, deck
	mov rsi, [player_hand_start]
	mov rdx, [player_hand_end]
	call print_n_cards
	ret

print_dealer_hand:
	mov rdi, text_dealer_hand
	mov rsi, text_dealer_hand_len
	call write
	
	mov rdi, deck
	mov rsi, [dealer_hand_start]
	mov rdx, [dealer_hand_end]
	call print_n_cards
	ret