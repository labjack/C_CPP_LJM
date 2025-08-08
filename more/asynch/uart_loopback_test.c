/**
 * Name: uart_loopback_test.c
 * Desc: Simple Asynch example uses the first found device and 9600/8/N/1.
 *       Does a write, waits 1 second, then returns whatever was read in that
 *       time. If you short RX to TX, then you will read back the same bytes
 *       that you write.
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
 *	Multiple Value Functions (such as eWriteNameArray and eReadNameArray):
 *		https://labjack.com/support/software/api/ljm/function-reference/multiple-value-functions
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Asynchronous Serial:
 *		https://labjack.com/support/datasheets/t-series/digital-io/asynchronous-serial
**/

#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

void PrintAsBytes(int numValues, const double * values);

int main()
{
	int err;
	int handle;
	enum {NUM_BYTES=4};
	const double writeValues[NUM_BYTES] = {0x12, 0x34, 0x56, 0x78};
	double readValues[NUM_BYTES];
	int deviceType, ConnectionType, SerialNumber, IPAddress, Port,
		MaxBytesPerMB;
	double rxDioNum;
	double txDioNum;

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
	
	if(deviceType == LJM_dtT4) {
		rxDioNum = 4;  // RX pin number = 4 (FIO4)
		txDioNum = 5;  // TX pin number = 5 (FIO5)
	}
	else {
		rxDioNum = 0;  // RX pin number = 0 (FIO0)
		txDioNum = 1;  // TX pin number = 1 (FIO1)
	}

	printf("Short FIO%0.0f and FIO%0.0f together to read back the same bytes:\n\n", rxDioNum, txDioNum);

	// Configure for loopback
	WriteNameOrDie(handle, "ASYNCH_ENABLE", 0);
	WriteNameOrDie(handle, "ASYNCH_RX_DIONUM", rxDioNum);
	WriteNameOrDie(handle, "ASYNCH_TX_DIONUM", txDioNum);
	WriteNameOrDie(handle, "ASYNCH_BAUD", 9600);
	WriteNameOrDie(handle, "ASYNCH_NUM_DATA_BITS", 8);
	WriteNameOrDie(handle, "ASYNCH_PARITY", 0);
	WriteNameOrDie(handle, "ASYNCH_NUM_STOP_BITS", 1);
	WriteNameOrDie(handle, "ASYNCH_ENABLE", 1);

	// Write
	printf("Writing: ");
	PrintAsBytes(4, writeValues);
	WriteNameOrDie(handle, "ASYNCH_NUM_BYTES_TX", NUM_BYTES);
	WriteNameArrayOrDie(handle, "ASYNCH_DATA_TX", NUM_BYTES, writeValues);

	WriteNameOrDie(handle, "ASYNCH_TX_GO", 1);

	MillisecondSleep(1000);

	// Read
	ReadNameArrayOrDie(handle, "ASYNCH_DATA_RX", NUM_BYTES, &readValues);
	printf("Read:    ");
	PrintAsBytes(NUM_BYTES, readValues);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}

void PrintAsBytes(int numValues, const double * values)
{
	int iter;
	printf("0x");
	for (iter = 0; iter < numValues; iter++) {
		printf("%02x ", (int)values[iter]);
	}
	printf("\n");
}
