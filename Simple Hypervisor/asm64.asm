
public	asm_host_continue_execution
public  asm_inv_ept_global

public asm_save_vmm_state
public asm_restore_vmm_state



public  asm_vmx_vmcall

extern  ?vmexit_handler@vmexit@@YAXPEAX@Z:proc
extern  ?initialize_vmm@hv@@YAXPEAX@Z:proc

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

;----------------------------------------------------------------------------------------------------

asm_host_continue_execution proc
	int 3		; A VM Exit just occured

    SAVE_GP
    sub     rsp ,60h

    movdqa  xmmword ptr [rsp], xmm0
    movdqa  xmmword ptr [rsp + 10h], xmm1
    movdqa  xmmword ptr [rsp + 20h], xmm2
    movdqa  xmmword ptr [rsp + 30h], xmm3
    movdqa  xmmword ptr [rsp + 40h], xmm4
    movdqa  xmmword ptr [rsp + 50h], xmm5

    mov     rcx, rsp
    sub     rsp,  20h
    call    ?vmexit_handler@vmexit@@YAXPEAX@Z       ; handle VM exit 
    add     rsp, 20h

    movdqa  xmm0, xmmword ptr [rsp]
    movdqa  xmm1, xmmword ptr [rsp + 10h]
    movdqa  xmm2, xmmword ptr [rsp + 20h]
    movdqa  xmm3, xmmword ptr [rsp + 30h]
    movdqa  xmm4, xmmword ptr [rsp + 40h]
    movdqa  xmm5, xmmword ptr [rsp + 50h]

    add     rsp,  60h

    RESTORE_GP
    vmresume

asm_host_continue_execution ENDP

;----------------------------------------------------------------------------------------------------

asm_save_vmm_state proc

    int 3
    pushfq
    SAVE_GP
    sub rsp, 020h
    mov rcx, rsp
    call ?initialize_vmm@hv@@YAXPEAX@Z
    
    int 3   ; error

asm_save_vmm_state endp

;----------------------------------------------------------------------------------------------------

asm_restore_vmm_state proc
    add rsp, 020h
    RESTORE_GP
    popfq
    ret
asm_restore_vmm_state endp

;----------------------------------------------------------------------------------------------------

__read_ldtr proc
    sldt ax
    ret
__read_ldtr endp

__read_tr proc
    str ax
    ret
__read_tr endp

__read_cs proc
    mov ax, cs
    ret
__read_cs endp

__read_ss proc
    mov ax, ss
    ret
__read_ss endp

__read_ds proc
    mov ax, ds
    ret
__read_ds endp

__read_es proc
    mov ax, es              
    ret
__read_es endp

__read_fs proc
    mov ax, fs
    ret
__read_fs endp

__read_gs proc
    mov ax, gs
    ret
__read_gs endp

__sgdt proc
    sgdt qword ptr [rcx]
    ret
__sgdt endp

__sidt proc
    sidt qword ptr [rcx]
    ret
__sidt endp

__load_ar proc
    lar rax, rcx
    jz no_error
    xor rax, rax
no_error:
    ret
__load_ar endp

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
