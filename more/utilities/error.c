/**
 * Name: error.c
 * Desc: Demonstrates LJM_ErrorToString
 *
 * Relevant Documentation:
 *
 * LJM Library:
 *	LJM Library Installer:
 *		https://labjack.com/support/software/installers/ljm
 *	LJM Users Guide:
 *		https://labjack.com/support/software/api/ljm
 *	LJM_ErrorToString:
 *		https://labjack.com/support/software/api/ljm/function-reference/utility/ljmerrortostring
 *	Constants:
 *		https://labjack.com/support/software/api/ljm/constants
**/

#include <stdio.h>
#include <LabJackM.h>

#include "../../LJM_Utilities.h"

void PrintErrorToString(int err)
{
	char errName[LJM_MAX_NAME_SIZE];
	LJM_ErrorToString(err, errName);
	printf("LJM_ErrorToString(%d) returned %s\n", err, errName);
}

int main()
{
	printf("Manual values:\n");
	PrintErrorToString(0);
	PrintErrorToString(LJME_CONSTANTS_FILE_NOT_FOUND);
	PrintErrorToString(LJME_INVALID_CONSTANTS_FILE);
	PrintErrorToString(LJME_TRANSACTION_ID_ERR);
	PrintErrorToString(LJME_WARNINGS_BEGIN);
	PrintErrorToString(LJME_U3_NOT_SUPPORTED_BY_LJM);
	PrintErrorToString(199); // non-existent error
	PrintErrorToString(2330); // LabJack device error

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
