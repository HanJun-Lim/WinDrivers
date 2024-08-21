#ifndef _MEM_H_
#define _MEM_H_

LPWSTR
AllocSplStr(
    __in IN LPCWSTR pStr
    );

LPVOID
AllocSplMem(
    IN DWORD cbAlloc
    );

BOOL
FreeSplStr(
    __in    LPWSTR pStr
    );

BOOL
FreeSplMem(
    __in_opt    PVOID pMem
    );

#endif // _MEM_H_

