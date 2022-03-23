/**
 * Name: auto_reconnect_test.c
 * Desc: Tests the LJM auto reconnect functionality.
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
 *	LJME_RECONNECT_FAILED:
 *		https://labjack.com/support/software/api/ljm/ljmereconnectfailed
 *	Library Configuration Functions:
 *		https://labjack.com/support/software/api/ljm/function-reference/library-configuration-functions
 *	Timeout Configs:
 *		https://labjack.com/support/software/api/ljm/constants/timeout-configs
 *	RegisterDeviceReconnectCallback:
 *		https://labjack.com/support/software/api/ljm/function-reference/LJMRegisterDeviceReconnectCallback
 *	Single Value Functions(such as eReadName):
 *		https://labjack.com/support/software/api/ljm/function-reference/single-value-functions
 *
 *	T-Series and I/O:
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

void ReconnectCallback(int handle)
{
	printf("Reconnected handle: %d\n", handle);
}

int main()
{
	int err, handle;
	int iteration = 0;
	int DeviceType, ConnectionType, SerialNumber, IPAddress, Port, MaxBytesPerMB;
	double value = 0;

	const char * NAME = {"SERIAL_NUMBER"};

	GetAndPrintConfigValue(LJM_LIBRARY_VERSION);

	// Set the timeouts shorter for testing convenience
	SetConfigValue(LJM_OPEN_TCP_DEVICE_TIMEOUT_MS, 500);
	SetConfigValue(LJM_SEND_RECEIVE_TIMEOUT_MS, 500);

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	err = LJM_RegisterDeviceReconnectCallback(handle, &ReconnectCallback);
	ErrorCheck(err, "LJM_RegisterDeviceReconnectCallback");

	printf("Press control + c to exit.\n");
	while (1) {
		printf("\n");
		printf("iteration: %d\n", iteration++);

		err = LJM_eReadName(handle, NAME, &value);
		PrintErrorIfError(err, "LJM_eReadName");
		if (err == LJME_NOERROR) {
			printf("%s: %.0f\n", NAME, value);

			err = LJM_GetHandleInfo(handle, &DeviceType, &ConnectionType,
				&SerialNumber, &IPAddress, &Port, &MaxBytesPerMB);
			PrintErrorIfError(err, "LJM_GetHandleInfo");
			if (err == LJME_NOERROR) {
				PrintDeviceInfo(DeviceType, ConnectionType, SerialNumber, IPAddress,
					Port, MaxBytesPerMB);
			}
		}

		printf("unplug, replug, wait\n");
		WaitForUser();
	}

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
