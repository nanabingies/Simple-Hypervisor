#pragma once
#define numPagesToAllocate	10

BOOLEAN CheckEPTSupport();

VOID InitializeEpt();

BOOLEAN EptBuildMTRRMap();