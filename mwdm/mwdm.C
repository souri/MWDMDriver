#include "wdm.h"
#include "ntddk.h"
#include <stdlib.h>

#define NT_DEVICE_NAME  L"\\Device\\MyWDM"
#define DOS_DEVICE_NAME  L"\\DosDevices\\MWDM"

#define IOCTL_READ1_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ2_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ3_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ4_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ5_FILE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct tagDEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;       // device object this driver creates
    PDEVICE_OBJECT NextDeviceObject;   // next-layered device object in this
                                       // device stack
    DEVICE_CAPABILITIES pdc;           // device capability
    IO_REMOVE_LOCK RemoveLock;         // removal control locking structure
    LONG handles;                      // # open handles
    PVOID DataBuffer;                  // Internal Buffer for Read/Write I/O
    UNICODE_STRING Device_Description; // Device Description
    SYSTEM_POWER_STATE SysPwrState;    // Current System Power State
    DEVICE_POWER_STATE DevPwrState;    // Current Device Power State
    PIRP PowerIrp;                     // Current Handling Power-Related IRP
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT PhysicalDeviceObject);
NTSTATUS PsdoDispatchCreate(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS PsdoDispatchClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS PsdoDispatchRead(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS PsdoDispatchWrite(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS PsdoDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS PsdoDispatchPower(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS PsdoDispatchPnP(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
VOID TimerDpc(IN PKDPC Dpc,	IN PVOID DeferredContext,IN PVOID SystemArgument1,IN PVOID SystemArgument2); 
VOID DriverUnload (PDRIVER_OBJECT pDriverObject);
VOID waitontimer();

char looptext[100] = {NULL};
int dispatch_write_callcount = 0, choice;
KTIMER timer;
KDPC dpc;
int usertime = 1;
LARGE_INTEGER DueTime={-10000000}; // units of 100ns -> 1sec
LONG timer_period = 1000;
PKDEFERRED_ROUTINE deferred_routine;
int timercount = 0;

NTSTATUS DriverEntry( 
IN PDRIVER_OBJECT DriverObject, 
IN PUNICODE_STRING RegistryPath 
)
{
	
UNICODE_STRING Global_sz_Drv_RegInfo;
RtlInitUnicodeString(
&Global_sz_Drv_RegInfo,
RegistryPath->Buffer);

// Initialize function pointers
DbgPrint("MWDM_DRIVER: Driver entry | Started\n");
DriverObject->DriverUnload = DriverUnload;
DriverObject->DriverExtension->AddDevice = AddDevice;

DriverObject->MajorFunction[IRP_MJ_CREATE] = PsdoDispatchCreate;
DriverObject->MajorFunction[IRP_MJ_CLOSE] = PsdoDispatchClose;
DriverObject->MajorFunction[IRP_MJ_READ] = PsdoDispatchRead;
DriverObject->MajorFunction[IRP_MJ_WRITE] = PsdoDispatchWrite;
DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PsdoDispatchDeviceControl;
DriverObject->MajorFunction[IRP_MJ_POWER] = PsdoDispatchPower;
DriverObject->MajorFunction[IRP_MJ_PNP] = PsdoDispatchPnP;


DbgPrint("MWDM_DRIVER: Driver entry | Done\n");
return STATUS_SUCCESS;
}

NTSTATUS
AddDevice(
IN PDRIVER_OBJECT DriverObject,
IN PDEVICE_OBJECT PhysicalDeviceObject 
)
{
	
	UNICODE_STRING Global_sz_DeviceName;
    ULONG DeviceExtensionSize;
    PDEVICE_EXTENSION p_DVCEXT;
    PDEVICE_OBJECT ptr_PDO;
    NTSTATUS status;
	UNICODE_STRING uniNtNameString;
	DbgPrint("MWDM_DRIVER: AddDevice | Started\n");
	RtlInitUnicodeString(&uniNtNameString,NT_DEVICE_NAME);

    //Get DEVICE_EXTENSION required memory space
    DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    //Create Device Object
    status = IoCreateDevice(
        DriverObject,
        DeviceExtensionSize,
        &uniNtNameString,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN, 
        FALSE,
        &ptr_PDO
        );

    if (NT_SUCCESS(status))
	{
        ptr_PDO->Flags &= ~DO_DEVICE_INITIALIZING;
		ptr_PDO->Flags |= DO_BUFFERED_IO;  //For Buffered I/O
        //ptr_PDO->Flags |= DO_DIRECT_IO;  //For Direct I/O</FONT>
        p_DVCEXT = ptr_PDO->DeviceExtension;
        p_DVCEXT->DeviceObject = ptr_PDO;
      
		RtlInitUnicodeString(&Global_sz_DeviceName,DOS_DEVICE_NAME);

		status=IoCreateSymbolicLink(&Global_sz_DeviceName,&uniNtNameString);
		if(!NT_SUCCESS(status))
		{
			DbgPrint("MWDM_DRIVER: AddDevice | Couldn't create the symbolic link\n");
			IoDeleteDevice(DriverObject->DeviceObject);
			return status;
		}
		DbgPrint("MWDM_DRIVER: AddDevice | Symlink created\n");

        //Store next-layered device object
        //Attach device object to device stack
        p_DVCEXT->NextDeviceObject = 
            IoAttachDeviceToDeviceStack(ptr_PDO, PhysicalDeviceObject);
    }
	DbgPrint("MWDM_DRIVER: AddDevice | Done\n");
    return status;
}

NTSTATUS
PsdoDispatchCreate(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
    //PIO_STACK_LOCATION p_IO_STK;
    //PDEVICE_EXTENSION p_DVCEXT;
    //NTSTATUS status;
	DbgPrint("MWDM_DRIVER: DispatchCreate | Started\n");
	/*p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
    p_DVCEXT = DeviceObject->DeviceExtension;
    status = IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, p_IO_STK->FileObject);
    if (NT_SUCCESS(status)) 
	{*/
        Irp->IoStatus.Status=STATUS_SUCCESS;
		Irp->IoStatus.Information=0;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
		DbgPrint("MWDM_DRIVER: DispatchCreate | Out of Createroutine\n");
		return STATUS_SUCCESS;
	/*} 
	else 
	{
        IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, p_IO_STK->FileObject);
        Irp->IoStatus.Status=STATUS_SUCCESS;
		Irp->IoStatus.Information=0;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
		return STATUS_SUCCESS;
    }*/
		DbgPrint("MWDM_DRIVER: DispatchCreate | Done\n");
}

NTSTATUS
PsdoDispatchClose(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{	
	DbgPrint("MWDM_DRIVER: DispatchClose | Started\n");
	/*PIO_STACK_LOCATION p_IO_STK;
    PDEVICE_EXTENSION p_DVCEXT;

    p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
    p_DVCEXT = DeviceObject->DeviceExtension;
    IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, 
    p_IO_STK->FileObject);*/

    Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	
	DbgPrint("MWDM_DRIVER: DispatchClose | Done\n");
	return STATUS_SUCCESS;
}

NTSTATUS
PsdoDispatchRead(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{

	DbgPrint("MWDM_DRIVER: DispatchRead | Started\n");
	if (choice == 1)
	{
		KdPrint(("Writing Looptext: %s\n", looptext));
		RtlCopyMemory(Irp->UserBuffer, looptext,100);
	}
	else if (choice == 2)
	{
		KdPrint(("Writing TimerCount: %s\n", looptext));
		/*RtlCopyMemory(Irp->UserBuffer, (char)timercount,100);*/
	}


	Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);	    
	//KdPrint(("UserString: %s\n", Irp->AssociatedIrp.SystemBuffer));
	DbgPrint("MWDM_DRIVER: DispatchRead | Done\n");
	return STATUS_SUCCESS;
}

NTSTATUS
PsdoDispatchWrite(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
	NTSTATUS status;
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	ANSI_STRING RxChoice;
	UNICODE_STRING U_RxChoiceDrv;

	PVOID Buf;		//Buffer provided by user program
	ULONG BufLen;	//Buffer length for user provided buffer
	LONGLONG Offset;//Buffer Offset
	PVOID DataBuf;  //Buffer provided by Driver
	ULONG DataLen;  //Buffer length for Driver Data Buffer

	HANDLE tmrthreadhandle;
	DbgPrint("MWDM_DRIVER: DispatchWrite | Started\n");
	/*p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;

	BufLen = p_IO_STK->Parameters.Read.Length;
	Offset = p_IO_STK->Parameters.Read.ByteOffset.QuadPart;
	Buf = (PUCHAR)(Irp->AssociatedIrp.SystemBuffer) + Offset;*/
	
	if (dispatch_write_callcount == 0) //update count and exit routine
	{
	KdPrint(("Count = 0, so updating choice"));
	KdPrint(("UserBuffer: %s\n", Irp->AssociatedIrp.SystemBuffer));
	choice = atoi(Irp->AssociatedIrp.SystemBuffer);
	KdPrint(("Choice: %d\n", choice));
	dispatch_write_callcount =1;
	Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);	    
	KdPrint(("MWDM_DRIVER: DispatchWrite | Done | Choice: %d | Callcount: %d\n",choice,dispatch_write_callcount));
	return STATUS_SUCCESS;
	}

	/*RtlInitAnsiString(&RxChoice, Irp->AssociatedIrp.SystemBuffer);
	RtlAnsiStringToUnicodeString(&U_RxChoiceDrv,&RxChoice,TRUE);
	choice = (char)U_RxChoiceDrv.Buffer;
	KdPrint(("Recieved Choice: %ws | Choice:%d\n", U_RxChoiceDrv.Buffer, choice));*/
	
	if(choice==1 && dispatch_write_callcount ==1)
	{
		//only choice gets registered in the first call of write, at next call this routine works
		KdPrint (("User Entered 1 | Starting Task 1\n"));
		KdPrint(("UserString: %s\n", Irp->AssociatedIrp.SystemBuffer));
		RtlCopyMemory(&looptext, Irp->AssociatedIrp.SystemBuffer,100);
		KdPrint(("LoopText: %s\n", looptext));
		dispatch_write_callcount=0;

	}
	else if(choice==2 && dispatch_write_callcount ==1)
	{
		KdPrint(("User Entered 2 | Starting Task 2"));
		KeInitializeTimer(&timer); /* pointer to timer object */
		KeInitializeDpc(&dpc, /* pointer to dpc	object */TimerDpc, /* pointer to timer dpc routine */&p_DVCEXT); /* context (device extension) */
		//KeInitializeSpinLock(&dev_ext_ptr->lock); /* pointer to spin lock */
		(void) KeSetTimerEx(&timer, /* pointer to timer object */	DueTime, /* due time, for one-shot only */
		timer_period, /* period in	milliseconds */	&dpc); /* pointer to dpc object */

		status = PsCreateSystemThread(&tmrthreadhandle, GENERIC_READ | GENERIC_WRITE, NULL, NULL, NULL, waitontimer, NULL);
		
		KdPrint(("Done with task 2, | Final TimerCount = %d\n",timercount));
		dispatch_write_callcount=0;
	}
	else if (choice==3 && dispatch_write_callcount ==1)
	{
		KdPrint (("User Entered 3 | Starting Task 3"));
		dispatch_write_callcount=0;
	}
	else if (choice==4 && dispatch_write_callcount ==1)
	{
		KdPrint (("User Entered 4 | Starting Task 4"));
		dispatch_write_callcount=0;
	}
	else if (choice==5 && dispatch_write_callcount ==1)
	{
		KdPrint (("User Entered 5 | Starting Task 5"));
		dispatch_write_callcount=0; 
	}
	else {
		KdPrint (("Could not determine what User entered | Wrong choice entered! | Choice: %d | Callcount: %d\n",choice,dispatch_write_callcount));
		dispatch_write_callcount=0;
	}
	Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);	    
	KdPrint(("MWDM_DRIVER: DispatchWrite | Done | Choice: %d | Callcount: %d\n",choice,dispatch_write_callcount));
	return STATUS_SUCCESS;
}

NTSTATUS
PsdoDispatchDeviceControl(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
	
	DbgPrint("MWDM_DRIVER: DispatchDeviceControl | Started\n");
	KdPrint(("MWDM_DRIVER: DispatchDeviceControl | cbin: %d\n",cbin));
	KdPrint(("MWDM_DRIVER: DispatchDeviceControl | cbout: %d\n",cbout));

	switch (code)
	{
		case IOCTL_READ3_FILE:				// code == 0x802
		{	
			KdPrint(("MWDM_DRIVER: DispatchDeviceControl | Task3 Start"));
			__try
			{
				KdPrint(("MWDM_DRIVER: DispatchDeviceControl | Task3 | BUFFER CONTENTS:%s\n",Irp->AssociatedIrp.SystemBuffer));
				usertime = atoi(Irp->AssociatedIrp.SystemBuffer);
				KdPrint(("Initial values: DueTime.Highpart:%ld, DueTime.Lowpart:%d , DueTime.Quadpart:%ll\n", DueTime.HighPart, DueTime.LowPart, DueTime.QuadPart));
				DueTime.LowPart = -10000000;
				DueTime.LowPart = DueTime.LowPart*usertime;
				timer_period = 1000;
				timer_period = timer_period * usertime;
				KdPrint(("Timer will now use an interval of: %d seconds",usertime));
				KdPrint(("TimerPeriod updated | DueTime->LowPart now: %d | Timer_Period: %l UserTime: %d", DueTime.LowPart, timer_period, usertime));
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				DbgPrint( "MWDM_DRIVER: DispatchDeviceControl |  Task3 | Task3IOCTL_READ2_FILE failed");
				Irp->IoStatus.Status		= STATUS_ACCESS_DENIED;
				Irp->IoStatus.Information	= 0;
				IoCompleteRequest( Irp, IO_NO_INCREMENT );
				return STATUS_ACCESS_DENIED;
			}  
		Irp->IoStatus.Status=STATUS_SUCCESS;
		Irp->IoStatus.Information=cbin;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
		KdPrint(("MWDM_DRIVER: DispatchDeviceControl | Task3 | Done"));
		return STATUS_SUCCESS;
		}
		default:
			KdPrint(("MWDM_DRIVER: DispatchDeviceControl | Entered default case!"));
	}
    Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	DbgPrint("MWDM_DRIVER: DispatchDeviceControl | Done\n");
	return STATUS_SUCCESS;
}

NTSTATUS
PsdoDispatchPower(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
	DbgPrint("MWDM_DRIVER: DispatchPower | Started\n");
    /*PIO_STACK_LOCATION p_IO_STK;
    PDEVICE_EXTENSION p_DVCEXT;

    p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
    p_DVCEXT = DeviceObject->DeviceExtension;
    IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, 
    p_IO_STK->FileObject);*/

    Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	DbgPrint("MWDM_DRIVER: DispatchPower | Done\n");
	return STATUS_SUCCESS;
}

NTSTATUS
PsdoDispatchPnP(
IN PDEVICE_OBJECT DeviceObject,
IN PIRP Irp
)
{
	DbgPrint("MWDM_DRIVER: DispatchPNP | Started\n");
    /*PIO_STACK_LOCATION p_IO_STK;
    PDEVICE_EXTENSION p_DVCEXT;

    p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
    p_DVCEXT = DeviceObject->DeviceExtension;
    IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, 
    p_IO_STK->FileObject);*/

    Irp->IoStatus.Status=STATUS_SUCCESS;
	Irp->IoStatus.Information=0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	DbgPrint("MWDM_DRIVER: DispatchPNP | Done\n");
	return STATUS_SUCCESS;
}

VOID DriverUnload (PDRIVER_OBJECT pDriverObject) 
{
	
	PDEVICE_OBJECT pNextObj;
	DbgPrint("MWDM_DRIVER: DriverUnload | Started\n");
	// Loop through each device controlled by Driver
	KeCancelTimer (&timer);
	pNextObj = pDriverObject->DeviceObject;

	while (pNextObj != NULL) 
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION) pNextObj->DeviceExtension;

		//...
		// UNDO whatever is done in Driver Entry
		//
		// ... delete symbolic link name
		//IoDeleteSymbolicLink(&pDevExt->symLinkName);

		pNextObj = pNextObj->NextDevice;

		// then delete the device using the Extension
		IoDeleteDevice( pDevExt->DeviceObject);
	}
	DbgPrint("MWDM_DRIVER: DriverUnload | Done\n");
	return;
}


void TimerDpc(
		IN PKDPC Dpc,
		IN PVOID DeferredContext,
		IN PVOID SystemArgument1,
		IN PVOID SystemArgument2)
		{
		if(timercount<10)
		{
		KdPrint(("MWDM_DRIVER: Entered timerDpc"));
		timercount+=1;
		}
		else
		{
			KeCancelTimer (&timer);
			timercount =0;
		}
		KdPrint(("MWDM_DRIVER: Timer Count: %d",timercount));
		return;
		}

void waitontimer()
{
	NTSTATUS status;
	//while(timercount<=5)
	//{
	//KdPrint(("MWDM_DRIVER: waitontimer | Waiting for Timer Event\n"));
	//status = KeWaitForSingleObject(&timer, Executive, KernelMode, TRUE, NULL); //NULL -> Infinite wait
	//KdPrint(("MWDM_DRIVER: waitontimer | Got Timer Event | Status: %x\n",status));
	//timercount+=1;
	//status = KeResetEvent(&timer);
	//KdPrint(("MWDM_DRIVER: waitontimer | TimerEventReset | Status: %x\n",status));
	//KdPrint(("MWDM_DRIVER: waitontimer | TimerCount : %d", timercount));
	//}
	return;
}