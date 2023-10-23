
PUBLIC	AsmHostContinueExecution
PUBLIC	AsmGuestContinueExecution
PUBLIC	AsmSaveHostRegisters
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
    PUSH RDX
    PUSH RCX
	PUSH RBX
    PUSH RAX

	MOV RCX, RSP
	SUB RSP, 028h
	CALL VmExitHandler
	ADD RSP, 028h

	POP RAX
	POP RBX
    POP RCX
    POP RDX
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

	;SUB RSP, 080h		
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

AsmSaveHostRegisters PROC
	int 3
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

	ADD		RSP, 088h		; Try and use sub as used in airhv
	MOV		RAX, RSP
	RET

AsmSaveHostRegisters ENDP


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