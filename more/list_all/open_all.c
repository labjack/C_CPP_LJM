/**
 * Name: open_all.c
 * Desc: Shows how to use Internal_LJM_OpenAll
 *
 * Relevant Documentation:
 *
 * LJM Library:
 *	LJM Library Installer
 *		 https://labjack.com/support/software/installers/ljm
 *	LJM Users Guide:
 *		https://labjack.com/support/software/api/ljm
 *	Opening and Closing:
 *		https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *	GetHandleInfo:
 *		https://labjack.com/support/software/api/ljm/function-reference/ljmgethandleinfo
 *	NumberToIP:
		https://labjack.com/support/software/api/ljm/function-reference/utility/ljmnumbertoip

**/

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <arpa/inet.h>  // For inet_ntoa()
#endif
#include <stdio.h>
#include <string.h>

#include <LabJackM.h>

#include "../../../header_files/api/InternalLabJackM.h"

#include "../../LJM_Utilities.h"

int GetDeviceInfo(const char * devType, const char * connType, const char * iden)
{
	int err;
	int handle;
	int portOrPipe, ipAddress, serialNumber, packetMaxBytes, deviceType, connectionType;

	printf("LJM_OpenS(\"%s\", \"%s\", \"%s\")\n", devType, connType, iden);

	err = LJM_OpenS(devType, connType, iden, &handle);
	ErrorCheck(err, "LJM_OpenS");

	err = LJM_GetHandleInfo(handle, &deviceType, &connectionType, &serialNumber, &ipAddress,
		&portOrPipe, &packetMaxBytes);
	ErrorCheck(err, "LJM_GetHandleInfo");

	PrintDeviceInfo(deviceType, connectionType, serialNumber, ipAddress, portOrPipe, packetMaxBytes);
	printf("\n");
	printf("\n");
	return handle;
}

void OpenAll(int OpenAllDeviceType, int OpenAllConnectionType, int numExpected)
{
	int devType, connType;
	int err;
	int serialNumber;
	int IPAddress, port, maxBytesPerMB;

	int numOpened = 0;
	int aHandles[LJM_LIST_ALL_SIZE];
	int deviceI;

	int numErrors = 0;
	int infoHandle = 1;
	const char * Info;

	char IPv4String[LJM_IPv4_STRING_SIZE];

	printf("Calling Internal_LJM_OpenAll with device type: %s, connection type: %s\n",
		NumberToDeviceType(OpenAllDeviceType),
		NumberToConnectionType(OpenAllConnectionType));
	err = Internal_LJM_OpenAll(OpenAllDeviceType, OpenAllConnectionType, &numOpened,
		aHandles, &numErrors, &infoHandle, &Info);
	ErrorCheck(err, "Internal_LJM_OpenAll with device type: %s, connection type: %s",
		NumberToDeviceType(OpenAllDeviceType),
		NumberToConnectionType(OpenAllConnectionType));

	printf("%d errors occurred during OpenAll\n", numErrors);
	printf("Info: %s\n", Info);

	printf("Opened %d device connections\n", numOpened);
	for (deviceI = 0; deviceI < numOpened; deviceI++) {
		err = LJM_GetHandleInfo(aHandles[deviceI], &devType,
			&connType, &serialNumber, &IPAddress, &port,
			&maxBytesPerMB);
		ErrorCheck(err, "LJM_GetHandleInfo");

		printf(
			"    [%3d] - aDeviceTypes: %s, aConnectionTypes: %s\n",
			deviceI,
			NumberToDeviceType(devType),
			NumberToConnectionType(connType)
		);
		printf("           aSerialNumber: %d", serialNumber);

		if (IsNetwork(connType)) {
			err = LJM_NumberToIP(IPAddress, IPv4String);
			ErrorCheck(err, "LJM_NumberToIP");
			printf(", aIPAddresses: %s", IPv4String);
			printf(", port: %d", port);
		}

		printf("\n");
		if (GetDeviceType(aHandles[deviceI]) != LJM_dtDIGIT) {
			printf("           ");GetAndPrint(aHandles[deviceI], "AIN2");
		}
	}

	ErrorCheck(LJM_CleanInfo(infoHandle), "LJM_CleanInfo");

	if (numOpened != numExpected) {
		printf("!!!!!!!!!!!! Expected %d devices but got %d\n", numExpected, numOpened);
	}

	if (numErrors > 0) {
		printf("!!!!!!!!!!!! error happened\n\n\n");
	}

	printf("\n");
	printf("\n");
}

int main()
{
	int numDevsExpected = 2;

	GetAndPrintConfigValue(LJM_LIBRARY_VERSION);

	OpenAll(LJM_dtANY, LJM_ctANY, numDevsExpected);

	// Devices should be closed as soon as possible after Internal_LJM_OpenAll, since
	// no other processes will be able to access the devices while they are
	// opened. You can close all devices with LJM_CloseAll or close each device
	// that isn't needed using LJM_Close.
	ErrorCheck(LJM_CloseAll(), "LJM_CloseAll");

	WaitForUserIfWindows();

	return LJME_NOERROR;
}
