/**
 * Name: read_ethernet_mac.c
 * Desc: Demonstrates how to read the Ethernet MAC from a LabJack.
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
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
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
	int handle;
	const char * MAC_NAME = "ETHERNET_MAC";
	const int MAC_ADDRESS = 60020;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);
	printf("\n");

	// See LJM_Utilities.h for more information
	GetAndPrintMACAddressFromValueAddress(handle, MAC_NAME, MAC_ADDRESS);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
