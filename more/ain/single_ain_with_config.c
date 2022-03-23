/**
 * Name: single_ain_with_config.c
 * Desc: Demonstrates configuring and reading a single analog input (AIN) with a LabJack.
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
 *	Multiple Value Functions(such as eWriteNames):
 *		https://labjack.com/support/software/api/ljm/function-reference/multiple-value-functions
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Analog Inputs:
 *		https://labjack.com/support/datasheets/t-series/ain
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
	int i;
	int deviceType, ConnectionType, SerialNumber, IPAddress, Port,
		MaxBytesPerMB;

	// Set up for reading AIN value
	double value = 0;
	const char * NAME = "AIN0";

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	// Get device info
	err = LJM_GetHandleInfo(handle, &deviceType, &ConnectionType,
		&SerialNumber, &IPAddress, &Port, &MaxBytesPerMB);
	ErrorCheck(err,
		"PrintDeviceInfoFromHandle (LJM_GetHandleInfo)");

	PrintDeviceInfo(deviceType, ConnectionType, SerialNumber, IPAddress, Port,
		MaxBytesPerMB);
	printf("\n");

	// Setup and call eWriteNames to configure AIN resolution on the LabJack.
	WriteNameOrDie(handle, "AIN0_RESOLUTION_INDEX", 0);
	// Range/gain configs only apply to the T7/T8
	if (deviceType != LJM_dtT4) {
		// Range = 10; This corresponds to ±10V (T7), or ±11V (T8)
		WriteNameOrDie(handle, "AIN0_RANGE", 10);
	}
	// Negative channel = single ended (199). Only applies to the T7
	if (deviceType == LJM_dtT7) {
		WriteNameOrDie(handle, "AIN0_NEGATIVE_CH", 199);
	}


	// Read AIN0 from the LabJack
	err = LJM_eReadName(handle, NAME, &value);
	ErrorCheck(err, "LJM_eReadName");

	// Print results
	printf("\n%s : %f V\n", NAME, value);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
