
public	asm_host_continue_execution
;public	asmGuestContinueExecution
;public	hostTerminateHypervisor
public  asm_setup_vmcs
;public	asmInveptGlobal
;public	asmInveptContext

public	asm_get_tr
public	asm_get_ldtr
public	asm_get_idt_limit
public	asm_get_gdt_limit
public	asm_get_idt_base
public	asm_get_gdt_base

extern	?setup_vmcs@@YA?AW4EVmErrors@@KPEAX_K@Z:proc
extern  ?vmexit_handler@vmexit@@YAFPEAX@Z:proc
extern  asm_inv_ept_global:proc
extern  ret_val:dword
extern  cr3_val:qword

.CONST
VM_ERROR_OK				equ		00h
VM_ERROR_ERR_INFO_OK	equ		01h
VM_ERROR_ERR_INFO_ERR	equ		02h

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

asm_host_continue_execution proc
	;int 3		; A VM Exit just occured

	pushfq
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

asm_setup_vmcs proc
	
	pushfq
	SAVE_GP

	sub     rsp, 060h
	movdqa  xmmword ptr [rsp], xmm0
    movdqa  xmmword ptr [rsp + 10h], xmm1
    movdqa  xmmword ptr [rsp + 20h], xmm2
    movdqa  xmmword ptr [rsp + 30h], xmm3
    movdqa  xmmword ptr [rsp + 40h], xmm4
    movdqa  xmmword ptr [rsp + 50h], xmm5

	mov		rdx, rsp
    mov     r8,  cr3_val
	sub		rsp, 020h
    call	?setup_vmcs@@YA?AW4EVmErrors@@KPEAX_K@Z
    mov     ret_val, eax
    add		rsp, 020h

	movdqa  xmm0, xmmword ptr [rsp]
    movdqa  xmm1, xmmword ptr [rsp + 10h]
    movdqa  xmm2, xmmword ptr [rsp + 20h]
    movdqa  xmm3, xmmword ptr [rsp + 30h]
    movdqa  xmm4, xmmword ptr [rsp + 40h]
    movdqa  xmm5, xmmword ptr [rsp + 50h]
	add     rsp,  060h

	RESTORE_GP
	popfq
    mov     eax, ret_val
	ret

asm_setup_vmcs endp

;----------------------------------------------------------------------------------------------------

asm_get_idt_base proc
	local	idtr[10]:byte
	sidt	idtr
	mov		RAX, qword ptr idtr[2]
	ret
asm_get_idt_base endp

asm_get_gdt_limit proc
	local	gdtr[10]:byte
	sgdt	gdtr
	mov		ax, word ptr gdtr[0]
	ret
asm_get_gdt_limit endp

asm_get_idt_limit proc
	local	idtr[10]:byte
	SIDT	idtr
	mov		ax, word ptr idtr[0]
	ret
asm_get_idt_limit endp

asm_get_rflags proc
	pushfq
	POP		rax
	ret
asm_get_rflags endp

asm_get_gdt_base proc
	local	gdtr[10]:byte
	sgdt	gdtr
	mov		rax, qword ptr gdtr[2]
	ret
asm_get_gdt_base endp

asm_get_ldtr proc
	sldt	rax
	ret
asm_get_ldtr endp

asm_get_tr proc
	str		rax
	ret
asm_get_tr endp

;----------------------------------------------------------------------------------------------------

asm_inv_ept_global proc
    invept	rcx, oword ptr [rdx]
	jz      error_with_code						; if (ZF) jmp
    jc      error_without_code					; if (CF) jmp
    xor		rax, rax						    ; VM_ERROR_OK
    ret

error_with_code:
	mov		rax,	VM_ERROR_ERR_INFO_ERR
    ret

error_without_code:
	mov		rax,	VM_ERROR_ERR_INFO_OK
	ret
asm_inv_ept_global endp

end
