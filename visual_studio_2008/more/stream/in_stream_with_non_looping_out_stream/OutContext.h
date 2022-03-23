/**
 * Name: OutContext.h
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
 * Relevant documentation:
 *
 * LJM library:
 *  LJM Users Guide: https://labjack.com/support/software/api/ljm
 *	LJM_Open: https://labjack.com/support/software/api/ljm/function-reference/ljmopen
 *	LJM single value functions (like eReadName, eReadAddress): 
 *	https://labjack.com/support/software/api/ljm/function-reference/single-value-functions
 *	Stream functions (eStreamRead, eStreamStart, etc.):
 *	https://labjack.com/support/software/api/ljm/function-reference/stream-functions
 *	error codes: https://labjack.com/support/software/api/ljm/error-codes
 *
 * T-series and I/O:
 *	Modbus register map: https://labjack.com/support/software/api/modbus/modbus-map
 *	Stream-mode:
 *	https://labjack.com/support/datasheets/t-series/communication/stream-mode
 *	Stream-out:
 *	https://labjack.com/support/datasheets/t-series/communication/stream-mode/stream-out/stream-out-description
 *	DAC: https://labjack.com/support/datasheets/t-series/dac
 *	Analog I/O: https://labjack.com/support/datasheets/t-series/ain
**/

#ifndef OUT_CONTEXT_H
#define OUT_CONTEXT_H


#include "in_stream_with_non_looping_out_stream_supplement.h"
#include <cmath>

class OutContext {

public:
	static const int BYTES_PER_VALUE = 2;

	// device information; see LJM_GetHandleInfo()
	int deviceType; 
	int connectionType; 
	int serialNum;
	int ip;
	int port;
	int maxBytes;
	int handle;

	int targetAddress;

	OutContext(stream_outs myStreamOut, int &handle);
	void ReadBufferStatus();
	void PrintUpdateInfo();
	void CheckUpdateBuffer();
	int GetStreamAddress();
	int GetStateSize();

private:
	struct StateInfo {
		string stateString;
		vector<double> values;
	};
	int currentIndex;
	vector<StateInfo> states;
	int stateSize;
	string targetTypeString;
	string target;
	int bufferNumBytes;
	int setLoop;
	string streamOutIndex;
	string streamOutRegString;
	string targetRegString;
	string bufferSizeRegString;
	string loopSizeRegString;
	string setLoopRegString;
	string bufferStatusRegString;
	string enableRegString;
	string bufferRegString;
	vector<double> GenerateState(double startVal, double changeInVal);
	int maxSamples;
	void InitializeStreamOut();
	void CreateStreamOutNames();
	void ConvertNameToOutBufferTypeStr();
	void UpdateStreamOutBuffer(bool isStreamRunning);
};

// constructor used to create and initialize out-streams
OutContext::OutContext(stream_outs myStreamOut, int &handle) {
	currentIndex = 0;
	streamOutRegString = "STREAM_OUT";
	OutContext::handle = handle;
	int outBufferNumValues = myStreamOut.bufferNumBytes / BYTES_PER_VALUE;
	bufferNumBytes = myStreamOut.bufferNumBytes;

	// The size of all the states in out_context.This must be half of the
	// out buffer or less. (Otherwise, values in a given loop would be getting
	// overwritten during a call to update_stream_out_buffer.)
	stateSize = outBufferNumValues / 2;

	// get information from the streamout objects created in 
	// in_stream_with_non_looping_out_stream.cpp
	target = myStreamOut.target;
	setLoop = myStreamOut.setLoop;
	streamOutIndex = myStreamOut.indexString;
	ConvertNameToOutBufferTypeStr();
	int type;
	// get target address
	int err = LJM_NameToAddress(target.c_str(), &targetAddress, &type);
	ErrorCheck(err, "Problem getting target address");
	

	// get device information
	LJM_GetHandleInfo(handle, &deviceType, &connectionType,
		&serialNum, &ip, &port, &maxBytes);
	const int SINGLE_ARRAY_SEND_MAX_BYTES = 524;
	if (maxBytes > SINGLE_ARRAY_SEND_MAX_BYTES) {
		maxBytes = SINGLE_ARRAY_SEND_MAX_BYTES;
	}
	const int NUM_HEADER_BYTES = 12;
	const int NUM_BYTES_PER_F32 = 4;
	// convert max_bytes to samples
	maxSamples = ((maxBytes - NUM_HEADER_BYTES) / NUM_BYTES_PER_F32) - 1;

	// generate the string names of the out-stream registers
	CreateStreamOutNames();

	// create some states for the out stream to follow
	struct StateInfo myState;
	myState.stateString = "increase from 0.0 to 2.5";
	myState.values = GenerateState(0, 2.5);
	states.push_back(myState);
	myState.stateString = "decrease from 5.0 to 2.5";
	myState.values = GenerateState(5, -2.5);
	states.push_back(myState);

	// initialize the out-stream
	InitializeStreamOut();
	UpdateStreamOutBuffer(false);
	// stream is initialized but not running here, tell
	// UpdateStreamOutBuffer for error checking
}

//******************************************************************************
// ReadBufferStatus
//
// Desc: gets and prints the buffer status register for the out-stream
//******************************************************************************
void OutContext::ReadBufferStatus() {
	PrintRegisterValue(handle, bufferStatusRegString.c_str());
}

//******************************************************************************
// PrintUpdateInfo
//
//	Desc: prints information about buffer updates
//******************************************************************************
void OutContext::PrintUpdateInfo() {
	printf("Updating %s buffer whenever %s is greater or equal to %d \n", 
		streamOutRegString.c_str(), bufferStatusRegString.c_str(), stateSize);
}

//******************************************************************************
// CheckUpdateBuffer
//
// Desc: checks if the buffer is updating correctly and calls for 
// new values to be written to the stream-out buffer
//******************************************************************************
void OutContext::CheckUpdateBuffer() {
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
	// stream is already running here, tell
	// UpdateStreamOutBuffer for error checking
}

//******************************************************************************
// generateState
//
// Desc: generates state for the out-stream to output. changes
// output linearly from startVal
//
//	double startVal: the output desired for the start of a state
//
//  double changeInVal: the change in output desired for a state
//******************************************************************************
vector<double> OutContext::GenerateState(double startVal, double changeInVal) {
	vector<double> values;
	double increment = double(1) / stateSize;
	double sample;

	// get samples between the start value and desired change in value
	for (int i = 0; i < stateSize; i++) {
		sample = startVal + changeInVal*increment*i;
		values.push_back(sample);
	}
	return values;
}

//******************************************************************************
// InitializeStreamOut
//
// sets the target, buffer size, and enable registers for the out-stream
//******************************************************************************
void OutContext::InitializeStreamOut() {
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
//
// Desc: Write values to the stream - out buffer.Note that once a set of 
// values have been written to the stream out buffer
// (STREAM_OUT0_BUFFER_F32, for example) and STREAM_OUT#_SET_LOOP has 
// been set, that set of values will continue to be output in order and 
// will not be interrupted until their "loop" is complete. Only once that
// set of values have been output in their entirety will the next set of 
// values that have been set using STREAM_OUT#_SET_LOOP start being used.
//******************************************************************************
void OutContext::UpdateStreamOutBuffer(bool isStreamRunning) {

	// write the loop size to the device
	int err = 0;
	int errorAddress = INITIAL_ERR_ADDRESS;
	vector<double> *values = &states[currentIndex].values;

	int start = 0;
	while (start < values->size()) {
		int numSamples = values->size() - start;
		if (numSamples > maxSamples) {
			numSamples = maxSamples;
		}
		int end = start + numSamples;

		// create a subvector to write to the labjack device
		vector<double> writeValues(values->begin() + start,
			values->begin() + end);

		if (end >= values->size()) {
			int aWrites[2] = { 1, 1 };
			const char *aNames[2] = { bufferRegString.c_str(),
				setLoopRegString.c_str() };
			int aNumValues[2] = {numSamples, 1};
			writeValues.push_back(setLoop);
			// get the pointer to the start of the write value vector
			double *writeValuesStart = &writeValues[0];
			err = LJM_eNames(handle, 2, aNames, aWrites,
				aNumValues, writeValuesStart, &errorAddress);
			ErrorCheck(err, "LJM_eNames update stream out buffer");
		}
		else {
			// get the pointer to the start of the write value vector
			double *writeValuesStart = &writeValues[0];
			err = LJM_eWriteNameArray(handle, bufferRegString.c_str(),
			numSamples, writeValuesStart, &errorAddress);
			if (err != 0 && isStreamRunning) PrepareForExit(handle);
			ErrorCheck(err, "LJM_eWriteNameArray update stream out buffer");
		}
		
		start = start + numSamples;
	}

	printf("	Wrote %s state: %s \n", streamOutRegString.c_str(),
		states[currentIndex].stateString.c_str());
	// Increment the state and wrap it back to zero
	currentIndex = (currentIndex + 1) % states.size();

}

int OutContext::GetStreamAddress() {
	// get address and type information for the stream register
	int address;
	int type;
	int err = 0;
	err = LJM_NameToAddress(streamOutRegString.c_str(), &address, &type);
	ErrorCheck(err, "LJM_NameToAddress get stream address");
	return address;
}

//******************************************************************************
// CreateStreamOutNames
//
// Desc: generates the appropriate register names for a given out stream
//******************************************************************************
void OutContext::CreateStreamOutNames() {

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

int OutContext::GetStateSize() {
	return stateSize;
}

//******************************************************************************
// ConvertNameToOutBufferTypeStr
//
// Desc: determines the proper buffer type for the out-stream target
//******************************************************************************
void OutContext::ConvertNameToOutBufferTypeStr() {
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
#endif