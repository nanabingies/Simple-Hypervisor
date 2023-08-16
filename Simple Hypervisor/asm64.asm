
PUBLIC HostContinueExecution
PUBLIC SaveHostRegisters
PUBLIC HostTerminateHypervisor

PUBLIC GetTr
PUBLIC GetLdtr
PUBLIC GetIdtLimit
PUBLIC GetGdtLimit
PUBLIC GetIdtBase
PUBLIC GetGdtBase

EXTERN g_StackPointerForReturning:QWORD
EXTERN g_BasePointerForReturning:QWORD

EXTERN VmExitHandler:PROC

.code _text

GetLdtr PROC
	SLDT	RAX
	RET
GetLdtr ENDP

GetTr PROC
	STR		RAX
	RET
GetTr ENDP

HostTerminateHypervisor PROC
	VMXOFF

	MOV RSP, g_StackPointerForReturning
	MOV RBP, g_BasePointerForReturning

	ADD RSP, 8
	
	XOR RAX, RAX
	MOV RAX, 1

	; Return Section
	MOV RBX, [RSP+60h+18h]
	MOV RSI, [RSP+60h+20h]
	MOV RDI, [RSP+60h+28h]
	POP RBP
	RET
HostTerminateHypervisor ENDP

HostContinueExecution PROC
	int 3		; A VM Exit just occured

	PUSH R15
    PUSH R14
    PUSH R13
    PUSH R12
    PUSH R11
    PUSH R10
    PUSH R9
    PUSH R8        
    PUSH RDI
    PUSH RSI
    PUSH RBP
    PUSH RBP	; RSP
    PUSH RBX
    PUSH RDX
    PUSH RCX
    PUSH RAX

	CALL VmExitHandler

	POP RAX
    POP RCX
    POP RDX
    POP RBX
    POP RBP		; RSP
    POP RBP
    POP RSI
    POP RDI 
    POP R8
    POP R9
    POP R10
    POP R11
    POP R12
    POP R13
    POP R14
    POP R15

	SUB RSP, 0100h ; to avoid error in future functions
    ;JMP VmResumeInstruction

	RET
HostContinueExecution ENDP

SaveHostRegisters PROC
	MOV g_StackPointerForReturning, RSP
	MOV g_BasePointerForReturning, RBP
	RET
SaveHostRegisters ENDP





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

END