/**
 * Name: open_all_test.c
 * Desc: Iterates through Internal_LJM_OpenAll to calculate the average number of opens
 *       and errors.
 *
 * Relevant Documentation:
 *
 * LJM Library:
 *	LJM Library Installer
 *		 https://labjack.com/support/software/installers/ljm
 *	LJM Users Guide:
 *		https://labjack.com/support/software/api/ljm
 *	Opening and Closing:
 *		https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *	GetHandleInfo:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmgethandleinfo
**/

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <arpa/inet.h>  // For inet_ntoa()
#endif
#include <stdio.h>
#include <string.h>

#include <LabJackM.h>

#include "../../LJM_Utilities.h"

void OpenAll(
	int * numOpened,
	int * aDevTypes,
	int * aConnTypes,
	int * numErrors,
	int * aErrors
) {
	int cou;
	int err;
	int OpenAllDeviceType = LJM_dtANY;
	int OpenAllConnectionType = LJM_ctTCP;
	int devType;
	int connType;

	int aHandles[LJM_LIST_ALL_SIZE];

	err = Internal_LJM_OpenAll(OpenAllDeviceType, OpenAllConnectionType, numOpened,
		aHandles, numErrors, aErrors);
	PrintErrorIfError(
		err,
		"Internal_LJM_OpenAll with device type: %s, connection type: %s",
		NumberToDeviceType(OpenAllDeviceType),
		NumberToConnectionType(OpenAllConnectionType)
	);

	for (cou = 0; cou < *numOpened; cou++) {
		ErrorCheck(
			LJM_GetHandleInfo(aHandles[cou], &devType, &connType, 0, 0, 0, 0),
			"LJM_GetHandleInfo"
		);
		aDevTypes[cou] = devType;
		aConnTypes[cou] = connType;
	}

	err = LJM_CloseAll();
	ErrorCheck(err, "LJM_CloseAll");
}

int main()
{
	int numOpened;
	int numErrors;

	int totOpens = 0;
	int totErrors = 0;
	double avgOpens;
	double avgErrors;

	enum { NUM_ITERS = 10 };
	int opens[NUM_ITERS];
	int errors[NUM_ITERS];

	int aDevTypes[LJM_LIST_ALL_SIZE];
	int aConnTypes[LJM_LIST_ALL_SIZE];
	int aErrors[LJM_LIST_ALL_SIZE];

	int cou;
	int dev;
	int errorI;

	// Collect
	printf("Now performing %d iterations...\n\n", NUM_ITERS);
	printf("iter - opens errors\n");
	for (cou = 0; cou < NUM_ITERS; cou++) {
		numOpened = 0;
		numErrors = 0;
		OpenAll(&numOpened, aDevTypes, aConnTypes, &numErrors, aErrors);

		opens[cou] = numOpened;
		errors[cou] = numErrors;

		totOpens += numOpened;
		totErrors += numErrors;

		printf("% 4d - % 5d % 6d", cou, numOpened, numErrors);

		for (dev = 0; dev < numOpened; dev++) {
			printf(
				" [%s, %s]",
				NumberToDeviceType(aDevTypes[dev]),
				NumberToConnectionType(aConnTypes[dev])
			);
		}

		if (numErrors > 0) {
			printf(" errors: ");
			for (errorI = 0; errorI < numErrors; errorI++) {
				PrintErrorIfError(aErrors[errorI], "  ", errorI);
			}
		}

		printf("\n");
	}

	avgOpens = (double)totOpens / NUM_ITERS;
	avgErrors = (double)totErrors / NUM_ITERS;

	printf("\n");
	printf("avg. - % 5f, % 5f\n", avgOpens, avgErrors);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
