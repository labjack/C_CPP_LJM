/**
 * Name: advanced_aperiodic_stream_out.cpp
 * Desc: Demonstrates setting up stream-in while manually setting up
 *		 aperiodic stream-out.
 * Note: the LJM aperiodic stream-out functions are recommended for
 *		 most use cases that require aperiodic stream-out
 *
 * Streams in while streaming out arbitrary values. These arbitrary stream-out
 * values act on DAC0 to alternate between increasing the voltage from 0 to 2.5 and
 * decreasing from 5.0 to 2.5 on (approximately). Though these values are initially
 * generated during the call to create_streamOut, the values could be
 * dynamically generated, read from a file, etc. To convert this example file into
 * a program to suit your needs, the primary things you need to do are:
 *
 *  1. Edit the global setup variables in this file
 *  2. Define your own streamOut object or equivalent
 *  3. Define your own process_stream_results function or equivalent
 *
 * You may also need to configure AIN, etc.
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

#include <cmath>
#include <vector>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include "../../LJM_StreamUtilities.h"
#include <LabJackM.h>

// Holds the stream-out buffer information that needs to be defined by 
// the program user
struct StreamInfo { 
	std::string target;
	int bufferNumBytes;
	int streamOutIndex;
	int setLoop;
	std::string indexString;
};

class StreamOut {

public:
	static const int BYTES_PER_VALUE = 2;
	// Device information; see LJM_GetHandleInfo()
	int deviceType; 
	int connectionType; 
	int serialNum;
	int ip;
	int port;
	int maxBytes;
	int handle;
	int targetAddress;
	StreamOut(StreamInfo myStreamOut, int &handle);
	void ReadBufferStatus();
	void PrintUpdateInfo();
	void CheckUpdateBuffer();
	int GetStreamAddress();
	int GetStateSize();

private:
	struct StateInfo {
		std::string stateString;
		std::vector<double> values;
	};
	int currentIndex;
	std::vector<StateInfo> states;
	int stateSize;
	std::string targetTypeString;
	std::string target;
	int bufferNumBytes;
	int setLoop;
	std::string streamOutIndex;
	std::string streamOutRegString;
	std::string targetRegString;
	std::string bufferSizeRegString;
	std::string loopSizeRegString;
	std::string setLoopRegString;
	std::string bufferStatusRegString;
	std::string enableRegString;
	std::string bufferRegString;
	std::vector<double> GenerateState(double startVal, double changeInVal);
	int maxSamples;
	void InitializeStreamOut();
	void CreateStreamOutNames();
	void ConvertNameToOutBufferTypeStr();
	void UpdateStreamOutBuffer(bool isStreamRunning);
};

void PrintRegisterValue(int&, const char *);
void PrepareForExit(int&);
int ProcessStreamResults(int, std::vector<double>, int, int, int);

//******************************************************************************
// 									Globals
//******************************************************************************
const double INITIAL_SCAN_RATE_HZ = 200; // Desired scan rate for the streams
const int NUM_CYCLES = 10;
enum {NUM_IN_READS = 2};
enum { NUM_SCAN_ADDRESSES = 4 }; // The total number of in and out streams
enum { NUM_STREAM_OUTS = 2 };
enum { BUFFER_NUM_BYTES = 512 };
// The register names of the in-streams
const char *InListString[NUM_IN_READS] = { "AIN0", "AIN1"}; 
const char *scanListString[NUM_SCAN_ADDRESSES] = { 
	"AIN0", 
	"AIN1",
	"STREAM_OUT0", 
	"STREAM_OUT1" 
};

//******************************************************************************
// 									Main
//******************************************************************************
int main() {
	int aScanList[NUM_SCAN_ADDRESSES];
	int aTypes[NUM_SCAN_ADDRESSES];
	// Setup stream out information
	struct StreamInfo streamInfo[NUM_STREAM_OUTS];
	streamInfo[0].target = "DAC0";
	streamInfo[0].bufferNumBytes = BUFFER_NUM_BYTES;
	streamInfo[0].streamOutIndex = 0;
	streamInfo[0].indexString = "0";
	// SetLoop = 2 waits to use new buffer data until another out-stream is
	// set to synch
	streamInfo[0].setLoop = 2; 
	streamInfo[1].target = "DAC1";
	streamInfo[1].bufferNumBytes = BUFFER_NUM_BYTES;
	streamInfo[1].streamOutIndex = 1;
	streamInfo[1].indexString = "1";
	// SetLoop = 3 = synch. Starts using new buffer data immediately
	streamInfo[1].setLoop = 3;
	int err = 0;
	// get the addresses and types of the registers in the scan list
	err = LJM_NamesToAddresses(NUM_SCAN_ADDRESSES, scanListString,
		aScanList, aTypes);
	ErrorCheck(err, "LJM_NamesToAddresses scan list");
	double scanRate = INITIAL_SCAN_RATE_HZ;
	int handle;
	// Open the first available LabJack device
	printf("Beginning... \n");
	err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
	ErrorCheck(err, "LJM_Open");
	PrintDeviceInfoFromHandle(handle);
	std::vector<StreamOut *> streamOuts;
	printf("\ninitializing stream out buffers... \n");
	for (int i = 0; i < NUM_STREAM_OUTS; i++) {
		StreamOut *streamOut = new StreamOut(streamInfo[i], handle);
		streamOuts.push_back(streamOut);
	}
	printf("\n");
	for (int i = 0; i < NUM_STREAM_OUTS; i++) {
		streamOuts[i]->ReadBufferStatus();
	}
	int scansPerRead = streamOuts[0]->GetStateSize();
	for (int i = 0; i < NUM_STREAM_OUTS; i++) {
		streamOuts[i]->PrintUpdateInfo();

		// Make sure scansPerRead is set to the smallest state size
		if (scansPerRead > streamOuts[i]->GetStateSize()) {
			scansPerRead = streamOuts[i]->GetStateSize();
		}
	}
	for (int i = 0; i < NUM_SCAN_ADDRESSES; i++) {
		printf("stream %d: %s \n", i, scanListString[i]);
	}
	printf("\n");
	printf("scans per read: %d \n\n", scansPerRead);
	err = LJM_eStreamStart(
		handle, 
		scansPerRead, 
		NUM_SCAN_ADDRESSES,
		aScanList, 
		&scanRate
	);
	ErrorCheck(err, "LJM_eStreamStart");
	printf("stream started with scan rate of %f Hz \n\n", scanRate);
	printf("performing %d buffer updates \n", NUM_CYCLES);

	int totalNumSkippedScans = 0;
	for (int i = 0; i < NUM_CYCLES; i++) {
		for (int j = 0; j < NUM_STREAM_OUTS; j++) {
			streamOuts[j]->CheckUpdateBuffer();
		}
		double aData[BUFFER_NUM_BYTES*NUM_SCAN_ADDRESSES/4] = { 0 };
		int deviceScanBacklog;
		int ljmScanBacklog;
		err = LJM_eStreamRead(
			handle, 
			aData, 
			&deviceScanBacklog,
			&ljmScanBacklog
		);
		if (err != 0) PrepareForExit(handle);
		ErrorCheck(err, "LJM_eStreamRead buffer data");
		std::vector<double> data(&aData[0],
			&aData[BUFFER_NUM_BYTES*NUM_SCAN_ADDRESSES/4]);
		totalNumSkippedScans += ProcessStreamResults(
			i, 
			data, 
			deviceScanBacklog,
			ljmScanBacklog, 
			streamOuts[0]->GetStateSize()
		);
	}
	PrepareForExit(handle);
	printf("Total number of skipped scans: %d\n", totalNumSkippedScans);

	return LJME_NOERROR;
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
int ProcessStreamResults(
	int iteration, 
	std::vector<double> data, 
	int deviceNumBacklogScans, 
	int ljmNumBacklogScans, 
	int stateSize
) {
	int numScans = data.size() / NUM_IN_READS;
	printf("\n");
	// Count the skipped samples which are indicated by - 9999 values. Missed
	// samples occur after a device's stream buffer overflows and are
	// reported after auto - recover mode ends.
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
			numSkippedSamples, static_cast<int>(data.size()));
	}
	if (deviceNumBacklogScans > stateSize) {
		printf("Device scan backlog = %d ", deviceNumBacklogScans);
	}
	if (ljmNumBacklogScans > stateSize) {
		printf("LJM scan backlog = %d", ljmNumBacklogScans);
	}
	return numSkippedSamples;
}

// Constructor used to create and initialize out-streams
StreamOut::StreamOut(StreamInfo myStreamOut, int &handle) {
	currentIndex = 0;
	streamOutRegString = "STREAM_OUT";
	StreamOut::handle = handle;
	int outBufferNumValues = myStreamOut.bufferNumBytes / BYTES_PER_VALUE;
	bufferNumBytes = myStreamOut.bufferNumBytes;
	// The size of all the states in streamOut.This must be half of the
	// out buffer or less. (Otherwise, values in a given loop would be getting
	// overwritten during a call to update the buffer.)
	stateSize = outBufferNumValues / 2;
	// Get information from the stream-out object
	target = myStreamOut.target;
	setLoop = myStreamOut.setLoop;
	streamOutIndex = myStreamOut.indexString;
	ConvertNameToOutBufferTypeStr();
	int type;
	int err = LJM_NameToAddress(target.c_str(), &targetAddress, &type);
	ErrorCheck(err, "Problem getting target address");
	LJM_GetHandleInfo(
		handle, 
		&deviceType, 
		&connectionType,
		&serialNum, 
		&ip, 
		&port, 
		&maxBytes
	);
	const int SINGLE_ARRAY_SEND_MAX_BYTES = 524;
	if (maxBytes > SINGLE_ARRAY_SEND_MAX_BYTES) {
		maxBytes = SINGLE_ARRAY_SEND_MAX_BYTES;
	}
	const int NUM_HEADER_BYTES = 12;
	const int NUM_BYTES_PER_F32 = 4;
	maxSamples = ((maxBytes - NUM_HEADER_BYTES) / NUM_BYTES_PER_F32) - 1;
	// Generate the string names of the out-stream registers
	CreateStreamOutNames();
	// Create some states for the out stream to follow
	struct StateInfo myState;
	myState.stateString = "increase from 0.0 to 2.5";
	myState.values = GenerateState(0, 2.5);
	states.push_back(myState);
	myState.stateString = "decrease from 5.0 to 2.5";
	myState.values = GenerateState(5, -2.5);
	states.push_back(myState);
	InitializeStreamOut();
	// Stream is initialized but not running here, only use
	// UpdateStreamOutBuffer for error checking
	UpdateStreamOutBuffer(false);
}

//******************************************************************************
// ReadBufferStatus
// Desc: gets and prints the buffer status register for the out-stream
//******************************************************************************
void StreamOut::ReadBufferStatus() {
	PrintRegisterValue(handle, bufferStatusRegString.c_str());
}

//******************************************************************************
// PrintUpdateInfo
// Desc: prints information about buffer updates
//******************************************************************************
void StreamOut::PrintUpdateInfo() {
	printf(
		"Updating %s buffer whenever %s is greater or equal to %d \n", 
		streamOutRegString.c_str(),
		bufferStatusRegString.c_str(), 
		stateSize
	);
}

//******************************************************************************
// CheckUpdateBuffer
// Desc: checks if the buffer is updating correctly and calls for 
//       new values to be written to the stream-out buffer
//******************************************************************************
void StreamOut::CheckUpdateBuffer() {
	double bufferStatus = 0;
	int infinityPreventer = 0;
	while (bufferStatus < stateSize - 1) {
		LJM_eReadName(handle, bufferStatusRegString.c_str(), &bufferStatus);
		infinityPreventer++;
		if (infinityPreventer > INITIAL_SCAN_RATE_HZ) {
			printf("Buffer status doesn't appear to be updating %s \n",
				bufferRegString.c_str());
			PrepareForExit(handle);
			exit(-1);
		}
	}
	UpdateStreamOutBuffer(true); 
}

//******************************************************************************
// generateState
// Desc: generates state for the out-stream to output. changes
// output linearly from startVal
//
//	double startVal: the output desired for the start of a state
//
//  double changeInVal: the change in output desired for a state
//******************************************************************************
std::vector<double> StreamOut::GenerateState(double startVal, double changeInVal) {
	std::vector<double> values;
	double increment = double(1) / stateSize;
	double sample;
	// Get samples between the start value and desired change in value
	for (int i = 0; i < stateSize; i++) {
		sample = startVal + changeInVal*increment*i;
		values.push_back(sample);
	}
	return values;
}

//******************************************************************************
// InitializeStreamOut
// Desc: Sets the target, buffer size, and enable registers for the out-stream
//******************************************************************************
void StreamOut::InitializeStreamOut() {
	int err = 0;
	err = LJM_eWriteName(handle, targetRegString.c_str(), targetAddress);
	ErrorCheck(err, "LJM_eWriteName initialize out stream target register");
	err = LJM_eWriteName(handle, bufferSizeRegString.c_str(), bufferNumBytes);
	ErrorCheck(err, "LJM_eWriteName initialize out stream buffer size register");
	err = LJM_eWriteName(handle, enableRegString.c_str(), 1);
	ErrorCheck(err, "LJM_eWriteName initialize out stream enable register");
	err = LJM_eWriteName(handle, loopSizeRegString.c_str(), stateSize);
	ErrorCheck(err, "LJM_eWriteName out stream loop size register");
}

//******************************************************************************
// UpdateStreamOutBuffer
// Desc: Write values to the stream - out buffer.Note that once a set of 
// values have been written to the stream out buffer
// (STREAM_OUT0_BUFFER_F32, for example) and STREAM_OUT#_SET_LOOP has 
// been set, that set of values will continue to be output in order and 
// will not be interrupted until their "loop" is complete. Only once that
// set of values have been output in their entirety will the next set of 
// values that have been set using STREAM_OUT#_SET_LOOP start being used.
//******************************************************************************
void StreamOut::UpdateStreamOutBuffer(bool isStreamRunning) {

	int err = 0;
	int errorAddress = INITIAL_ERR_ADDRESS;
	std::vector<double> *values = &states[currentIndex].values;
	int start = 0;
	while (start < static_cast<int>(values->size())) {
		int numSamples = values->size() - start;
		if (numSamples > maxSamples) {
			numSamples = maxSamples;
		}
		int end = start + numSamples;
		std::vector<double> writeValues(
			values->begin() + start,
			values->begin() + end
		);
		if (end >= static_cast<int>(values->size())) {
			int aWrites[2] = { 1, 1 };
			const char *aNames[2] = { 
				bufferRegString.c_str(),
				setLoopRegString.c_str()
			};
			int aNumValues[2] = {numSamples, 1};
			writeValues.push_back(setLoop);
			// Get the pointer to the start of the write value vector
			double *writeValuesStart = &writeValues[0];
			err = LJM_eNames(
				handle, 
				2, 
				aNames, 
				aWrites,
				aNumValues, 
				writeValuesStart, 
				&errorAddress
			);
			ErrorCheck(err, "LJM_eNames update stream out buffer");
		}
		else {
			double *writeValuesStart = &writeValues[0];
			err = LJM_eWriteNameArray(
				handle, 
				bufferRegString.c_str(),
				numSamples, 
				writeValuesStart, 
				&errorAddress
			);
			if (err != 0 && isStreamRunning) PrepareForExit(handle);
			ErrorCheck(err, "LJM_eWriteNameArray update stream out buffer");
		}
		start = start + numSamples;
	}
	printf(
		"	Wrote %s state: %s \n", 
		streamOutRegString.c_str(),
		states[currentIndex].stateString.c_str()
	);
	// Increment the state and wrap it back to zero
	currentIndex = (currentIndex + 1) % states.size();

}

//******************************************************************************
// GetStreamAddress
// Desc: Get address and type information for the stream register
//******************************************************************************
int StreamOut::GetStreamAddress() {
	int address;
	int type;
	int err = 0;
	err = LJM_NameToAddress(streamOutRegString.c_str(), &address, &type);
	ErrorCheck(err, "LJM_NameToAddress get stream address");
	return address;
}

//******************************************************************************
// CreateStreamOutNames
// Desc: generates the appropriate register names for a given out stream
//******************************************************************************
void StreamOut::CreateStreamOutNames() {

	// STREAM_OUT#(0:3) register
	streamOutRegString.append(streamOutIndex);
	// STREAM_OUT#(0:3)_TARGET register
	targetRegString = streamOutRegString;
	targetRegString.append("_TARGET");

	bufferSizeRegString = streamOutRegString;
	bufferSizeRegString.append("_BUFFER_SIZE");
	loopSizeRegString = streamOutRegString;
	loopSizeRegString.append("_LOOP_SIZE");
	setLoopRegString = streamOutRegString;
	setLoopRegString.append("_SET_LOOP");
	bufferStatusRegString = streamOutRegString;
	bufferStatusRegString.append("_BUFFER_STATUS");
	enableRegString = streamOutRegString;
	enableRegString.append("_ENABLE");
	bufferRegString = streamOutRegString;
	bufferRegString.append("_BUFFER_");
	bufferRegString.append(targetTypeString.c_str());
}

int StreamOut::GetStateSize() {
	return stateSize;
}

//******************************************************************************
// ConvertNameToOutBufferTypeStr
// Desc: Determines the proper buffer type for the out-stream target
//******************************************************************************
void StreamOut::ConvertNameToOutBufferTypeStr() {
	int type;
	int address;
	int err = 0;
	err = LJM_NameToAddress(target.c_str(), &address, &type);
	ErrorCheck(err, "LJM_NameToAddress convert name to out buffer type string");
	switch (type) {
	case LJM_UINT16:
		targetTypeString = "U16";
		break;
	case LJM_UINT32:
		targetTypeString = "U32";
		break;
	// Note: there is no STREAM_OUT#(0:3)_BUFFER_I32 
	case LJM_FLOAT32:
		targetTypeString = "F32";
		break;
	default:
		targetTypeString = "U32";
		break;
	}
}
