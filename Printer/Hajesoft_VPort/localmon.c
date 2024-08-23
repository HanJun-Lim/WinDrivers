#include "precomp.h"


#pragma hdrstop

#include <DriverSpecs.h>
__user_driver

#include <lmon.h>
#include "irda.h"

#pragma comment(lib,"Winspool.lib")
#pragma comment(lib,"Spoolss.lib")

HANDLE              LcmhMonitor;
HINSTANCE           LcmhInst;
CRITICAL_SECTION    LcmSpoolerSection;

PMONITORINIT        g_pMonitorInit;

DWORD LcmPortInfo1Strings[]={FIELD_OFFSET(PORT_INFO_1, pName),
                          (DWORD)-1};

DWORD LcmPortInfo2Strings[]={FIELD_OFFSET(PORT_INFO_2, pPortName),
                          FIELD_OFFSET(PORT_INFO_2, pMonitorName),
                          FIELD_OFFSET(PORT_INFO_2, pDescription),
                          (DWORD)-1};

WCHAR gszWindows[]= L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
WCHAR szPortsEx[] = L"portsex"; /* Extra ports values */
WCHAR szNUL[]     = L"NUL";
WCHAR szNUL_COLON[] = L"NUL:";
WCHAR szFILE[]    = L"FFLE:";
WCHAR szLcmCOM[]  = L"CCM";
WCHAR szLcmLPT[]  = L"LLT";
WCHAR szVPORT[]    = L"VPORT";

extern DWORD g_COMWriteTimeoutConstant_ms;

BOOL
LocalMonInit(HINSTANCE hModule)
{

    LcmhInst = hModule;

    return InitializeCriticalSectionAndSpinCount(&LcmSpoolerSection, 0x80000000);
}


VOID
LocalMonCleanUp(
    VOID
    )
{
    DeleteCriticalSection(&LcmSpoolerSection);
}

/// <summary>
/// 스풀러가 프린터 포트에 대한 열거를 하고 싶어서 프린터 포트의 이름을 찾고자 할때마다 호출
/// </summary>
/// <param name="hMonitor"></param>
/// <param name="pName"></param>
/// <param name="Level"></param>
/// <param name="pPorts"></param>
/// <param name="cbBuf"></param>
/// <param name="pcbNeeded"></param>
/// <param name="pcReturned"></param>
/// <returns></returns>
BOOL WINAPI
LcmEnumPorts(
    __in                        HANDLE  hMonitor,
    __in_opt                    LPWSTR  pName,
                                DWORD   Level,
    __out_bcount_opt(cbBuf)     LPBYTE  pPorts,
                                DWORD   cbBuf,
    __out                       LPDWORD pcbNeeded,
    __out                       LPDWORD pcReturned
    )
{
    PINILOCALMON    pIniLocalMon    = (PINILOCALMON)hMonitor;
    PLCMINIPORT     pIniPort        = NULL;
    DWORD           cb              = 0;
    LPBYTE          pEnd            = NULL;
    DWORD           LastError       = 0;

    UNREFERENCED_PARAMETER(pName);

    LcmEnterSplSem();

    cb=0;

    pIniPort = pIniLocalMon->pIniPort;

    CheckAndAddIrdaPort(pIniLocalMon);

    while (pIniPort) {

        if ( !(pIniPort->Status & PP_FILEPORT) ) {

            cb+=GetPortSize(pIniPort, Level);
        }
        pIniPort=pIniPort->pNext;
    }

    *pcbNeeded=cb;

    if (cb <= cbBuf) {

        pEnd=pPorts+cbBuf;
        *pcReturned=0;

        pIniPort = pIniLocalMon->pIniPort;
        while (pIniPort) {

            if (!(pIniPort->Status & PP_FILEPORT)) {

                pEnd = CopyIniPortToPort(pIniPort, Level, pPorts, pEnd);

                if( !pEnd ){
                    LastError = GetLastError();
                    break;
                }

                
                // 각 레벨에 맞는 포트에 대한 정보 구조체가 있으며,
                // 그 구조체의 포맷에 맞게끔 VPort는 자신이 가진 포트의 이름을 돌려주면 됨
                switch (Level) {
                case 1:
                    pPorts+=sizeof(PORT_INFO_1);
                    break;
                case 2:
                    pPorts+=sizeof(PORT_INFO_2);
                    break;
                default:
                    LastError = ERROR_INVALID_LEVEL;
                    goto Cleanup;
                }
                (*pcReturned)++;
            }
            pIniPort=pIniPort->pNext;
        }

    } else

        LastError = ERROR_INSUFFICIENT_BUFFER;

Cleanup:
   LcmLeaveSplSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;

    } else

        return TRUE;
}

BOOL WINAPI
LcmOpenPort(
    __in    HANDLE  hMonitor,
    __in    LPWSTR  pName,
    __out   PHANDLE pHandle
    )
{
	*pHandle = hMonitor;
	return TRUE;
}

BOOL WINAPI
LcmStartDocPort(
    __in    HANDLE  hPort,
    __in    LPWSTR  pPrinterName,
            DWORD   JobId,
            DWORD   Level,
    __in    LPBYTE  pDocInfo)
{
	PINILOCALMON    pIniLocalMon = (PINILOCALMON)hPort;
	pIniLocalMon->JobId = JobId;
	wcscpy(pIniLocalMon->PrinterName, pPrinterName);
	return TRUE;
}

BOOL WINAPI
LcmWritePort(
            __in                HANDLE  hPort,
            __in_bcount(cbBuf)  LPBYTE  pBuffer,
            DWORD   cbBuf,
            __out               LPDWORD pcbWritten)
{
	*pcbWritten = cbBuf;
	return TRUE;
}


BOOL WINAPI
LcmReadPort(
    __in                HANDLE      hPort,
    __out_bcount(cbBuf) LPBYTE      pBuffer,
                        DWORD       cbBuf,
    __out               LPDWORD     pcbRead)
{
	return FALSE;
}

BOOL WINAPI
LcmEndDocPort(
    __in    HANDLE   hPort
    )
{
	PINILOCALMON    pIniLocalMon = (PINILOCALMON)hPort;
	JOB_INFO_3	JobInfo3 = { 0, };

	if (OpenPrinter(pIniLocalMon->PrinterName, &pIniLocalMon->hPrinter, NULL) == TRUE)
	{
		JobInfo3.JobId = pIniLocalMon->JobId;
		JobInfo3.NextJobId = 0;
		SetJob(pIniLocalMon->hPrinter, pIniLocalMon->JobId, 3, (LPBYTE)&JobInfo3, JOB_CONTROL_LAST_PAGE_EJECTED);

		ClosePrinter(pIniLocalMon->hPrinter);
		pIniLocalMon->hPrinter = NULL;
	}
    return TRUE;
}

BOOL WINAPI
LcmClosePort(
    __in    HANDLE  hPort
    )
{
    return TRUE;
}


BOOL WINAPI
LcmAddPortEx(
    __in        HANDLE   hMonitor,
    __in_opt    LPWSTR   pName,
                DWORD    Level,
    __in        LPBYTE   pBuffer,
    __in_opt    LPWSTR   pMonitorName
)
{
	return FALSE;
}

BOOL WINAPI
LcmGetPrinterDataFromPort(
    __in                            HANDLE  hPort,
                                    DWORD   ControlID,
    __in_opt                        LPWSTR  pValueName,
    __in_bcount_opt(cbInBuffer)     LPWSTR  lpInBuffer,
                                    DWORD   cbInBuffer,
    __out_bcount_opt(cbOutBuffer)   LPWSTR  lpOutBuffer,
                                    DWORD   cbOutBuffer,
    __out                           LPDWORD lpcbReturned)
{
	return TRUE;
}

BOOL WINAPI
LcmSetPortTimeOuts(
    __in        HANDLE          hPort,
    __in        LPCOMMTIMEOUTS  lpCTO,
    __reserved  DWORD           reserved)    // must be set to 0
{
	return TRUE;
}

VOID WINAPI
LcmShutdown(
    __in    HANDLE hMonitor
    )
{
    PINILOCALMON    pIniLocalMon    = (PINILOCALMON)hMonitor;
	FreeSplMem(pIniLocalMon);
}

MONITOR2 Monitor2 = {
	sizeof(MONITOR2),
	LcmEnumPorts,
	LcmOpenPort,
	NULL,           // OpenPortEx is not supported
	LcmStartDocPort,
	LcmWritePort,
	LcmReadPort,
	LcmEndDocPort,
	LcmClosePort,
	NULL,           // AddPort is not supported
	LcmAddPortEx,
	NULL,           // ConfigurePort is not supported
	NULL,           // DeletePort is not supported
	LcmGetPrinterDataFromPort,
	LcmSetPortTimeOuts,
	LcmXcvOpenPort,
	LcmXcvDataPort,
	LcmXcvClosePort,
	LcmShutdown
};


/// <summary>
/// 스풀러가 로딩될 때, MONITOR2 구조체에 정보를 채우기 위해 해당 함수를 호출함
/// </summary>
/// <param name="pMonitorInit"></param>
/// <param name="phMonitor"></param>
/// <returns></returns>
LPMONITOR2 WINAPI
InitializePrintMonitor2(
    PMONITORINIT pMonitorInit,
    PHANDLE phMonitor
    )
{
    LPWSTR   pPortTmp = NULL;
    DWORD    rc = 0, i = 0;
    PINILOCALMON pIniLocalMon = NULL;
    LPWSTR   pPorts = NULL;
    DWORD    sRetval = ERROR_SUCCESS;

	// cache pointer
    pIniLocalMon = (PINILOCALMON)AllocSplMem( sizeof( INILOCALMON ));
    if( !pIniLocalMon )
    {
        goto Fail;
    }

    pIniLocalMon->signature = ILM_SIGNATURE;
    pIniLocalMon->pMonitorInit = pMonitorInit;
    g_pMonitorInit = pMonitorInit;

    LcmEnterSplSem();

    //
    // We now have all the ports
    //
    for(pPortTmp = pPorts; pPortTmp && *pPortTmp; pPortTmp += rc + 1){

        rc = (DWORD)wcslen(pPortTmp);

        if (!_wcsnicmp(pPortTmp, L"Ne", 2)) {

            i = 2;

            //
            // For Ne- ports
            //
            if ( rc > 2 && pPortTmp[2] == L'-' )
                ++i;
            for ( ; i < rc - 1 && iswdigit(pPortTmp[i]) ; ++i )
            ;

            if ( i == rc - 1 && pPortTmp[rc-1] == L':' ) {
                continue;
            }
        }

        LcmCreatePortEntry(pIniLocalMon, pPortTmp);
    }

    FreeSplMem(pPorts);

    LcmLeaveSplSem();

    CheckAndAddIrdaPort(pIniLocalMon);

    *phMonitor = (HANDLE)pIniLocalMon;


    return &Monitor2;

Fail:

    FreeSplMem( pPorts );
    FreeSplMem( pIniLocalMon );

    return NULL;
}



BOOL WINAPI
DllMain(
    HINSTANCE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    static BOOL bLocalMonInit = FALSE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        bLocalMonInit = LocalMonInit(hModule);
        DisableThreadLibraryCalls(hModule);
        return TRUE;

    case DLL_PROCESS_DETACH:

        if (bLocalMonInit)
        {
            LocalMonCleanUp();
            bLocalMonInit = FALSE;
        }

        return TRUE;
    }

    UNREFERENCED_PARAMETER(lpRes);

    return TRUE;
}
