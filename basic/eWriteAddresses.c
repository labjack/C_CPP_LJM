/**
 * Name: eWriteAddresses.c
 * Desc: Shows how to use the LJM_eWriteAddresses function
 *
 * Relevant Documentation:
 *
 * LJM Library:
 *	LJM Library Installer:
 *		https://labjack.com/support/software/installers/ljm
 *	LJM Users Guide:
 *		https://labjack.com/support/software/api/ljm
 *	Opening and Closing:
 *		https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *	eWriteAddresses:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmewriteaddresses
 *	Constants:
 *		https://labjack.com/support/software/api/ljm/constants
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	DAC:
 *		https://labjack.com/support/datasheets/t-series/dac
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../LJM_Utilities.h"

int main()
{
	int err, i, handle;
	int errorAddress = INITIAL_ERR_ADDRESS;

	// write 2.5V to DAC0 and write 12345 to TEST_UINT16 (55110)

	#define NUM_FRAMES 2

	int aAddresses[NUM_FRAMES] = {1000, 55110};
	int aTypes[NUM_FRAMES] = {LJM_FLOAT32, LJM_UINT16};
	double aValues[NUM_FRAMES] = {2.5, 12345};

	// Open first found LabJack
	err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
	ErrorCheck(err, "LJM_Open");

	PrintDeviceInfoFromHandle(handle);

	printf("\nWriting:\n");
	for (i=0; i<NUM_FRAMES; i++) {
		printf("\t%f to address %d (data type: %d)\n", aValues[i], aAddresses[i], aTypes[i]);
	}

	err = LJM_eWriteAddresses(handle, NUM_FRAMES, aAddresses, aTypes, aValues, &errorAddress);
	ErrorCheckWithAddress(err, errorAddress, "LJM_eWriteAddresses");

	err = LJM_Close(handle);
	ErrorCheck(err, "LJM_Close");

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
