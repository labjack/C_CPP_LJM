/**
 * Name: single_dio_write.c
 * Desc: Demonstrates how to set a single digital state on a LabJack.
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
 *	eWriteName:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmewritename
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

	// Set up for setting DIO state
	double value = 0; // Output state = low (0 = low, 1 = high)
	char * name;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	if (GetDeviceType(handle) == LJM_dtT4) {
		// Setting FIO4 on the LabJack T4. FIO0-FIO3 are reserved for AIN0-AIN3.
		name = "FIO4";

		// If the FIO/EIO line is an analog input, it needs to first be changed
		// to a digital I/O by reading from the line or setting it to digital
		// I/O with the DIO_ANALOG_ENABLE register.
		// For example:
		// 	double temp;
		// 	LJM_eReadName(handle, name, &temp);
	}
	else {
		// Setting FIO0 on the LabJack T7 and T8
		name = "FIO0";
	}

	// Set DIO state on the LabJack
	err = LJM_eWriteName(handle, name, value);
	ErrorCheck(err, "LJM_eWriteName");

	printf("\nSet %s state : %f\n", name, value);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
