#include "precomp.h"


#pragma hdrstop

#include <DriverSpecs.h>
__user_driver

#include "irda.h"

extern WCHAR szVPORT[];

BOOL
IsIRDAInstalled(
    )
{
	return TRUE;
}


VOID
CheckAndAddIrdaPort(
    __in PINILOCALMON    pIniLocalMon
    )
{
    PLCMINIPORT    pIniPort = NULL;


    LcmEnterSplSem();

    for ( pIniPort = pIniLocalMon->pIniPort ;
          pIniPort && !IS_IRDA_PORT(pIniPort->pName) ;
          pIniPort = pIniPort->pNext )
    ;

    LcmLeaveSplSem();

    if ( pIniPort || !IsIRDAInstalled() )
        return;

    //
    // Add the port to the list and write to registry
    //
    LcmCreatePortEntry(pIniLocalMon, szVPORT);
}

VOID
CloseIrdaConnection(
    __in    PLCMINIPORT    pIniPort
    )
{
}

