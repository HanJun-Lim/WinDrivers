// printer_setup.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <windows.h>

int main()
{
	DRIVER_INFO_3	DriverInfo3 = { 0, };
	PRINTER_INFO_2	PrinterInfo = { 0, };

	BOOL bRet = FALSE;
	DWORD dwRet;
	TCHAR CurrentDirectory[MAX_PATH] = { 0, };
	TCHAR OSPrinterDriverDirectory[MAX_PATH] = { 0, };

	TCHAR SystemUniDrvPath[MAX_PATH] = { 0, };
	TCHAR SystemUniDrvUIPath[MAX_PATH] = { 0, };
	TCHAR CurrentGPDFilePath[MAX_PATH] = { 0, };

	TCHAR CopiedUniDrvPath[MAX_PATH] = { 0, };
	TCHAR CopiedUniDrvUIPath[MAX_PATH] = { 0, };
	TCHAR CopiedGPDFilePath[MAX_PATH] = { 0, };

	TCHAR	szPort[MAX_PATH] = { 0, };
	HANDLE	hPrinter = 0;

	bRet = GetPrinterDriverDirectory(NULL, NULL, 1, (LPBYTE)OSPrinterDriverDirectory, MAX_PATH, &dwRet);
	if (bRet == FALSE)
		goto exit;

	GetCurrentDirectory(MAX_PATH, CurrentDirectory);

	_stprintf_s(CopiedUniDrvPath, MAX_PATH, _T("%s\\%s"), OSPrinterDriverDirectory, _T("UNIDRV.DLL"));
	_stprintf_s(CopiedUniDrvUIPath, MAX_PATH, _T("%s\\%s"), OSPrinterDriverDirectory, _T("UNIDRVUI.DLL"));
	_stprintf_s(CopiedGPDFilePath, MAX_PATH, _T("%s\\%s"), OSPrinterDriverDirectory, _T("HAJEPRINTER.GPD"));

	_stprintf_s(SystemUniDrvPath, MAX_PATH, _T("%s\\3\\%s"), OSPrinterDriverDirectory, _T("UNIDRV.DLL"));
	_stprintf_s(SystemUniDrvUIPath, MAX_PATH, _T("%s\\3\\%s"), OSPrinterDriverDirectory, _T("UNIDRVUI.DLL"));
	_stprintf_s(CurrentGPDFilePath, MAX_PATH, _T("%s\\%s"), CurrentDirectory, _T("HAJEPRINTER.GPD"));

	if (CopyFile(CurrentGPDFilePath, CopiedGPDFilePath, FALSE) == FALSE)
		goto exit;

	if (CopyFile(SystemUniDrvPath, CopiedUniDrvPath, FALSE) == FALSE)
		goto exit;

	if (CopyFile(SystemUniDrvUIPath, CopiedUniDrvUIPath, FALSE) == FALSE)
		goto exit;

	// GPD File, UNIDRV.DLL, UNIDRVUI.DLL 모두 OSPrinterDriverDirectory 폴더로 복사가 되었습니다

	// 프린터드라이버를 설치합니다
	DriverInfo3.cVersion = 0x03;
	DriverInfo3.pName = _T("HAJESOFT PRINTER");
	DriverInfo3.pEnvironment = NULL;
	DriverInfo3.pDriverPath = CopiedUniDrvPath;
	DriverInfo3.pDataFile = CopiedGPDFilePath;
	DriverInfo3.pConfigFile = CopiedUniDrvUIPath;
	DriverInfo3.pHelpFile = NULL;
	DriverInfo3.pDependentFiles = NULL;
	DriverInfo3.pMonitorName = NULL;
	DriverInfo3.pDefaultDataType = _T("RAW");

	bRet = AddPrinterDriver(NULL, 3, (LPBYTE)&DriverInfo3);
	if (bRet == FALSE)
		goto exit;


	// 프린터장치를 설치합니다
	_stprintf_s(szPort, MAX_PATH, _T("LPT1:")); // 프린터포트를 지정합니다

	PrinterInfo.pPrinterName = _T("HAJESOFT PRINTER");
	PrinterInfo.pPortName = szPort;
	PrinterInfo.pDriverName = _T("HAJESOFT PRINTER");
	PrinterInfo.pPrintProcessor = _T("winprint");

	hPrinter = AddPrinter(NULL, 2, (LPBYTE)&PrinterInfo);

	if (hPrinter != 0)
	{
		ClosePrinter(hPrinter);
	}
	else
	{
		bRet = FALSE;
		goto exit;
	}

	bRet = TRUE;

exit:
	if( bRet == FALSE)
		return -1;
	else
		return 0;
}

