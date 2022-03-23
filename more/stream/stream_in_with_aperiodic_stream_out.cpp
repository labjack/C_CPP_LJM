/**
 * Name: stream_in_with_aperiodic_stream_out.cpp
 * Desc: Demonstrates usage of aperiodic stream-out functions with stream-in
 *
 * Streams in while streaming out arbitrary values. These arbitrary stream-out
 * values act on DAC0 to cyclically increase the voltage from 0 to 2.5.
 * Though these values are generated before the stream starts, the values could
 * be dynamically generated, read from a file, etc.
 *
 * Relevant Documentation:
 *
 * LJM Library:
 *  LJM Library Installer:
 *      https://labjack.com/support/software/installers/ljm
 *  LJM Users Guide:
 *      https://labjack.com/support/software/api/ljm
 *  Opening and Closing:
 *      https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *  LJM Single Value Functions (like eReadName, eReadAddress):
 *      https://labjack.com/support/software/api/ljm/function-reference/single-value-functions
 *  Stream Functions (eStreamRead, eStreamStart, etc.):
 *      https://labjack.com/support/software/api/ljm/function-reference/stream-functions
 *
 * T-Series and I/O:
 *  Modbus Map:
 *      https://labjack.com/support/software/api/modbus/modbus-map
 *  Stream Mode:
 *      https://labjack.com/support/datasheets/t-series/communication/stream-mode
 *  Analog Inputs:
 *      https://labjack.com/support/datasheets/t-series/ain
 *  Stream-Out:
 *      https://labjack.com/support/datasheets/t-series/communication/stream-mode/stream-out/stream-out-description
 *  Digital I/O:
 *      https://labjack.com/support/datasheets/t-series/digital-io
 *  DAC:
 *      https://labjack.com/support/datasheets/t-series/dac
**/

#include "../../LJM_StreamUtilities.h"
#include <LabJackM.h>

int main() {
    int handle;
    int err = 0;
    // Desired scan rate for the streams
    double scanRate = 1000;
    // Desired number of write cycles (periods of waveform in the loop) 
    const int NUM_WRITES = 8;
    // The total number of in and out streams 
    enum { NUM_SCAN_ADDRESSES = 2 };
    // Note that in-streams compete for resources with out-streams, so
    // performance of out-streams with in-streams is notably worse than just
    // streaming out-streams if dynamically loading data to stream out.
    const char* scanList[NUM_SCAN_ADDRESSES] = { "AIN0", "STREAM_OUT0" };
    int targetAddr = 1000; // DAC0
    // With current T-series devices, 4 stream-outs can be ran concurrently
    // stream-out index should therefore be a value 0-3.
    int streamOutIndex = 0;
    int samplesToWrite = 512;
    double* values = new double[samplesToWrite];
    double increment = double(1) / samplesToWrite;
    // Make an arbitrary waveform that increases voltage linearly from 0-2.5V
    for (int i = 0; i < samplesToWrite; i++) {
        double sample = 2.5 * increment * i;
        values[i] = sample;
    }

    printf("Beginning... \n");
    // Open first available LabJack device
    err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
    ErrorCheck(err, "LJM_Open");
    PrintDeviceInfoFromHandle(handle);
    ErrorCheck(err, "PrintDeviceInfoFromHandle");
    printf("\nInitializing stream out buffer... \n");
    err = LJM_InitializeAperiodicStreamOut(
        handle,
        streamOutIndex,
        targetAddr,
        scanRate
    );
    ErrorCheck(err, "LJM_InitializeAperiodicStreamOut");
    printf("\n");
    int queueVals;
    // Write some data to the buffer before the stream starts
    err = LJM_WriteAperiodicStreamOut(
        handle,
        streamOutIndex,
        samplesToWrite,
        values,
        &queueVals
    );
    ErrorCheck(err, "LJM_WriteAperiodicStreamOut");
    err = LJM_WriteAperiodicStreamOut(
        handle,
        streamOutIndex,
        samplesToWrite,
        values,
        &queueVals
    );
    ErrorCheck(err, "LJM_WriteAperiodicStreamOut");
    int scansPerRead = scanRate / 2;
    for (int i = 0; i < NUM_SCAN_ADDRESSES; i++) {
        printf("stream %d: %s \n", i, scanList[i]);
    }
    printf("\n");
    int aScanList[NUM_SCAN_ADDRESSES];
    int aTypes[NUM_SCAN_ADDRESSES];
    int deviceScanBacklog;
    int ljmScanBacklog;
    err = LJM_NamesToAddresses(
        NUM_SCAN_ADDRESSES,
        scanList,
        aScanList,
        aTypes
    );
    ErrorCheck(err, "LJM_NamesToAddresses scan list");
    int startTime = GetCurrentTimeMS();
    err = LJM_eStreamStart(
        handle,
        scansPerRead,
        NUM_SCAN_ADDRESSES,
        aScanList,
        &scanRate
    );
    ErrorCheck(err, "LJM_eStreamStart");
    printf("stream started with scan rate of %f Hz \n\n", scanRate);
    printf("performing %d buffer updates \n", NUM_WRITES);
    double* aData = new double[scansPerRead * NUM_SCAN_ADDRESSES];
    for (int i = 0; i < NUM_WRITES; i++) {
        err = LJM_WriteAperiodicStreamOut(
            handle,
            streamOutIndex,
            samplesToWrite,
            values,
            &queueVals
        );
        ErrorCheck(err, "LJM_WriteAperiodicStreamOut in loop");
        err = LJM_eStreamRead(
            handle,
            aData,
            &deviceScanBacklog,
            &ljmScanBacklog
        );
        ErrorCheck(err, "LJM_eStreamRead buffer data");
        printf(
            "iteration: %d - deviceScanBacklog: %d, LJMScanBacklog: %d\n",
            i,
            deviceScanBacklog,
            ljmScanBacklog
        );
    }
    // Since scan rate determines how quickly data can be written from the device
    // large chunks of data written at low scan rates can take longer to write
    // out than it takes to call LJM_WriteAperiodicStreamOut and 
    // LJM_eStreamRead. some delay may be necessary if it is desired to write out
    // all data then immediately close the stream
    int runTime = GetCurrentTimeMS() - startTime;
    // 512 samples * 10 writes = 5120 samples. scan rate = 1000
    // samples/sec, so it should take 5.12 seconds to write all data out
    int streamOutMS = 1000 * samplesToWrite * (NUM_WRITES + 2) / scanRate;
    if (runTime < streamOutMS) {
        MillisecondSleep(streamOutMS - runTime);
    }
    printf("Stopping stream...\n");
    err = LJM_eStreamStop(handle);
    ErrorCheck(err, "Problem closing stream");
    err = LJM_Close(handle);
    ErrorCheck(err, "Problem closing device");
    delete[] aData;
    delete[] values;
    return LJME_NOERROR;
}
