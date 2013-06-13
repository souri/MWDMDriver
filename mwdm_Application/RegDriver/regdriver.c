#include <windows.h>
#include <stdio.h>
#include "string.h"

#define IOCTL_READ1_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ2_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ3_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ4_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ5_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

int __cdecl main(int argc,char **argv) {
	HANDLE hDevice;
	BOOL status;
	char outBuffer[10] = {NULL};
	int outCount, bW;

	char inBuffer[100] = {NULL};
	DWORD inCount = sizeof(inBuffer);
	DWORD bR,dwRead;
	int choice = 0;
	char looptext[100] = {NULL};

	char str[100],str1[100];
	int n;

	printf("Beggining test of MWDM Driver...\n");

	hDevice =CreateFile("\\\\.\\MWDM",
					GENERIC_READ | GENERIC_WRITE,
					0,		// share mode none
					NULL,	// no security
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL );		
	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MWDM", GetLastError() );
		return 1;
	}
	
	printf("*****Multifunction Driver Test Utility*****\n\n");	
	
	printf("1.Loopback data\n");
	printf("2.Check Timer Functions the Driver\n");
	printf("3.Check IOCTL routines\n");
	printf("4.Access PCI configuration address space\n");
	printf("5.Check Power Status\n");
	printf("Enter the Task you want to run\n");
	
	scanf("%d",&choice);
	switch(choice)
	{
		case 1:
			strcpy(outBuffer,"1\0");
			outCount = strlen(outBuffer)+1;
			printf("Trying WriteFile :)\n");
			status = WriteFile(hDevice, outBuffer, 2, &bW, NULL);
			//status = WriteFile(hDevice, "123\0", 4, &bW, NULL);
			printf("WriteFile reported status: %d\n", status);
			printf("Enter text to send to Driver\n");
			scanf("%s",&looptext);
			outCount = strlen(looptext)+1;
			printf("Writing data to be looped back :)\n");
			status = WriteFile(hDevice, looptext, outCount, &bW, NULL);
			printf("WriteFile reported status: %d\n", status);
			printf("Attempting to read from device...\n");
			status =ReadFile(hDevice, inBuffer, 100, &bR, NULL);
			printf("Read data: %s\n",inBuffer);
			break;

		case 2:
			strcpy(outBuffer,"2\0");
			printf("Trying WriteFile :)\n");
			status = WriteFile(hDevice, outBuffer, 2, &bW, NULL);
			status = WriteFile(hDevice, outBuffer, 2, &bW, NULL);
			printf("WriteFile reported status: %d\n", status);
			break;

		case 3:
			strcpy(outBuffer,"3\0");
			memset(str,sizeof(str),NULL); 
			memset(str1,sizeof(str1),NULL);
			printf("Enter the value of timer interval in seconds:\n");
			fflush(stdin);
			gets(str);
			DeviceIoControl(hDevice, IOCTL_READ3_FILE ,str ,sizeof(str),str1 ,sizeof(str1), &dwRead, NULL);
			break;

		case 4:
			strcpy(outBuffer,"4\0");
			break;

		case 5:
			strcpy(outBuffer,"5\0");
			break;
		default: printf("Wrong Choice | You entered: %d\n", choice);
				break;
	}

	//outCount = strlen(outBuffer)+1;
	//printf("Trying WriteFile :)\n");
	//status = WriteFile(hDevice, outBuffer, 2, &bW, NULL);
	////status = WriteFile(hDevice, "123\0", 4, &bW, NULL);
	//printf("WriteFile reported status: %d\n", status);


	printf("Attempting to close device MTD...\n");
	status =CloseHandle(hDevice);
	if (!status) {
		printf("Failed on call to CloseHandle - error: %d\n",
			GetLastError() );
		return 6;
	}
	printf("Succeeded in closing device...exiting normally\n");
	Sleep(2000);
	return 0;
}


	