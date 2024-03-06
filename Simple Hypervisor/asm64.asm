
;PUBLIC	asmHostContinueExecution
;PUBLIC	asmGuestContinueExecution
;PUBLIC	hostTerminateHypervisor
PUBLIC  asm_setup_vmcs
;PUBLIC	asmInveptGlobal
;PUBLIC	asmInveptContext

EXTERN	?setup_vmcs@@YA?AW4EVmErrors@@KPEAX@Z:PROC

.CONST
VM_ERROR_OK				EQU		00h
VM_ERROR_ERR_INFO_OK	EQU		01h
VM_ERROR_ERR_INFO_ERR	EQU		02h

.code _text

asm_setup_vmcs PROC
	int		3
	
	PUSHFQ
	PUSH	RAX
	PUSH	RBX
	PUSH	RCX
	PUSH	RDX
	PUSH	RBP
	PUSH	-1				; Dummy for RSP
	PUSH	RSI
	PUSH	RDI
	PUSH	R8
	PUSH	R9
	PUSH	R10
	PUSH	R11
	PUSH	R12
	PUSH	R13
	PUSH	R14
	PUSH	R15

	SUB     RSP, 060h
	MOVDQA  xmmword ptr [RSP], XMM0
    MOVDQA  xmmword ptr [RSP + 10h], XMM1
    MOVDQA  xmmword ptr [RSP + 20h], XMM2
    MOVDQA  xmmword ptr [RSP + 30h], XMM3
    MOVDQA  xmmword ptr [RSP + 40h], XMM4
    MOVDQA  xmmword ptr [RSP + 50h], XMM5

	MOV		RDX, RSP
	SUB		RSP, 020h
    CALL	?setup_vmcs@@YA?AW4EVmErrors@@KPEAX@Z
    ADD		RSP, 020h

	MOVDQA  XMM0, xmmword ptr [RSP]
    MOVDQA  XMM1, xmmword ptr [RSP + 10h]
    MOVDQA  XMM2, xmmword ptr [RSP + 20h]
    MOVDQA  XMM3, xmmword ptr [RSP + 30h]
    MOVDQA  XMM4, xmmword ptr [RSP + 40h]
    MOVDQA  XMM5, xmmword ptr [RSP + 50h]
	ADD     RSP,  060h

	POP		R15
	POP		R14
	POP		R12
	POP		R13
	POP		R12
	POP		R11
	POP		R10
	POP		R9
	POP		R8
	POP		RDI
	POP		RSI
	;ADD     RSP, 8    ; dummy for rsp
	POP		RBP
	POP		RDX
	POP		RCX
	POP		RBX
	POP		RAX

	POPFQ
	RET

asm_setup_vmcs ENDP

END
