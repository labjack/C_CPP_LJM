/**
 * Name: stream_log_testing.c
 * Desc: Example that demonstrates logging to file while streaming. Will create
 *       a new directory every hour and new file every minute. Logs data as CSV.
 *       Directory naming is in the format yyyy_mm_dd_hh.
 *       Each file name indicates the minute in the hour given by the directory.
 *       Each CSV row will contain a timestamp relative to the start of the
 *       minute in the format ssuuuuuu (s=seconds, u=microseconds).
 *       A system timestamp is synchronized to the stream time every
 *       eStreamRead call (once every second with the default settings).
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
 *  Stream Mode:
 *      https://labjack.com/support/datasheets/t-series/communication/stream-mode
 *
 * T-Series and I/O:
 *  Modbus Map:
 *      https://labjack.com/support/software/api/modbus/modbus-map
**/

#include <stdio.h>
#include <string.h>

#include <signal.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/resource.h>
#endif
#include <sys/stat.h>

#include <LabJackM.h>

#include "../../LJM_StreamUtilities.h"

#ifndef FILENAME_MAX
#define FILENAME_MAX 4096
#endif

//----------------------------------------------
// Start Constants and Function Stubs
//----------------------------------------------
enum { MAX_DIR_NAME_LENGTH = 80 };

const char * __restrict USAGE = "Usage:\t%s SECONDS_DURATION LOGPATH(optional)\n";

// For SIGINT handler
static volatile void (*ljm_handler)(int) = NULL;
static int handleToCleanUp = 0;

// The core timer runs at 40 MHz
const int CORE_TIMER_FREQUENCY = 40000000;

// How fast to stream in Hz
enum { INIT_SCAN_RATE = 16000 };

// How many scans to get per call to LJM_eStreamRead. INIT_SCAN_RATE/2 is
// often recommended to start. We are going to read every second.
enum { SCANS_READ_SCALAR = 1 };
const int SCANS_PER_READ = INIT_SCAN_RATE / SCANS_READ_SCALAR;

// Channels/Addresses to stream. NUM_CHANNELS can be less than or equal to
// the size of CHANNEL_NAMES
enum { NUM_CHANNELS = 6 };
const char * CHANNEL_NAMES[] = {"AIN0","AIN1","AIN2","AIN3","AIN4","AIN5"};

const char * FILENAME_EXT = ".csv";

typedef struct {
    char dirPath[FILENAME_MAX];
    char dirName[MAX_DIR_NAME_LENGTH];
    struct tm * timestamp;
    FILE * logFile;
    int handle;
    double scanRate;
    int lastMinute;
    int lastHour;
}LogInfo;

void StreamAndLog(LogInfo * logInfo, long int numScans);

void HardcodedConfigureStream(const int handle);

void KeyboardInterruptHandler(int sig);

void DataToFile(LogInfo * logInfo, const double * aData, const int numScansToProcess);

void SetupNewFile(LogInfo * logInfo);

void SetupNewDir(LogInfo * logInfo);

void FailAndMessage(const char * __restrict formattedDescription, ...);

void UpdateTime(LogInfo * logInfo);

int MakeDirectory(const char * dirname);

void SetupInterruptHandler(void);
//----------------------------------------------
// End Constants and Function Stubs
//----------------------------------------------

int main(int argc, char * argv[])
{
    long int numScans;
    int error;
    long int duration;
    LogInfo logInfo;
    // Initialization for some logInfo members we do not touch in main
    logInfo.logFile = NULL;
    logInfo.timestamp = NULL;
    logInfo.lastHour = -5; // An impossible value to ensure we create a new directory at the start of the program.
    logInfo.lastMinute = -5; // An impossible value to ensure we create a new file at the start of the program.

    // Check if we got reasonable input arguments
    if (argc < 2) {
        FailAndMessage(USAGE, argv[0]);
    }
    else if (argc == 3) { // If the optional LOGPATH is passed in
        if (strlen(argv[2]) >= FILENAME_MAX) {
            printf("Bad path passed in");
            FailAndMessage(USAGE, argv[0]);
        }
        strcpy(logInfo.dirPath, argv[2]);
        error = MakeDirectory(logInfo.dirPath);
        // mkdir will fail if the directory already exists
        // We do not care about a failure in that case
        // Error out otherwise
        if(error != 0 && errno != EEXIST) {
            FailAndMessage("\nError making directory %s. %s\n", logInfo.dirPath, strerror(errno));
        }
    }
    else {
        strcpy(logInfo.dirPath, ".");
    }

    duration = strtol(argv[1], NULL, 10);
    if (duration <= 0) {
        printf("Test duration must be greater than 0 seconds\n");
        FailAndMessage(USAGE, argv[0]);
    }
    SetupInterruptHandler();

    numScans = duration*SCANS_READ_SCALAR;
    logInfo.scanRate = INIT_SCAN_RATE;
    // Open first found LabJack with a USB connection
    logInfo.handle = OpenOrDie(LJM_dtANY, LJM_ctUSB, "LJM_idANY");
    handleToCleanUp = logInfo.handle;
    PrintDeviceInfoFromHandle(logInfo.handle);
    printf("\n");

    StreamAndLog(&logInfo, numScans);

    CloseOrDie(logInfo.handle);

    return LJME_NOERROR;
}

void HardcodedConfigureStream(const int handle)
{
    const int STREAM_TRIGGER_INDEX = 0;
    const int STREAM_CLOCK_SOURCE = 0;
    const int STREAM_RESOLUTION_INDEX = 0;
    const double STREAM_SETTLING_US = 0;
    const double AIN_ALL_RANGE = 0;
    const int AIN_ALL_NEGATIVE_CH = LJM_GND;

    printf("Writing configurations:\n");

    if (STREAM_TRIGGER_INDEX == 0) {
        printf("    Ensuring triggered stream is disabled:");
    }
    printf("    Setting STREAM_TRIGGER_INDEX to %d\n", STREAM_TRIGGER_INDEX);
    WriteNameOrDie(handle, "STREAM_TRIGGER_INDEX", STREAM_TRIGGER_INDEX);

    if (STREAM_CLOCK_SOURCE == 0) {
        printf("    Enabling internally-clocked stream:");
    }
    printf("    Setting STREAM_CLOCK_SOURCE to %d\n", STREAM_CLOCK_SOURCE);
    WriteNameOrDie(handle, "STREAM_CLOCK_SOURCE", STREAM_CLOCK_SOURCE);

    // Configure the analog inputs' negative channel, range, settling time and
    // resolution.
    // Note: when streaming, negative channels and ranges can be configured for
    // individual analog inputs, but the stream has only one settling time and
    // resolution.
    printf("    Setting STREAM_RESOLUTION_INDEX to %d\n",
        STREAM_RESOLUTION_INDEX);
    WriteNameOrDie(handle, "STREAM_RESOLUTION_INDEX", STREAM_RESOLUTION_INDEX);

    printf("    Setting STREAM_SETTLING_US to %f\n", STREAM_SETTLING_US);
    WriteNameOrDie(handle, "STREAM_SETTLING_US", STREAM_SETTLING_US);

    printf("    Setting AIN_ALL_RANGE to %f\n", AIN_ALL_RANGE);
    WriteNameOrDie(handle, "AIN_ALL_RANGE", AIN_ALL_RANGE);

    printf("    Setting AIN_ALL_NEGATIVE_CH to ");
    if (AIN_ALL_NEGATIVE_CH == LJM_GND) {
        printf("LJM_GND");
    }
    else {
        printf("%d", AIN_ALL_NEGATIVE_CH);
    }
    printf("\n");
    WriteNameOrDie(handle, "AIN_ALL_NEGATIVE_CH", AIN_ALL_NEGATIVE_CH);
    printf("    Setting STREAM_BUFFER_SIZE_BYTES\n");
    // Max buffer size is 32768 bytes
    WriteNameOrDie(handle, "STREAM_BUFFER_SIZE_BYTES", 32768);
}

void StreamAndLog(LogInfo * logInfo, long int numScans)
{
    int err;
    int iteration = 0;
    int numSkippedScans = 0;
    int deviceScanBacklog = 0;
    int LJMScanBacklog = 0;
    int totalSkippedScans = 0;

    int * aScanList = malloc(sizeof(int) * NUM_CHANNELS);
    unsigned int aDataSize = NUM_CHANNELS * SCANS_PER_READ;
    double * aData = malloc(sizeof(double) * aDataSize);
    // Clear aData. This is not strictly necessary, but can help debugging.
    if (aData != NULL && aScanList != NULL) {
        memset(aData, 0, sizeof(double) * aDataSize);
    }
    else {
        FailAndMessage("Failure allocating memory, exiting.");
    }

    err = LJM_NamesToAddresses(NUM_CHANNELS, CHANNEL_NAMES, aScanList, NULL);
    ErrorCheck(err, "Getting positive channel addresses");

    HardcodedConfigureStream(logInfo->handle);

    printf("\n");
    printf("Starting stream. Will run for %f seconds\n", (double)SCANS_PER_READ*numScans/logInfo->scanRate);
    err = LJM_eStreamStart(logInfo->handle, SCANS_PER_READ, NUM_CHANNELS, aScanList,
        &logInfo->scanRate);
    ErrorCheck(err, "LJM_eStreamStart");

    printf("Stream started. Actual scan rate: %.02f Hz (%.02f sample rate)\n",
        logInfo->scanRate, logInfo->scanRate * NUM_CHANNELS);
    printf("\n");

    // Read the scans. Run a fast loop to prevent autorecovery/dummy samples
    while(iteration < numScans) {
        err = LJM_eStreamRead(logInfo->handle, aData, &deviceScanBacklog,
            &LJMScanBacklog);
        ErrorCheck(err, "LJM_eStreamRead");
        // Print every 4 eStreamRead calls.
        if (!(iteration % 4)) {
            printf("iteration: %d - deviceScanBacklog: %d, LJMScanBacklog: %d",
            iteration, deviceScanBacklog, LJMScanBacklog);
            printf("\n");

            numSkippedScans = CountAndOutputNumSkippedScans(NUM_CHANNELS,
                SCANS_PER_READ, aData);

            if (numSkippedScans) {
                printf("  %d skipped scans in this LJM_eStreamRead\n",
                    numSkippedScans);
                totalSkippedScans += numSkippedScans;
            }
            printf("\n****** Total number of skipped scans: %d ******\n\n",
            totalSkippedScans);
        }
        DataToFile(logInfo, aData, SCANS_PER_READ);
        iteration++;
    }

    free(aData);
    free(aScanList);
    fclose(logInfo->logFile);

    printf("Stopping stream\n");
    LJM_eStreamStop(logInfo->handle);
    ErrorCheck(err, "Stopping stream");
}

void KeyboardInterruptHandler(int sig)
{
    LJM_eWriteName(handleToCleanUp, "STREAM_ENABLE", 0);

    if (ljm_handler != NULL) {
        ljm_handler(sig);
    }
    exit(-1);
}

void DataToFile(LogInfo * logInfo, const double * aData, const int numScansToProcess)
{
    long int microSeconds = 0;
    const int MICROSECONDS_PER_SECOND = 1000000;
    UpdateTime(logInfo);
    for (int i=0; i<numScansToProcess; i++) {
        fprintf(logInfo->logFile, "%02d%06ld", logInfo->timestamp->tm_sec, microSeconds);
        for(int j=0; j<NUM_CHANNELS;j++) {
            fprintf(logInfo->logFile, ",%.4f", aData[i+j]);
        }
        fprintf(logInfo->logFile, "\n");
        microSeconds+=(long)(MICROSECONDS_PER_SECOND/logInfo->scanRate);
    }
}

void SetupNewFile(LogInfo * logInfo)
{
    int ret;
    if (logInfo->logFile != NULL) {
        fclose(logInfo->logFile);
    }
    char filePath[FILENAME_MAX] = {0};
    ret = snprintf(filePath, FILENAME_MAX, "%s/%s/%02d%s", logInfo->dirPath, logInfo->dirName, logInfo->timestamp->tm_min, FILENAME_EXT);
    if(ret < 0) {
        LJM_eWriteName(logInfo->handle, "STREAM_ENABLE", 0);
        CloseOrDie(logInfo->handle);
        FailAndMessage("Error, bad file name/path.\n");
    }
    // Create or overwrite a file. Will be placed in the current working directory.
    logInfo->logFile = fopen(filePath, "a+");
    if (logInfo->logFile == NULL) {
        LJM_eWriteName(logInfo->handle, "STREAM_ENABLE", 0);
        CloseOrDie(logInfo->handle);
        FailAndMessage("Error, cannot open file.\n");
    }
}

void SetupNewDir(LogInfo * logInfo)
{
    int err, ret;
    char tempPath[FILENAME_MAX] = {0};
    strftime(logInfo->dirName, MAX_DIR_NAME_LENGTH, "%Y_%m_%d_%H", logInfo->timestamp);
    ret = snprintf(tempPath, FILENAME_MAX, "%s/%s", logInfo->dirPath, logInfo->dirName);
    if(ret < 0) {
        LJM_eWriteName(logInfo->handle, "STREAM_ENABLE", 0);
        CloseOrDie(logInfo->handle);
        FailAndMessage("Error, bad directory name/path.\n");
    }
    err = MakeDirectory(tempPath);
    // mkdir will fail if the directory already exists
    // We do not care about a failure in that case
    if (err != 0 && errno != EEXIST) {
        LJM_eWriteName(logInfo->handle, "STREAM_ENABLE", 0);
        CloseOrDie(logInfo->handle);
        FailAndMessage("\nError making directory %s. %s\n", tempPath, strerror(errno));
    }
}

void UpdateTime(LogInfo * logInfo)
{
    time_t t1 = time(NULL);
    // Get a new system timestamp
    logInfo->timestamp = localtime(&t1);
    if(logInfo->lastHour != logInfo->timestamp->tm_hour) {
        SetupNewDir(logInfo);
        logInfo->lastHour = logInfo->timestamp->tm_hour;
    }
    if(logInfo->lastMinute != logInfo->timestamp->tm_min) {
        SetupNewFile(logInfo);
        logInfo->lastMinute = logInfo->timestamp->tm_min;
    }
}

void FailAndMessage(const char * __restrict formattedDescription, ...)
{
    va_list args;
    va_start(args, formattedDescription);
    vfprintf(stdout, formattedDescription, args);
    va_end(args);
    exit(-1);
}

int MakeDirectory(const char* dirname)
{
    int err=0;
#ifdef _WIN32
    err = _mkdir(dirname);
#else
    // Use default directory permissions upon creation
    const mode_t DIRMODE = 0777;
    err = mkdir(dirname, DIRMODE);
#endif // _WIN32
    return err;
}

void SetupInterruptHandler(void)
{
    // Initialize the LJM signal handler by "attempting" device communication
    // using LJM_CloseAll - we can ignore the return value. Other functions
    // such as LJM_Open or LJM_ListAll work as well. We want to handle a
    // keyboard interrupt to try to stop stream.
#ifdef _WIN32
    LJM_CloseAll();
    ljm_handler = signal(SIGINT, KeyboardInterruptHandler);
#else
    struct sigaction act, oldact;
    LJM_CloseAll();
    act.sa_handler = KeyboardInterruptHandler;
    act.sa_flags = 0;
    error = sigaction(SIGINT, &act, &oldact);
    if (error) {
        FailAndMessage("There was an error during sigaction (%s)\n", strerror(errno));
}
    ljm_handler = (volatile void (*)(int))oldact.sa_handler;
    // Set the process priority if root.
    setpriority(PRIO_PROCESS, 0, -10);
#endif
}
