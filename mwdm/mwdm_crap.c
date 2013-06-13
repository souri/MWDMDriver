#include <wdm.h>

UNICODE_STRING Global_sz_Drv_RegInfo;
UNICODE_STRING Global_sz_DeviceName;
//PDEVICE_POWER_INFORMATION Global_PowerInfo_Ptr;

NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject,IN PUNICODE_STRING  RegistryPath);

DRIVER_ADD_DEVICE AddDevice;
NTSTATUS AddDevice(IN PDRIVER_OBJECT  DriverObject,IN PDEVICE_OBJECT  PhysicalDeviceObject);

DRIVER_UNLOAD DriverUnload;
VOID DriverUnload(IN PDRIVER_OBJECT  DriverObject); 
NTSTATUS PsdoDispatchCreate(IN PDEVICE_OBJECT  DeviceObject,IN PIRP  Irp);
NTSTATUS PsdoDispatchClose(IN PDEVICE_OBJECT  DeviceObject,IN PIRP  Irp);
NTSTATUS PsdoDispatchDeviceControl(IN PDEVICE_OBJECT  DeviceObject,IN PIRP  Irp);

/*
Device Extension Structure defined by WDM writer
*/
typedef struct tagDEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;		// device object this driver creates
	PDEVICE_OBJECT NextDeviceObject;	// next-layered device object in this device stack
	DEVICE_CAPABILITIES pdc;			// device capability
	IO_REMOVE_LOCK RemoveLock;			// removal control locking structure
	LONG handles;						// # open handles
	PVOID DataBuffer;                   // Internal Buffer for Read/Write I/O
	UNICODE_STRING Device_Description;	// Device Description
 //       SYSTEM_POWER_STATE SysPwrState;		// Current System Power State
 //       DEVICE_POWER_STATE DevPwrState;		// Current Device Power State
	//PIRP PowerIrp;							// Current Handling Power-Related IRP
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/*
Device Power State Structure for Device Capability Handling
*/
//typedef struct tagDEVICE_POWER_INFORMATION {
//  BOOLEAN   SupportQueryCapability;
//  ULONG  DeviceD1;
//  ULONG  DeviceD2;
//  ULONG  WakeFromD0;
//  ULONG  WakeFromD1;
//  ULONG  WakeFromD2;
//  ULONG  WakeFromD3;
//  SYSTEM_POWER_STATE  SystemWake;
//  DEVICE_POWER_STATE  DeviceWake;
//  DEVICE_POWER_STATE  DeviceState[PowerSystemMaximum];
//} DEVICE_POWER_INFORMATION, *PDEVICE_POWER_INFORMATION;

//IOCTL Code
//For Dev Info Ret
//#define IOCTL_READ_DEVICE_INFO	          \
//	CTL_CODE(		          \
//		 FILE_DEVICE_UNKNOWN,	  \
//		 0x800,			  \
//		 METHOD_BUFFERED,	  \
//		 FILE_ANY_ACCESS)
//Power info
//#define IOCTL_READ_POWER_INFO \
//        CTL_CODE(	      \
//        FILE_DEVICE_UNKNOWN,  \
//        0x801,                \
//        METHOD_BUFFERED,      \
//        FILE_ANY_ACCESS)

//Default Stuff
NTSTATUS CompleteRequest(
         IN PIRP Irp,
         IN NTSTATUS status,
         IN ULONG_PTR info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

//Start**************************************
NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject,IN PUNICODE_STRING  RegistryPath)
{
	DbgPrint("In DriverEntry : Begin\r\n");

	RtlInitUnicodeString(&Global_sz_Drv_RegInfo, RegistryPath->Buffer);
	
	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = PsdoDispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PsdoDispatchClose;
	//DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PsdoDispatchDeviceControl;
	/*DriverObject->MajorFunction[IRP_MJ_POWER] = PsdoDispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = PsdoDispatchPnP;*/

	KdPrint(("In DriverEntry : End\r\n"));

	return STATUS_SUCCESS;
}

NTSTATUS AddDevice(IN PDRIVER_OBJECT  DriverObject,IN PDEVICE_OBJECT  PhysicalDeviceObject)
{
	UNICODE_STRING symLinkName;
	ULONG DeviceExtensionSize;
	PDEVICE_EXTENSION p_DVCEXT;
	PDEVICE_OBJECT ptr_PDO;
	NTSTATUS status;
	//ULONG IdxPwrState;

	DbgPrint("In AddDevice : Begin\r\n");

	RtlInitUnicodeString(&Global_sz_DeviceName,L"\\Device\\MultiWDM");
	RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\MWDM"); 
	//Get DEVICE_EXTENSION required memory space
	DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
	
	status = IoCreateDevice(
		DriverObject,
		DeviceExtensionSize,
		&Global_sz_DeviceName,
		FILE_DEVICE_UNKNOWN, 
		0, 
		FALSE,
		&ptr_PDO
    );

	KdPrint(("AddDevice | IoCreateDevice Status: %d",status));

	if (NT_SUCCESS(status)) {
		//Set Device Object Flags
		//ptr_PDO->Flags &= ~DO_DEVICE_INITIALIZING;
		ptr_PDO->Flags |= DO_BUFFERED_IO;

		//Device Extension memory maps
		p_DVCEXT = ptr_PDO->DeviceExtension;
		p_DVCEXT->DeviceObject = ptr_PDO;
		
		//Initialize driver description string
		/*RtlInitUnicodeString(
			&p_DVCEXT->Device_Description,
			L"This is a Pseudo Device Driver\r\n"
			L"Created by Souri\r\n");*/
		/*IoInitializeRemoveLock(
			&p_DVCEXT->RemoveLock,
			'KCOL',
			0,
			0
		);*/
		status = IoCreateSymbolicLink(&symLinkName,&Global_sz_DeviceName);

		//Initialize driver power state
		//p_DVCEXT->SysPwrState = PowerSystemWorking;
		//p_DVCEXT->DevPwrState = PowerDeviceD0;
		//
		////Initialize device power information
		//Global_PowerInfo_Ptr = ExAllocatePool(
		//	NonPagedPool, sizeof(DEVICE_POWER_INFORMATION));
		//RtlZeroMemory(
		//	Global_PowerInfo_Ptr,
		//	sizeof(DEVICE_POWER_INFORMATION));
		//Global_PowerInfo_Ptr->SupportQueryCapability = FALSE;
		//Global_PowerInfo_Ptr->DeviceD1 = 0;
		//Global_PowerInfo_Ptr->DeviceD2 = 0;
		//Global_PowerInfo_Ptr->WakeFromD0 = 0;
		//Global_PowerInfo_Ptr->WakeFromD1 = 0;
		//Global_PowerInfo_Ptr->WakeFromD2 = 0;
		//Global_PowerInfo_Ptr->WakeFromD3 = 0;
		//Global_PowerInfo_Ptr->DeviceWake = 0;
		//Global_PowerInfo_Ptr->SystemWake = 0;
		//for (IdxPwrState = 0;
		//	IdxPwrState < PowerSystemMaximum;
		//	IdxPwrState++)
		//{
		//	Global_PowerInfo_Ptr->DeviceState[IdxPwrState] = 0;
		//}

		//Store next-layered device object
		//Attach device object to device stack
		p_DVCEXT->NextDeviceObject = NULL;
		p_DVCEXT->NextDeviceObject = IoAttachDeviceToDeviceStack(ptr_PDO, PhysicalDeviceObject);
		if(p_DVCEXT->NextDeviceObject != NULL){
			KdPrint(("p_DVCEXT->NextDeviceObject successfuly attached to stack\n"));
		}
	}

	KdPrint(("In AddDevice : End | Symlink Status: %d\r\n",status));


	return status;
}

VOID DriverUnload(IN PDRIVER_OBJECT  DriverObject)
{
	PDEVICE_EXTENSION p_DVCEXT;

	DbgPrint("In DriverUnload : Begin\r\n");

	p_DVCEXT = DriverObject->DeviceObject->DeviceExtension;
	
	//ExFreePool(Global_PowerInfo_Ptr);
	RtlFreeUnicodeString(&Global_sz_Drv_RegInfo);
	/*RtlFreeUnicodeString(&p_DVCEXT->Device_Description);*/
	IoDetachDevice(p_DVCEXT->DeviceObject);
	IoDeleteDevice(p_DVCEXT->NextDeviceObject);

	DbgPrint("In DriverUnload : End\r\n");
	return;
}

NTSTATUS PsdoDispatchCreate(IN PDEVICE_OBJECT  DeviceObject, IN PIRP  Irp)
{
	DbgPrint("IRP_MJ_CREATE Received : Begin\r\n");
	DbgPrint("IRP_MJ_CREATE Received : End\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS PsdoDispatchClose(IN PDEVICE_OBJECT  DeviceObject,IN PIRP  Irp)
{
	DbgPrint("IRP_MJ_CLOSE Received : Begin\r\n");
	
	return STATUS_SUCCESS;
}

//NTSTATUS PsdoDispatchDeviceControl(IN PDEVICE_OBJECT  DeviceObject,IN PIRP  Irp)
//{
//	ULONG code, cbin, cbout, info, pwrinf_size;
//	PIO_STACK_LOCATION p_IO_STK;
//	PDEVICE_EXTENSION p_DVCEXT;
//	//PDEVICE_POWER_INFORMATION pValue;
//	//ULONG IdxPwrState;
//	NTSTATUS status;
//
//	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
//	p_DVCEXT = DeviceObject->DeviceExtension;
//	code = p_IO_STK->Parameters.DeviceIoControl.IoControlCode;
//	cbin = p_IO_STK->Parameters.DeviceIoControl.InputBufferLength;
//	cbout = p_IO_STK->Parameters.DeviceIoControl.OutputBufferLength;
//	IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, Irp);
//
//	switch(code)
//	{
//	case IOCTL_READ_DEVICE_INFO:
//		if (p_DVCEXT->Device_Description.Length > cbout) 
//		{
//			cbout = p_DVCEXT->Device_Description.Length;
//			info = cbout;
//		} else {
//			info = p_DVCEXT->Device_Description.Length;
//		}
//			
//		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
//			p_DVCEXT->Device_Description.Buffer,
//			info);
//		status = STATUS_SUCCESS;
//		break;
////	case IOCTL_READ_POWER_INFO:
////		pwrinf_size = sizeof(DEVICE_POWER_INFORMATION);
////		if (pwrinf_size > cbout)
////		{
////			cbout = pwrinf_size;
////			info = cbout;
////		} else {
////			info = pwrinf_size;
////		}
////		//Display Related Device Power State
////		DbgPrint("Support Query Device Capability : %$r\n", 
////			Global_PowerInfo_Ptr->SupportQueryCapability ? "Yes" : "No");
////		DbgPrint("DeviceD1 : %d\r\n", Global_PowerInfo_Ptr->DeviceD1);
////		DbgPrint("DeviceD2 : %d\r\n", Global_PowerInfo_Ptr->DeviceD2);
////		DbgPrint("WakeFromD0 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD0);
////		DbgPrint("WakeFromD1 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD1);
////		DbgPrint("WakeFromD2 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD2);
////		DbgPrint("WakeFromD3 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD3);
////		DbgPrint("SystemWake : %d\r\n", Global_PowerInfo_Ptr->SystemWake);
////		DbgPrint("DeviceWake : %d\r\n", Global_PowerInfo_Ptr->DeviceWake);
////		for (IdxPwrState = 0; 
////			IdxPwrState < PowerSystemMaximum;
////			IdxPwrState++)
////		{
////			DbgPrint("DeviceState[%d] : %d\r\n", 
////				IdxPwrState, 
////				Global_PowerInfo_Ptr->DeviceState[IdxPwrState]);
////		}
////#ifdef _DEF_HANDLE_BY_POWER_INFO_STRUCTURE
////		pValue = (PDEVICE_POWER_INFORMATION)
////			Irp->AssociatedIrp.SystemBuffer;
////		pValue->SupportQueryCapability = Global_PowerInfo_Ptr->SupportQueryCapability;
////		pValue->DeviceD1 = Global_PowerInfo_Ptr->DeviceD1;
////		pValue->DeviceD2 = Global_PowerInfo_Ptr->DeviceD2;
////		pValue->DeviceWake = Global_PowerInfo_Ptr->DeviceWake;
////		pValue->SystemWake = Global_PowerInfo_Ptr->SystemWake;
////		pValue->WakeFromD0 = Global_PowerInfo_Ptr->WakeFromD0;
////		pValue->WakeFromD1 = Global_PowerInfo_Ptr->WakeFromD1;
////		pValue->WakeFromD2 = Global_PowerInfo_Ptr->WakeFromD2;
////		pValue->WakeFromD3 = Global_PowerInfo_Ptr->WakeFromD3;
////		for (IdxPwrState = 0; 
////			IdxPwrState < PowerSystemMaximum;
////			IdxPwrState++)
////		{
////			pValue->DeviceState[IdxPwrState] = 
////			Global_PowerInfo_Ptr->DeviceState[IdxPwrState];
////		}
////#else
////		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
////			Global_PowerInfo_Ptr,
////			info);
////#endif
////		status = STATUS_SUCCESS;
////		break;
//	default:
//		info = 0;
//		status = STATUS_INVALID_DEVICE_REQUEST;
//		break;
//	}
//
//	IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);
//
//	CompleteRequest(Irp, STATUS_SUCCESS, info);
//	return status;
//}

