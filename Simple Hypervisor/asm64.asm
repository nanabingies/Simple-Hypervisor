
public	asm_host_continue_execution
public  asm_inv_ept_global

public	asm_get_tr
public	asm_get_ldtr
public	asm_get_idt_limit
public	asm_get_gdt_limit
public	asm_get_idt_base
public	asm_get_gdt_base

public  asm_vmx_vmcall
public  asm_hv_launch

extern  ?vmexit_handler@vmexit@@YAFPEAX@Z:proc

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

	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	sub rsp, 0108h ; 16 xmm registers... and +8 bytes for alignment...
	movaps [rsp], xmm0
	movaps [rsp + 010h], xmm1
	movaps [rsp + 020h], xmm2
	movaps [rsp + 030h], xmm3
	movaps [rsp + 040h], xmm4
	movaps [rsp + 050h], xmm5
	movaps [rsp + 060h], xmm6
	movaps [rsp + 070h], xmm7
	movaps [rsp + 080h], xmm8
	movaps [rsp + 090h], xmm9
	movaps [rsp + 0A0h], xmm10
	movaps [rsp + 0B0h], xmm11
	movaps [rsp + 0C0h], xmm12
	movaps [rsp + 0D0h], xmm13
	movaps [rsp + 0E0h], xmm14
	movaps [rsp + 0F0h], xmm15

	mov rcx, rsp
	sub rsp, 020h
	call ?vmexit_handler@vmexit@@YAFPEAX@Z			; handle VM exit 
	add rsp, 020h

	movups xmm0, [rsp]
	movups xmm1, [rsp + 010h]
	movups xmm2, [rsp + 020h]
	movups xmm3, [rsp + 030h]
	movups xmm4, [rsp + 040h]
	movups xmm5, [rsp + 050h]
	movups xmm6, [rsp + 060h]
	movups xmm7, [rsp + 070h]
	movups xmm8, [rsp + 080h]
	movups xmm9, [rsp + 090h]
	movups xmm10, [rsp + 0A0h]
	movups xmm11, [rsp + 0B0h]
	movups xmm12, [rsp + 0C0h]
	movups xmm13, [rsp + 0D0h]
	movups xmm14, [rsp + 0E0h]
	movups xmm15, [rsp + 0F0h]
	add rsp, 0108h ; 16 xmm registers... and +8 bytes for alignment...

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp 
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax

    vmresume

	int 3	; vmresume failed
asm_host_continue_execution ENDP

;----------------------------------------------------------------------------------------------------

asm_hv_launch proc
    pushfq

    mov rcx, 0681Ch		; VMCS_GUEST_RSP
	vmwrite rcx, rsp	; current rsp pointer...

	mov rcx, 0681Eh		; VMCS_GUEST_RIP
	lea rdx, done		;
	vmwrite rcx, rdx	; return 00h on success...
	vmlaunch

    pushfq				; push rflags to the stack then put it into rax...
	pop rax				;

	popfq				; restore rflags back to what it was in the c++ code...
	ret

done:
	popfq				; restore flags and return back to c++ code...
	mov rax, 00h
	ret
asm_hv_launch endp

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
	mov		rax, 02h
    ret

error_without_code:
	mov		rax, 01h
	ret

asm_inv_ept_global endp

;----------------------------------------------------------------------------------------------------

asm_vmx_vmcall proc
    vmcall
    ret
asm_vmx_vmcall endp

end
