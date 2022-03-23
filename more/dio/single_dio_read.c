/**
 * Name: single_dio_read.c
 * Desc: Demonstrates how to read a single digital input/output.
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
 *	eReadName:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmereadname
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Digital I/O:
 *		https://labjack.com/support/datasheets/t-series/digital-io
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

	// Set up for reading DIO state
	double value = 0;
	char * name;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	if (GetDeviceType(handle) == LJM_dtT4) {
		// Reading from FIO4 on the LabJack T4. FIO0-FIO3 are reserved for
		// AIN0-AIN3. Note: Reading a single digital I/O will change the line
		// from analog to digital input.
		name = "FIO4";
	}
	else {
		// Reading from FIO0 on the LabJack T7 and T8
		name = "FIO0";
	}

	// Read DIO state from the LabJack
	err = LJM_eReadName(handle, name, &value);
	ErrorCheck(err, "LJM_eReadName");

	printf("\n%s state : %f\n", name, value);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
