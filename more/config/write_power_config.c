/**
 * Name: write_power_config.c
 * Desc: Demonstrates how to configure default power settings on a LabJack.
 * Note: This example is not supported on the T8.
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
 *	eWriteNames:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmewritenames
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	WiFi:
 *		https://labjack.com/support/datasheets/t-series/wifi
 *	Ethernet:
 *		https://labjack.com/support/datasheets/t-series/ethernet
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

	// Set up operation
	enum { NUM_FRAMES = 4};
	const char * aNames[NUM_FRAMES] = {"POWER_ETHERNET_DEFAULT", "POWER_WIFI_DEFAULT",
		"POWER_AIN_DEFAULT", "POWER_LED_DEFAULT"};
	double aValues[NUM_FRAMES] = {1, 0, 1, 1};
	int errorAddress = INITIAL_ERR_ADDRESS;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	// Perform operation
	err = LJM_eWriteNames(handle, NUM_FRAMES, aNames, aValues, &errorAddress);
	ErrorCheckWithAddress(err, errorAddress, "LJM_eWriteNames");

	printf("\nConfigurations set:\n");
	for (i=0; i<NUM_FRAMES; i++) {
		printf("    %s : %.0f\n", aNames[i], aValues[i]);
	}

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
