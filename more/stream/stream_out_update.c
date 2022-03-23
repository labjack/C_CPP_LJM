/**
 * Name: stream_out_update.c
 * Desc: This program demonstrates updating the Stream Out loop to have a
 *       constant value of 1.23. It can be run concurrently with
 *       stream_basic_with_stream_out or stream_out_only, for example.
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
 *	Stream Functions:
 *		https://labjack.com/support/software/api/ljm/function-reference/stream-functions
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Stream Mode: 
 *		https://labjack.com/support/datasheets/t-series/communication/stream-mode
 *	Stream-Out: 
 *		https://labjack.com/support/datasheets/t-series/communication/stream-mode/stream-out/stream-out-description
**/

#include "../../LJM_Utilities.h"

void UpdateStreamOut(int handle)
{
	WriteNameOrDie(handle, "STREAM_OUT0_LOOP_SIZE", 1);
	WriteNameOrDie(handle, "STREAM_OUT0_BUFFER_F32", 1.23);
	WriteNameOrDie(handle, "STREAM_OUT0_SET_LOOP", 1);
}

int main()
{
	int handle;

	// Open first found LabJack. If the other stream out program is
	// running on a USB connection, this program will need to connect via
	// Ethernet or WiFi, since only one program can claim a LabJack's USB
	// connection at a time.
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	UpdateStreamOut(handle);
	CloseOrDie(handle);

	return LJME_NOERROR;
}
