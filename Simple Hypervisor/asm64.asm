
PUBLIC	HostContinueExecution
PUBLIC	GuestContinueExecution
PUBLIC	SaveHostRegisters
PUBLIC	HostTerminateHypervisor


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

.code _text


HostTerminateHypervisor PROC
	VMXOFF

	MOV RSP, g_StackPointerForReturning
	MOV RBP, g_BasePointerForReturning

	ADD RSP, 8
	
	XOR RAX, RAX
	MOV RAX, 1

	; Return Section
	LEA R11, [RSP+60h]
	MOV RBX, [R11+18h]
	MOV RSI, [R11+20h]
	MOV RDI, [R11+28h]
	MOV RSP, R11
	POP RBP
	RET

HostTerminateHypervisor ENDP

; ----------------------------------------------------------------------------------- ;

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
    PUSH RBP	; RSP Placeholder
    PUSH RDX
    PUSH RCX
	PUSH RBX
    PUSH RAX

	CALL VmExitHandler

	POP RAX
	POP RBX
    POP RCX
    POP RDX
    POP RBP		; RSP Placeholder
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

	SUB RSP, 0100h		; to avoid error in future functions
    JMP ResumeVm

	RET
HostContinueExecution ENDP

; ----------------------------------------------------------------------------------- ;

GuestContinueExecution PROC

	ADD		RSP, 0100h
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
	POP		RBP
	POP		RDX
	POP		RCX
	POP		RBX
	POP		RAX

	POPFQ
	RET

GuestContinueExecution ENDP

; ----------------------------------------------------------------------------------- ;

SaveHostRegisters PROC
	
	PUSHFQ
	PUSH	RAX
	PUSH	RBX
	PUSH	RCX
	PUSH	RDX
	PUSH	RBP
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

	SUB		RSP, 100h
	MOV		RAX, RSP
	RET

SaveHostRegisters ENDP


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

END