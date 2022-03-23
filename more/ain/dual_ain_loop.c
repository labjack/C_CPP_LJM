/**
 * Name: dual_ain_loop.c
 * Desc: Demonstrates reading 2 analog inputs (AINs) in a loop from a LabJack.
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
 *	Multiple Value Functions(such as eWriteNames):
 *		https://labjack.com/support/software/api/ljm/function-reference/multiple-value-functions
 *	Timing Functions(such as StartInterval):
 *		https://labjack.com/support/software/api/ljm/function-reference/timing-functions
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
// ErrorCheck, etc., such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"


void ConfigAIN(const char * namesAIN, int numAIN, int deviceType);

int main()
{
	int err, errorAddress;
	int handle;
	int i;
	int SkippedIntervals;
	int deviceType, ConnectionType, SerialNumber, IPAddress, Port,
		MaxBytesPerMB;
	const int INTERVAL_HANDLE = 1;

	// Set up for reading AIN values
	enum { NUM_FRAMES_AIN = 2 };
	double aValuesAIN[NUM_FRAMES_AIN] = {0};
	const char * aNamesAIN[NUM_FRAMES_AIN] = {"AIN0", "AIN1"};

	int msDelay = 1000;

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

	// Setup and call eWriteNames to configure AIN resolution on the LabJack.
	WriteNameOrDie(handle, "AIN0_RESOLUTION_INDEX", 0);
	WriteNameOrDie(handle, "AIN1_RESOLUTION_INDEX", 0);

	// Range/gain configs only apply to the T7/T8
	if (deviceType != LJM_dtT4) {
		// Range = 10; This corresponds to ±10V (T7), or ±11V (T8)
		WriteNameOrDie(handle, "AIN0_RANGE", 10);
		WriteNameOrDie(handle, "AIN1_RANGE", 10);
	}
	// Negative channel = single ended (199). Only applies to the T7
	if (deviceType == LJM_dtT7) {
		WriteNameOrDie(handle, "AIN0_NEGATIVE_CH", 199);
		WriteNameOrDie(handle, "AIN1_NEGATIVE_CH", 199);
	}

	printf("\nStarting read loop.  Press Ctrl+c to stop.\n");

	err = LJM_StartInterval(INTERVAL_HANDLE, msDelay * 1000);
	ErrorCheck(err, "LJM_StartInterval");

	// Note: The LabJackM (LJM) library will catch the Ctrl+c signal, close
	//       all open devices, then exit the program.
	while (1) {
		// Read AIN from the LabJack
		err = LJM_eReadNames(handle, NUM_FRAMES_AIN, aNamesAIN, aValuesAIN,
			&errorAddress);
		ErrorCheckWithAddress(err, errorAddress, "LJM_eReadNames");

		printf("%s : %f V, %s : %f V\n", aNamesAIN[0], aValuesAIN[0],
			aNamesAIN[1], aValuesAIN[1]);

		err = LJM_WaitForNextInterval(INTERVAL_HANDLE, &SkippedIntervals);
		ErrorCheck(err, "LJM_WaitForNextInterval");
		if (SkippedIntervals > 0) {
			printf("SkippedIntervals: %d\n", SkippedIntervals);
		}
	}

	err = LJM_CleanInterval(INTERVAL_HANDLE);
	PrintErrorIfError(err, "LJM_CleanInterval");

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
