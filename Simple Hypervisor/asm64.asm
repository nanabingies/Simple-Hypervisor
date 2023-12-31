
PUBLIC	AsmHostContinueExecution
PUBLIC	AsmGuestContinueExecution
PUBLIC	HostTerminateHypervisor
PUBLIC  AsmSetupVmcs
PUBLIC	AsmInveptGlobal
PUBLIC	AsmInveptContext


PUBLIC	GetTr
PUBLIC	GetLdtr
PUBLIC	GetIdtLimit
PUBLIC	GetGdtLimit
PUBLIC	GetIdtBase
PUBLIC	GetGdtBase

EXTERN	g_StackPointerForReturning:QWORD
EXTERN	g_BasePointerForReturning:QWORD

EXTERN	VmExitHandler:PROC
EXTERN	ResumeVm:PROC
EXTERN	SetupVmcs:PROC

.CONST
VM_ERROR_OK				EQU		00h
VM_ERROR_ERR_INFO_OK	EQU		01h
VM_ERROR_ERR_INFO_ERR	EQU		02h

.code _text


HostTerminateHypervisor PROC
	VMXOFF

	MOV RSP, g_StackPointerForReturning
	MOV RBP, g_BasePointerForReturning

	ADD RSP, 8
	
	XOR RAX, RAX
	MOV RAX, 1

	; Return Section
	int 3			; Debugging
	LEA R11, [RSP+60h]
	MOV RBX, [R11+18h]
	MOV RSI, [R11+20h]
	MOV RDI, [R11+28h]
	MOV RSP, R11
	POP RBP
	RET

HostTerminateHypervisor ENDP

; ----------------------------------------------------------------------------------- ;

AsmHostContinueExecution PROC
	;int 3		; A VM Exit just occured

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

	MOV RCX, RSP
	SUB RSP, 020h
	CALL VmExitHandler
	ADD RSP, 020h

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
		
    JMP ResumeVm

	RET
AsmHostContinueExecution ENDP

; ----------------------------------------------------------------------------------- ;

AsmGuestContinueExecution PROC
	int		3				; Local Kernel Debugging

	SUB		RSP, 088h
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
	ADD     RSP, 8    ; dummy for rsp
	POP		RBP
	POP		RDX
	POP		RCX
	POP		RBX
	POP		RAX

	POPFQ
	RET

AsmGuestContinueExecution ENDP

; ----------------------------------------------------------------------------------- ;


AsmSetupVmcs PROC
	
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
    CALL	SetupVmcs
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

AsmSetupVmcs ENDP


; ----------------------------------------------------------------------------------- ;


GetIdtBase PROC
	LOCAL	IDTR[10]:BYTE
	SIDT	IDTR
	MOV		RAX, QWORD PTR IDTR[2]
	RET
GetIdtBase ENDP

GetGdtLimit PROC
	LOCAL	GDTR[10]:BYTE
	SGDT	GDTR
	MOV		AX, WORD PTR GDTR[0]
	RET
GetGdtLimit ENDP

GetIdtLimit PROC
	LOCAL	IDTR[10]:BYTE
	SIDT	IDTR
	MOV		AX, WORD PTR IDTR[0]
	RET
GetIdtLimit ENDP

GetRflags PROC
	PUSHFQ
	POP		RAX
	RET
GetRflags ENDP

GetGdtBase PROC
	LOCAL	GDTR[10]:BYTE
	SGDT	GDTR
	MOV		RAX, QWORD PTR GDTR[2]
	RET
GetGdtBase ENDP

GetLdtr PROC
	SLDT	RAX
	RET
GetLdtr ENDP

GetTr PROC
	STR		RAX
	RET
GetTr ENDP


; ----------------------------------------------------------------------------------- ;

AsmInveptGlobal PROC
	INVEPT	RCX,	OWORD PTR [RDX]
	JZ errorWithCode						; if (ZF) jmp
    JC errorWithoutCode						; if (CF) jmp
    XOR		RAX,	RAX						; VM_ERROR_OK
    RET

errorWithCode:
	MOV		RAX,	VM_ERROR_ERR_INFO_ERR
    RET

errorWithoutCode:
	MOV		RAX,	VM_ERROR_ERR_INFO_OK
	RET

AsmInveptGlobal	ENDP

AsmInveptContext PROC

AsmInveptContext ENDP


END
