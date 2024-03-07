
PUBLIC	asm_host_continue_execution
;PUBLIC	asmGuestContinueExecution
;PUBLIC	hostTerminateHypervisor
PUBLIC  asm_setup_vmcs
;PUBLIC	asmInveptGlobal
;PUBLIC	asmInveptContext

PUBLIC	asm_get_tr
PUBLIC	asm_get_ldtr
PUBLIC	asm_get_idt_limit
PUBLIC	asm_get_gdt_limit
PUBLIC	asm_get_idt_base
PUBLIC	asm_get_gdt_base

extern	?setup_vmcs@@YA?AW4EVmErrors@@KPEAX@Z:PROC
extern  ?vmexit_handler@vmexit@@YAFPEAX@Z:PROC

.CONST
VM_ERROR_OK				EQU		00h
VM_ERROR_ERR_INFO_OK	EQU		01h
VM_ERROR_ERR_INFO_ERR	EQU		02h

.code _text

SAVE_GP macro
        push    rax
        push    rcx
        push    rdx
        push    rbx
        push    -01h ; placeholder for rsp
        push    rbp 
        push    rsi
        push    rdi
        push    r8
        push    r9
        push    r10
        push    r11
        push    r12
        push    r13
        push    r14
        push    r15
endm

RESTORE_GP macro
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdi
        pop     rsi
        pop     rbp
        pop     rbx ; placeholder for rsp
        pop     rbx
        pop     rdx
        pop     rcx
        pop     rax
endm

asm_host_continue_execution PROC
	;int 3		; A VM Exit just occured

	PUSHFQ
	SAVE_GP

	sub     rsp, 060h
	movdqa  xmmword ptr [rsp], xmm0
    movdqa  xmmword ptr [rsp + 10h], xmm1
    movdqa  xmmword ptr [rsp + 20h], xmm2
    movdqa  xmmword ptr [rsp + 30h], xmm3
    movdqa  xmmword ptr [rsp + 40h], xmm4
    movdqa  xmmword ptr [rsp + 50h], xmm5

	mov rcx, rsp
	sub rsp, 020h
	call ?vmexit_handler@vmexit@@YAFPEAX@Z			; handle VM exit 
	add rsp, 020h

	movdqa  xmm0, xmmword ptr [rsp]
    movdqa  xmm1, xmmword ptr [rsp + 10h]
    movdqa  xmm2, xmmword ptr [rsp + 20h]
    movdqa  xmm3, xmmword ptr [rsp + 30h]
    movdqa  xmm4, xmmword ptr [rsp + 40h]
    movdqa  xmm5, xmmword ptr [rsp + 50h]
	add     rsp,  060h

	cmp     al, 0
    jnz      exit

	RESTORE_GP
    vmresume

exit:
	sub rsp, 20h
    ;call ?return_rsp_for_vmxoff@@YA_KXZ
    add rsp, 20h

	push rax

    sub rsp, 20h
    ;call ?return_rip_for_vmxoff@@YA_KXZ
    add rsp, 20h

	push rax

    mov rcx,rsp
    mov rsp,[rcx+8h]
    mov rax,[rcx]
    push rax

    mov r15, [rcx + 10h]
    mov r14, [rcx + 18h]
    mov r13, [rcx + 20h]
    mov r12, [rcx + 28h]
    mov r11, [rcx + 30h]
    mov r10, [rcx + 38h]
    mov r9,  [rcx + 40h]
    mov r8,  [rcx + 48h]
    mov rdi, [rcx + 50h]
    mov rsi, [rcx + 58h]
    mov rbp, [rcx + 60h]
    mov rbx, [rcx + 70h]
    mov rdx, [rcx + 78h]
    mov rax, [rcx + 88h]
    mov rcx, [rcx + 80h]

	ret
asm_host_continue_execution ENDP

;----------------------------------------------------------------------------------------------------

asm_setup_vmcs PROC
	int		3
	
	pushfq
	SAVE_GP

	sub     RSP, 060h
	movdqa  xmmword ptr [RSP], XMM0
    movdqa  xmmword ptr [RSP + 10h], XMM1
    movdqa  xmmword ptr [RSP + 20h], XMM2
    movdqa  xmmword ptr [RSP + 30h], XMM3
    movdqa  xmmword ptr [RSP + 40h], XMM4
    movdqa  xmmword ptr [RSP + 50h], XMM5

	mov		RDX, RSP
	sub		RSP, 020h
    call	?setup_vmcs@@YA?AW4EVmErrors@@KPEAX@Z
    add		RSP, 020h

	movdqa  XMM0, xmmword ptr [RSP]
    movdqa  XMM1, xmmword ptr [RSP + 10h]
    movdqa  XMM2, xmmword ptr [RSP + 20h]
    movdqa  XMM3, xmmword ptr [RSP + 30h]
    movdqa  XMM4, xmmword ptr [RSP + 40h]
    movdqa  XMM5, xmmword ptr [RSP + 50h]
	add     RSP,  060h

	RESTORE_GP
	popfq
	ret

asm_setup_vmcs ENDP

;--------------------------------------------------------------------------------------------

asm_get_idt_base PROC
	LOCAL	IDTR[10]:BYTE
	SIDT	IDTR
	MOV		RAX, QWORD PTR IDTR[2]
	RET
asm_get_idt_base ENDP

asm_get_gdt_limit PROC
	LOCAL	GDTR[10]:BYTE
	SGDT	GDTR
	MOV		AX, WORD PTR GDTR[0]
	RET
asm_get_gdt_limit ENDP

asm_get_idt_limit PROC
	LOCAL	IDTR[10]:BYTE
	SIDT	IDTR
	MOV		AX, WORD PTR IDTR[0]
	RET
asm_get_idt_limit ENDP

asm_get_rflags PROC
	PUSHFQ
	POP		RAX
	RET
asm_get_rflags ENDP

asm_get_gdt_base PROC
	LOCAL	GDTR[10]:BYTE
	SGDT	GDTR
	MOV		RAX, QWORD PTR GDTR[2]
	RET
asm_get_gdt_base ENDP

asm_get_ldtr PROC
	SLDT	RAX
	RET
asm_get_ldtr ENDP

asm_get_tr PROC
	STR		RAX
	RET
asm_get_tr ENDP

END
