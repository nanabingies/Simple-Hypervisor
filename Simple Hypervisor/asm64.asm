PUBLIC GetLdtr
PUBLIC GetTr
PUBLIC HostContinueExecution
PUBLIC SaveStackRegs

.code _text

GetLdtr PROC
	SLDT	RAX
	RET
GetLdtr ENDP

GetTr PROC
	STR		RAX
	RET
GetTr ENDP

HostContinueExecution PROC
	int 3
	int 3
HostContinueExecution ENDP

SaveStackRegs PROC
	RET
SaveStackRegs ENDP

END