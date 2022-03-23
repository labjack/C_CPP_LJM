/**
 * Name: in_stream_with_non_looping_out_stream_supplement.h
 * Desc: class for holding stream-out buffer information and associated functions
 * 
 * targetAddress: holds the target registers address
 * stateInfo: object that holds a state string and an array of data values 
 * currentIndex: int tracking which is the current state
 * states: vector for storing the states
 * stateSize: int describing how big each state's "values" list is
 * targetType_str: str used to generate this dict's "names" list
 * target: str name of the register to update during stream-out
 * bufferNumBytes: int number of bytes of this stream-out buffer
 * setLoop: int number to be written to to STREAM_OUT#(0:3)_SET_LOOP
 * streamOutIndex: int number of this stream-out
 *
 * streamOutReg variables hold the names of registers for out-streams
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
 *	LJM Single Value Functions (like eReadName, eReadAddress): 
 *		https://labjack.com/support/software/api/ljm/function-reference/single-value-functions
 *	Stream Functions (eStreamRead, eStreamStart, etc.):
 *		https://labjack.com/support/software/api/ljm/function-reference/stream-functions
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Stream Mode:
 *		https://labjack.com/support/datasheets/t-series/communication/stream-mode
 *	Analog Inputs:
 *		https://labjack.com/support/datasheets/t-series/ain
 *	Stream-Out:
 *		https://labjack.com/support/datasheets/t-series/communication/stream-mode/stream-out/stream-out-description
 *	Digital I/O:
 *		https://labjack.com/support/datasheets/t-series/digital-io
 *	DAC:
 *		https://labjack.com/support/datasheets/t-series/dac
**/

#ifndef IN_STREAM_WITH_NON_LOOPING_OUT_STREAM_SUPPLEMENT_H
#define IN_STREAM_WITH_NON_LOOPING_OUT_STREAM_SUPPLEMENT_H

#include <vector>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "../../../../LJM_StreamUtilities.h"
using namespace std;

// holds the stream-out buffer information that needs to be defined by 
// the program user
struct stream_outs { 
	string target;
	int bufferNumBytes;
	int streamOutIndex;
	int setLoop;
	string indexString;
};

const double INITIAL_SCAN_RATE_HZ = 200; // desired scan rate for the streams
const int NUM_CYCLES = 10;
enum {NUM_IN_READS = 2};
enum { NUM_SCAN_ADDRESSES = 4 }; // the total number of in and out streams
enum { NUM_STREAM_OUTS = 2 };
enum { BUFFER_NUM_BYTES = 512 };

// the register names of the in-streams
const char *InListString[NUM_IN_READS] = { "AIN0", "AIN1"}; 
const char *scanListString[NUM_SCAN_ADDRESSES] = { "AIN0", "AIN1",
	"STREAM_OUT0", "STREAM_OUT1" };
struct stream_outs streamOuts[NUM_STREAM_OUTS];

int aScanList[NUM_SCAN_ADDRESSES];
int aTypes[NUM_SCAN_ADDRESSES];
void SetupStreamInfo();
void PrintRegisterValue(int&, const char *);
void PrepareForExit(int&);
int ProcessStreamResults(int, vector<double>, int, int, int);

void SetupStreamInfo() {
	//initialize stream outs
	streamOuts[0].target = "DAC0";
	streamOuts[0].bufferNumBytes = BUFFER_NUM_BYTES;
	streamOuts[0].streamOutIndex = 0;
	streamOuts[0].indexString = "0";
	// set_loop 2 waits to use new buffer data until another out-stream is
	// set to synch
	streamOuts[0].setLoop = 2; 


	streamOuts[1].target = "DAC1";
	streamOuts[1].bufferNumBytes = BUFFER_NUM_BYTES;
	streamOuts[1].streamOutIndex = 1;
	streamOuts[1].indexString = "1";
	// set_loop 3 = synch. starts using new buffer data immediately
	streamOuts[1].setLoop = 3;

	// get address and type information for the registers to scan
	int err = 0;
	err = LJM_NamesToAddresses(NUM_SCAN_ADDRESSES, scanListString,
		aScanList, aTypes);
	ErrorCheck(err, "LJM_NamesToAddresses scan list");
}

//******************************************************************************
// printRegisterValue
//
//	int handle: handle for the LabJack device you want to read registers
//	from
//
//	int registerAddress: physical address of register
//	
//	int registerType: data type of the register being read
//******************************************************************************
void PrintRegisterValue(int &handle, const char * registerName) {
	double registerValue;
	LJM_eReadName(handle, registerName, &registerValue);
	printf("%s = %f \n", registerName, registerValue);
}

//******************************************************************************
// PrepareForExit
//
// Desc: closes the streams that are open and closes the link to the 
// labjack device
//
//	int handle: handle for the LabJack device you want to read registers
//	from
//******************************************************************************
void PrepareForExit(int &handle) {
	printf("Stopping stream...\n");
	int err = LJM_eStreamStop(handle);
	ErrorCheck(err, "Problem closing stream");

	err = LJM_Close(handle);
	ErrorCheck(err, "Problem closing device");

}
//******************************************************************************
// ProcessStreamResults
//
// Desc: checks for any skipped samples for the out-stream and checks 
// for backlog scans
//
//	int iteration: the buffer update cycle number
//
//	vector<double> data: vector of data sent to the outstream buffer
//	
//	int deviceNumBacklogScans: The number of scans left in the device 
//  buffer, as measured from when data was last collected from the 
//	device. DeviceScanBacklog should usually be near zero and not growing.
//
//	int ljmNumBacklogScans: The number of scans left in the LJM buffer,
//	which does not include concurrent data sent in the aData array. 
//	LJMScanBacklog should usually be near zero and not growing.
//
//	int stateSize: the size of the states values array
//******************************************************************************

int ProcessStreamResults(int iteration, vector<double> data, 
			int deviceNumBacklogScans, int ljmNumBacklogScans, int stateSize) {
	int numScans = data.size() / NUM_IN_READS;
	printf("\n");
	/*Count the skipped samples which are indicated by - 9999 values.Missed
	samples occur after a device's stream buffer overflows and are
	reported after auto - recover mode ends.*/
	int numSkippedSamples = count(data.begin(), data.end(), -9999.0);
	printf("eStreamRead %d\n", iteration);
	printf("	1st scan out of %d:", numScans);
	for (int i = 0; i < NUM_IN_READS; i++) {
		printf(" %s = %f", InListString[i], data[i]);
		if (i < NUM_IN_READS - 1) printf(", ");
		else printf("\n");
	}
	if (numSkippedSamples > 0) {
		printf("  **** Samples skipped = %d (of %d) **** \n",
			numSkippedSamples, data.size());
	}
	if (deviceNumBacklogScans > stateSize) {
		printf("Device scan backlog = %d ", deviceNumBacklogScans);
	}
	if (ljmNumBacklogScans > stateSize) {
		printf("LJM scan backlog = %d", ljmNumBacklogScans);
	}
	return numSkippedSamples;
}
#endif