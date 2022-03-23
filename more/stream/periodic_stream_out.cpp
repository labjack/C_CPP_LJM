/**
 * Name: periodic_stream_out.cpp
 * Desc: Demonstrates usage of the periodic stream-out functions
 *
 * Streams out arbitrary values. These arbitrary stream-out
 * values act on DAC0 to cyclically increase the voltage from 0 to 2.5.
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
 *  Stream-Out:
 *      https://labjack.com/support/datasheets/t-series/communication/stream-mode/stream-out
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
    // Desired duration to run the stream out
    int runTimeMS = 5000;
    // The total number of in and out streams
    enum { NUM_SCAN_ADDRESSES = 1 }; 
    const char * scanList[NUM_SCAN_ADDRESSES] = {"STREAM_OUT0"};
    int targetAddr = 1000; // DAC0
    // With current T-series devices, 4 stream-outs can be ran concurrently.
    // Stream-out index should therefore be a value 0-3.
    int streamOutIndex = 0;
    int samplesToWrite = 512;
    // Make an arbitrary waveform that increases voltage linearly from 0-2.5V
    double * values = new double[samplesToWrite];
    double increment = double(1) / samplesToWrite;
    for (int i = 0; i < samplesToWrite; i++) {
        double sample = 2.5*increment*i;
        values[i] = sample;
    }

    printf("Beginning... \n");
    // Open first available LabJack device
    err = LJM_Open(LJM_dtANY, LJM_ctANY, "LJM_idANY", &handle);
    ErrorCheck(err, "LJM_Open");
    PrintDeviceInfoFromHandle(handle);
    ErrorCheck(err, "PrintDeviceInfoFromHandle");
    printf("\nInitializing stream out... \n");
    err = LJM_PeriodicStreamOut(
        handle, 
        streamOutIndex,
        targetAddr,  
        scanRate,
        samplesToWrite,
        values
    );
    ErrorCheck(err, "LJM_PeriodicStreamOut");
    printf("\n");
    int scansPerRead = scanRate/ 2;
    for (int i = 0; i < NUM_SCAN_ADDRESSES; i++) {
        printf("stream %d: %s \n", i, scanList[i]);
    }
    printf("\n");
    int aScanList[NUM_SCAN_ADDRESSES];
    int aTypes[NUM_SCAN_ADDRESSES];
    err = LJM_NamesToAddresses(
        NUM_SCAN_ADDRESSES, 
        scanList,
        aScanList, 
        aTypes
    );
    ErrorCheck(err, "LJM_NamesToAddresses scan list");
    err = LJM_eStreamStart(
        handle, 
        scansPerRead, 
        NUM_SCAN_ADDRESSES,
        aScanList, 
        &scanRate
    );
    ErrorCheck(err, "LJM_eStreamStart");
    printf("Stream started with scan rate of %f Hz \n\n", scanRate);
    // Run for some time then stop the stream
    MillisecondSleep(runTimeMS);
    printf("Stopping stream...\n");
    err = LJM_eStreamStop(handle);
    ErrorCheck(err, "Problem closing stream");
    err = LJM_Close(handle);
    ErrorCheck(err, "Problem closing device");
    delete[] values;
    return LJME_NOERROR;
} 
