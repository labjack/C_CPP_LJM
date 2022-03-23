/**
 * Name: write_device_name_string.c
 * Desc: Demonstrates how to write the device name string to a LabJack.
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
 *	eWriteNameString:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmewritenamestring
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Hardware Overview(Device Information Registers):
 *		https://labjack.com/support/datasheets/t-series/hardware-overview
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

int main()
{
	int err;
	int handle;

	const char * NAME_REGISTER = "DEVICE_NAME_DEFAULT";
	const char * NAME_TO_WRITE = "My Favorite LabJack Device";

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	// Write
	printf("\nWriting \"%s\" to %s\n", NAME_TO_WRITE, NAME_REGISTER);
	err = LJM_eWriteNameString(handle, NAME_REGISTER, NAME_TO_WRITE);
	ErrorCheck(err, "LJM_eWriteNameString");

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
