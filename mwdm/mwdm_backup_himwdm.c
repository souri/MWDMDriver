
#include <wdm.h>
UNICODE_STRING HiImWdmRegistryPath;

ULONG SHARED_MEMORY_SIZE = 4;
UCHAR SharedMemory[4];
UNICODE_STRING devName;
UNICODE_STRING symLinkName;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of local functions

VOID WdmUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS WdmPower(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmPnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmAddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo);

NTSTATUS WdmCreate(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS WdmWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmRead(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS WdmDeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

void RegisterWmi(	IN PDEVICE_OBJECT fdo);

void DeregisterWmi(	IN PDEVICE_OBJECT fdo);

NTSTATUS WdmSystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);


typedef struct _WDM_DEVICE_EXTENSION
{
	PDEVICE_OBJECT	pdo;
	PDEVICE_OBJECT	fdo;
	PDEVICE_OBJECT	NextDevice;
	UNICODE_STRING	ifSymLinkName;
} WDM_DEVICE_EXTENSION, *PDEVICE_EXTENSION;



NTSTATUS DriverEntry(	IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	// Save a copy of our RegistryPath for WMI
	/*HiImWdmRegistryPath.MaximumLength = RegistryPath->MaximumLength;
	HiImWdmRegistryPath.Length = RegistryPath->Length;
	HiImWdmRegistryPath.Buffer = (PWSTR)ExAllocatePool(PagedPool, HiImWdmRegistryPath.MaximumLength);
	if( HiImWdmRegistryPath.Buffer == NULL) return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory( HiImWdmRegistryPath.Buffer, HiImWdmRegistryPath.MaximumLength);
	RtlMoveMemory( HiImWdmRegistryPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);*/
	
	// Export other driver entry points...
	DbgPrint(("MWDM_DRIVER: DriverEntry | Entered"));
 	DriverObject->DriverExtension->AddDevice = WdmAddDevice;
 	DriverObject->DriverUnload = WdmUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = WdmCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = WdmClose;

	DriverObject->MajorFunction[IRP_MJ_PNP] = WdmPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = WdmPower;

	DriverObject->MajorFunction[IRP_MJ_READ] = WdmRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = WdmWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WdmDeviceControl;

	//DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = WdmSystemControl;
	DbgPrint(("MWDM_DRIVER: DriverEntry | Completed"));

	return STATUS_SUCCESS;
}
////////
VOID WdmUnload(IN PDRIVER_OBJECT DriverObject)
{
	DbgPrint(("MWDM_DRIVER: WdmUnload | Entered"));
	ExFreePool(HiImWdmRegistryPath.Buffer);
}



////////

NTSTATUS WdmAddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo)
{
	NTSTATUS status;
    PDEVICE_OBJECT fdo;
	PDEVICE_EXTENSION dx;

	DbgPrint(("MWDM_DRIVER: WdmAddDevice | Entered"));

	RtlInitUnicodeString(&devName, L"\\Device\\MultiWDM");
	RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\MWDM"); 


	// Create our Functional Device Object in fdo
	status = IoCreateDevice(DriverObject,
		sizeof(PDEVICE_EXTENSION),
		&devName,	
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,	// Not exclusive
		&fdo);
	if( NT_ERROR(status))
		return status;

	// Remember fdo in our device extension
	dx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	dx->pdo = pdo;
	dx->fdo = fdo;

	//dx->devName = devName;
	//dx->symLinkName = symLinkName;

	// Register and enable our device interface
	/*status = IoRegisterDeviceInterface(pdo, &WDM_GUID, NULL, &dx->ifSymLinkName);
	if( NT_ERROR(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	IoSetDeviceInterfaceState(&dx->ifSymLinkName, TRUE);*/

	// Attach to the driver stack below us
	dx->NextDevice = IoAttachDeviceToDeviceStack(fdo,pdo);

	// Set fdo flags appropriately
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	fdo->Flags |= DO_BUFFERED_IO;

	DbgPrint(("MWDM_DRIVER: WdmAddDevice | Completed"));

	//RegisterWmi(fdo);

	return STATUS_SUCCESS;
}

NTSTATUS WdmPnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	NTSTATUS status;
	PDEVICE_EXTENSION dx=(PDEVICE_EXTENSION)fdo->DeviceExtension;

	// Remember minor function
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG MinorFunction = stack->MinorFunction;

	DbgPrint(("MWDM_DRIVER: WdmPNP | Entered"));

	// Just pass to lower driver
	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver( dx->NextDevice, Irp);

	// Device removed
	if( MinorFunction==IRP_MN_REMOVE_DEVICE)
	{
		// disable device interface
		IoSetDeviceInterfaceState(&dx->ifSymLinkName, FALSE);
		RtlFreeUnicodeString(&dx->ifSymLinkName);

		//DeregisterWmi(fdo);

		// unattach from stack
		if (dx->NextDevice)
			IoDetachDevice(dx->NextDevice);

		// delete our fdo
		IoDeleteDevice(fdo);
	}
	DbgPrint(("MWDM_DRIVER: WdmAddDevice | Completed"));
	return status;
}


NTSTATUS WdmPower(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PDEVICE_EXTENSION dx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	DbgPrint(("MWDM_DRIVER: WdmPower | Entered"));

	// Just pass to lower driver
	PoStartNextPowerIrp( Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver( dx->NextDevice, Irp);
}

/////////////////////////////////////////////////////////////////////////////

NTSTATUS WdmCreate(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	// Complete successfully
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	DbgPrint(("MWDM_DRIVER: WdmCreate | Completed"));
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////

NTSTATUS WdmClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	// Complete successfully
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	DbgPrint(("MWDM_DRIVER: WdmClose | Completed"));
	return STATUS_SUCCESS;
}
 
/////////////////////////////////////////////////////////////////////////////

NTSTATUS WdmRead(IN PDEVICE_OBJECT fdo,
				IN PIRP Irp)
{
	PDEVICE_EXTENSION dx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;


	ULONG ReadLen = pIrpStack->Parameters.Read.Length;

	DbgPrint(("MWDM_DRIVER: WdmAddRead | Started"));	
	if( ReadLen>SHARED_MEMORY_SIZE)
		status = STATUS_INVALID_PARAMETER;
	else if( ReadLen>0)
	{
		RtlMoveMemory( Irp->AssociatedIrp.SystemBuffer, SharedMemory, ReadLen);
		BytesTxd = ReadLen;
	}

	// Complete IRP
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = BytesTxd;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/////////////////////////////////////////////////////////////////////////////

NTSTATUS WdmWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PDEVICE_EXTENSION dx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;

	ULONG WriteLen = pIrpStack->Parameters.Write.Length;

	DbgPrint(("MWDM_DRIVER: WdmWrite | Entered"));
	if( WriteLen>SHARED_MEMORY_SIZE)
		status = STATUS_INVALID_PARAMETER;
	else if( WriteLen>0)
	{
		RtlMoveMemory( SharedMemory, Irp->AssociatedIrp.SystemBuffer, WriteLen);
		BytesTxd = WriteLen;
	}

	// Complete IRP
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = BytesTxd;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/////////////////////////////////////////////////////////////////////////////

NTSTATUS WdmDeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	// Complete successfully
	DbgPrint(("MWDM_DRIVER: WdmAddDeviceControl | Entered"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}