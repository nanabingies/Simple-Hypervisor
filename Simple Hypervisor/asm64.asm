
PUBLIC HostContinueExecution
PUBLIC SaveStackRegs
PUBLIC HostTerminateHypervisor

;PUBLIC GetCs
;PUBLIC GetDs
;PUBLIC GetEs
;PUBLIC GetSs
;PUBLIC GetFs
;PUBLIC GetGs
;PUBLIC GetRflags

PUBLIC GetTr
PUBLIC GetLdtr
PUBLIC GetIdtLimit
PUBLIC GetGdtLimit
PUBLIC GetIdtBase
PUBLIC GetGdtBase

EXTERN g_StackPointerForReturning:QWORD
EXTERN g_BasePointerForReturning:QWORD

EXTERN VmExit:QWORD

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

	CALL VmExit
	RET
HostContinueExecution ENDP

SaveStackRegs PROC
	MOV g_StackPointerForReturning, RSP
	MOV g_BasePointerForReturning, RBP
	RET
SaveStackRegs ENDP

GetCs PROC
	MOV		RAX, CS
	RET
GetCs ENDP

GetDs PROC
	MOV		RAX, DS
	RET
GetDs ENDP

GetEs PROC
	MOV		RAX, ES
	RET
GetEs ENDP

GetSs PROC
	MOV		RAX, SS
	RET
GetSs ENDP

GetFs PROC
	MOV		RAX, FS
	RET
GetFs ENDP

GetGs PROC
	MOV		RAX, GS
	RET
GetGs ENDP

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