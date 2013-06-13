#include <windows.h>
#include <stdio.h>
#include "string.h"

int __cdecl main(int argc,char **argv) {
	HANDLE hDevice;
	BOOL status;
	char outBuffer[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\mykey";
	DWORD outCount, bW;

	printf("Beginning test of Loopback Driver (CH7)...\n");

	hDevice =
		CreateFile("\\\\.\\RRK",
					GENERIC_READ | GENERIC_WRITE,
					0,		// share mode none
					NULL,	// no security
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL );		// no template
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"LBK1", GetLastError() );
		return 1;
	}

	printf("Succeeded in obtaining handle to RRK device.\n");
	printf("Attempting write to device...\n");
	Sleep(3000);
	
	outCount = strlen(outBuffer);
	
	status =WriteFile(hDevice, outBuffer, strlen(outBuffer), &bW, NULL);
	//printf("Write succesful...\n");
	
	
	if (!status) {
		printf("Failed on call to WriteFile - error: %d\n",
			GetLastError() );
		return 2;
	}
	if (outCount == bW) {
		printf("Succeeded in writing %d bytes\n", outCount);
		printf("Buffer was: %s\n", outBuffer);
	} else {
		printf("Failed to write the correct number of bytes.\n"
			"Attempted to write %d bytes, but WriteFile reported %d bytes.\n",
			outCount, bW);
		return 3;
	}
/*
	printf("Attempting to read from device...\n");
	char inBuffer[80];
	DWORD inCount = sizeof(inBuffer);
	DWORD bR;
	status =
		ReadFile(hDevice, inBuffer, inCount, &bR, NULL);
	if (!status) {
		printf("Failed on call to ReadFile - error: %d\n",
			GetLastError() );
		return 4;
	}
	if (bR == bW) {
		printf("Succeeded in reading %d bytes\n", bR);
		printf("Buffer was: %s\n", inBuffer);
	} else {
		printf("Failed to read the correct number of bytes.\n"
			"Should have read %d bytes, but ReadFile reported %d bytes.\n",
			bW, inCount);
		return 5;
	}

	printf("Attempting to close device LBK1...\n");
	status =
		CloseHandle(hDevice);
	if (!status) {
		printf("Failed on call to CloseHandle - error: %d\n",
			GetLastError() );
		return 6;
	}
	printf("Succeeded in closing device...exiting normally\n");*/
	Sleep(2000);
	return 0;
}


	