/**
 * Name: read_device_name_string.c
 * Desc: Demonstrates how to read the device name string from a LabJack.
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
 *	eReadNameString:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmereadnamestring
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
	char allocatedString[LJM_STRING_ALLOCATION_SIZE];

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	err = LJM_eReadNameString(handle, NAME_REGISTER, allocatedString);
	ErrorCheck(err, "LJM_eReadNameString");

	printf("\n%s : %s\n", NAME_REGISTER, allocatedString);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
