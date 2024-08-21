#include "precomp.h"


#pragma hdrstop

#include <DriverSpecs.h>
__user_driver

#pragma comment(lib,"Advapi32.lib")

#pragma prefast (suppress : __WARNING_INCORRECT_ANNOTATION_STRING, "The Pointer is part of an Array. This is the only way to annotate it")
#pragma prefast (suppress : __WARNING_INCORRECT_ANNOTATION, "The Pointer is part of an Array. This is the only way to annotate it")

extern PMONITORINIT g_pMonitorInit;
extern WCHAR g_szPortsList[];

//
// These globals are needed so that AddPort can call
// SPOOLSS!EnumPorts to see whether the port to be added
// already exists.

typedef BOOL
(WINAPI *FnEnumPortsW)(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pPort,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
    );

HMODULE hSpoolssDll = NULL;
FnEnumPortsW pfnSpoolssEnumPorts = NULL;

VOID
LcmRemoveColon(
    __inout LPWSTR  pName
    )
{
    DWORD   Length = 0;


    Length = (DWORD)wcslen(pName);

    if (pName[Length-1] == L':')
        pName[Length-1] = 0;
}

HMODULE
LoadSystemLibrary (
    __in    LPCWSTR szDllName
    )
{
    HMODULE hModule                  = NULL;
    WCHAR   szFullDllName [MAX_PATH] = L"";


    if (GetSystemDirectory (szFullDllName, COUNTOF (szFullDllName)) &&
        StringCchCat (szFullDllName, COUNTOF (szFullDllName), L"\\") == S_OK &&
        StringCchCat (szFullDllName, COUNTOF (szFullDllName), szDllName) == S_OK)
    {
        hModule = LoadLibrary (szFullDllName);
    }

    return hModule;
}

/* PortExists
 *
 * Calls EnumPorts to check whether the port name already exists.
 * This asks every monitor, rather than just this one.
 * The function will return TRUE if the specified port is in the list.
 * If an error occurs, the return is FALSE and the variable pointed
 * to by pError contains the return from GetLastError().
 * The caller must therefore always check that *pError == NO_ERROR.
 */
BOOL
PortExists(
    __in_opt LPWSTR pName,
    __in     LPWSTR pPortName,
    __out    PDWORD pError
)
{
    DWORD           cbNeeded    = 0;
    DWORD           cReturned   = 0;
    DWORD           cbPorts     = 0;
    LPPORT_INFO_1   pPorts      = NULL;
    DWORD           i           = 0;
    BOOL            Found       = TRUE;


    *pError = NO_ERROR;

    if (!hSpoolssDll) {

        hSpoolssDll = LoadSystemLibrary (L"SPOOLSS.DLL");

        if (hSpoolssDll) {
            pfnSpoolssEnumPorts = (FnEnumPortsW)GetProcAddress(hSpoolssDll,
                                                 "EnumPortsW");
            if (!pfnSpoolssEnumPorts) {

                *pError = GetLastError();
                FreeLibrary(hSpoolssDll);
                hSpoolssDll = NULL;
            }

        } else {

            *pError = GetLastError();
        }
    }

    if (!pfnSpoolssEnumPorts)
        return FALSE;


    if (!(*pfnSpoolssEnumPorts)(pName, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cbPorts = cbNeeded;

            pPorts = (LPPORT_INFO_1)AllocSplMem(cbPorts);

            if (pPorts)
            {
                if ((*pfnSpoolssEnumPorts)(pName, 1, (LPBYTE)pPorts, cbPorts,
                                           &cbNeeded, &cReturned))
                {
                    Found = FALSE;

                    for (i = 0; i < cReturned; i++)
                    {
                        if (!lstrcmpi(pPorts[i].pName, pPortName))
                            Found = TRUE;
                    }
                }
            }

            FreeSplMem(pPorts);
        }
    }

    else
        Found = FALSE;


    return Found;
}


VOID
LcmSplInSem(
   VOID
)
{

    if (LcmSpoolerSection.OwningThread != (HANDLE) UIntToPtr(GetCurrentThreadId())) {
    }
}

VOID
LcmSplOutSem(
   VOID
)
{

    if (LcmSpoolerSection.OwningThread == (HANDLE) UIntToPtr(GetCurrentThreadId())) {
    }
}

VOID
LcmEnterSplSem(
   VOID
)
{
    EnterCriticalSection(&LcmSpoolerSection);
}

VOID
LcmLeaveSplSem(
   VOID
)
{
#if DBG
    LcmSplInSem();
#endif
    LeaveCriticalSection(&LcmSpoolerSection);
}

PINIENTRY
LcmFindIniKey(
   __in PINIENTRY pIniEntry,
   __in LPWSTR pName
)
{

   if (!pName)
      return NULL;

   LcmSplInSem();

   while (pIniEntry && lstrcmpi(pName, pIniEntry->pName))
      pIniEntry = pIniEntry->pNext;

   return pIniEntry;
}

LPBYTE
LcmPackStrings(
    __in                            DWORD   dwElementsCount,
    __in_ecount(dwElementsCount)    LPWSTR  *pSource,
    __out                           LPBYTE  pDest,
    __in_ecount(dwElementsCount)    DWORD   *DestOffsets,
    __inout                         LPBYTE  pEnd
)
{

    DWORD dwCount = 0;

    for (dwCount = 0; dwCount < dwElementsCount; dwCount++)
    {
        if (*pSource) {
            size_t cbString = wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
            pEnd-= cbString;
            (VOID) StringCbCopy ((LPWSTR) pEnd, cbString, *pSource);
            *(LPWSTR UNALIGNED *)(pDest+*DestOffsets)= (LPWSTR) pEnd;
        } else
            *(LPWSTR UNALIGNED *)(pDest+*DestOffsets)=0;

      pSource++;
      DestOffsets++;
   }

   return pEnd;
}

DWORD
WINAPIV
StrNCatBuffW(
    __out_ecount(cchBuffer) PWSTR       pszBuffer,
    UINT        cchBuffer,
    ...
    )
/*++

Description:

    This routine concatenates a set of null terminated strings
    into the provided buffer.  The last argument must be a NULL
    to signify the end of the argument list.  This only called
        from LocalMon by functions that use WCHARS.

Arguments:

    pszBuffer  - pointer buffer where to place the concatenated
                 string.
    cchBuffer  - character count of the provided buffer including
                 the null terminator.
    ...        - variable number of string to concatenate.

Returns:

    ERROR_SUCCESS if new concatenated string is returned,
    or ERROR_XXX if an error occurred.

Notes:

    The caller must pass valid strings as arguments to this routine,
    if an integer or other parameter is passed the routine will either
    crash or fail abnormally.  Since this is an internal routine
    we are not in try except block for performance reasons.

--*/
{
    DWORD   dwRetval    = ERROR_INVALID_PARAMETER;
    PCWSTR  pszTemp     = NULL;
    PWSTR   pszDest     = NULL;
    va_list pArgs;


    //
    // Validate the pointer where to return the buffer.
    //
    if (pszBuffer && cchBuffer)
    {
        //
        // Assume success.
        //
        dwRetval = ERROR_SUCCESS;

        //
        // Get pointer to argument frame.
        //
        va_start(pArgs, cchBuffer);

        //
        // Get temp destination pointer.
        //
        pszDest = pszBuffer;

        //
        // Insure we have space for the null terminator.
        //
        cchBuffer--;

        //
        // Collect all the arguments.
        //
        for ( ; ; )
        {
            //
            // Get pointer to the next argument.
            //
            pszTemp = va_arg(pArgs, PCWSTR);

            if (!pszTemp)
            {
                break;
            }

            //
            // Copy the data into the destination buffer.
            //
            for ( ; cchBuffer; cchBuffer-- )
            {
                *pszDest = *pszTemp;
                if (!(*pszDest))
                {
                    break;
                }

                pszDest++, pszTemp++;
            }

            //
            // If were unable to write all the strings to the buffer,
            // set the error code and delete the incomplete copied strings.
            //
            if (!cchBuffer && pszTemp && *pszTemp)
            {
                dwRetval = ERROR_BUFFER_OVERFLOW;
                *pszBuffer = L'\0';
                break;
            }
        }

        //
        // Terminate the buffer always.
        //
        *pszDest = L'\0';

        va_end(pArgs);
    }

    //
    // Set the last error in case the caller forgets to.
    //
    if (dwRetval != ERROR_SUCCESS)
    {
        SetLastError(dwRetval);
    }

    return dwRetval;

}

LPWSTR
AdjustFileName(
    __in LPWSTR pFileName
    )
{
    LPWSTR pNewName         = NULL;

    pNewName = AllocSplStr( pFileName);

    return pNewName;
}

BOOL
PortIsValid(
	__in    LPWSTR pPortName
)
{
	return TRUE;
}

VOID
GetTransmissionRetryTimeoutFromRegistry(
	__out DWORD*      pdwTimeout
)
{
}

DWORD
SetTransmissionRetryTimeoutInRegistry(
	__in LPCWSTR       pszTimeout
)
{
	return 0;
}

BOOL
AddPortInRegistry(
	__in LPCWSTR pszPortName
)
{
	return TRUE;
}

VOID
DeletePortFromRegistry(
	__in LPCWSTR         pszPortName
)
{
}