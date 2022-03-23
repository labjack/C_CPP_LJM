/**
 * Name: eWriteAddress.c
 * Desc: Shows how to use the LJM_eWriteAddress function
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
 *	eWriteAddress:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmewriteaddress
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
	int err, handle;

	// write 2.5V to DAC0

	const int ADDRESS = 1000; // DAC0
	const int TYPE = LJM_FLOAT32;
	double value = 2.5;

	// Open first found LabJack
	err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
	ErrorCheck(err, "LJM_Open");

	PrintDeviceInfoFromHandle(handle);

	printf("\nWriting %f to address %d (data type: %d)\n", value, ADDRESS, TYPE);

	err = LJM_eWriteAddress(handle, ADDRESS, TYPE, value);
	ErrorCheck(err, "LJM_eWriteAddress");

	err = LJM_Close(handle);
	ErrorCheck(err, "LJM_Close");

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
