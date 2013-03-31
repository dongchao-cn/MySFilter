#include <ntifs.h>
#include "BaseDef.h"
#include "BaseFun.h"

// 本驱动对象全局指针
extern PDRIVER_OBJECT gSFilterDriverObject;

// CDO的全局指针
extern PDEVICE_OBJECT gSFilterControlDeviceObject;

// 所有的FastIO都return FALSE; 让其调用IRP
BOOLEAN
SfFastIoCheckIfPossible(
						IN PFILE_OBJECT FileObject,
						IN PLARGE_INTEGER FileOffset,
						IN ULONG Length,
						IN BOOLEAN Wait,
						IN ULONG LockKey,
						IN BOOLEAN CheckForReadOperation,
						OUT PIO_STATUS_BLOCK IoStatus,
						IN PDEVICE_OBJECT DeviceObject
						)
{
	return FALSE;
}

BOOLEAN
SfFastIoRead(
			 IN PFILE_OBJECT FileObject,
			 IN PLARGE_INTEGER FileOffset,
			 IN ULONG Length,
			 IN BOOLEAN Wait,
			 IN ULONG LockKey,
			 OUT PVOID Buffer,
			 OUT PIO_STATUS_BLOCK IoStatus,
			 IN PDEVICE_OBJECT DeviceObject
			 )
{
	return FALSE;
}

BOOLEAN
SfFastIoWrite(
			  IN PFILE_OBJECT FileObject,
			  IN PLARGE_INTEGER FileOffset,
			  IN ULONG Length,
			  IN BOOLEAN Wait,
			  IN ULONG LockKey,
			  IN PVOID Buffer,
			  OUT PIO_STATUS_BLOCK IoStatus,
			  IN PDEVICE_OBJECT DeviceObject
			  )
{
	return FALSE;
}

BOOLEAN
SfFastIoQueryBasicInfo(
					   IN PFILE_OBJECT FileObject,
					   IN BOOLEAN Wait,
					   OUT PFILE_BASIC_INFORMATION Buffer,
					   OUT PIO_STATUS_BLOCK IoStatus,
					   IN PDEVICE_OBJECT DeviceObject
					   )
{
	return FALSE;
}

BOOLEAN
SfFastIoQueryStandardInfo(
						  IN PFILE_OBJECT FileObject,
						  IN BOOLEAN Wait,
						  OUT PFILE_STANDARD_INFORMATION Buffer,
						  OUT PIO_STATUS_BLOCK IoStatus,
						  IN PDEVICE_OBJECT DeviceObject
						  )
{
	return FALSE;
}

BOOLEAN
SfFastIoLock(
			 IN PFILE_OBJECT FileObject,
			 IN PLARGE_INTEGER FileOffset,
			 IN PLARGE_INTEGER Length,
			 PEPROCESS ProcessId,
			 ULONG Key,
			 BOOLEAN FailImmediately,
			 BOOLEAN ExclusiveLock,
			 OUT PIO_STATUS_BLOCK IoStatus,
			 IN PDEVICE_OBJECT DeviceObject
			 )
{
	return FALSE;
}

BOOLEAN
SfFastIoUnlockSingle(
					 IN PFILE_OBJECT FileObject,
					 IN PLARGE_INTEGER FileOffset,
					 IN PLARGE_INTEGER Length,
					 PEPROCESS ProcessId,
					 ULONG Key,
					 OUT PIO_STATUS_BLOCK IoStatus,
					 IN PDEVICE_OBJECT DeviceObject
					 )
{
	return FALSE;
}

BOOLEAN
SfFastIoUnlockAll(
				  IN PFILE_OBJECT FileObject,
				  PEPROCESS ProcessId,
				  OUT PIO_STATUS_BLOCK IoStatus,
				  IN PDEVICE_OBJECT DeviceObject
				  )
{
	return FALSE;
}

BOOLEAN
SfFastIoUnlockAllByKey(
					   IN PFILE_OBJECT FileObject,
					   PVOID ProcessId,
					   ULONG Key,
					   OUT PIO_STATUS_BLOCK IoStatus,
					   IN PDEVICE_OBJECT DeviceObject
					   )
{
	return FALSE;
}

BOOLEAN
SfFastIoDeviceControl(
					  IN PFILE_OBJECT FileObject,
					  IN BOOLEAN Wait,
					  IN PVOID InputBuffer OPTIONAL,
					  IN ULONG InputBufferLength,
					  OUT PVOID OutputBuffer OPTIONAL,
					  IN ULONG OutputBufferLength,
					  IN ULONG IoControlCode,
					  OUT PIO_STATUS_BLOCK IoStatus,
					  IN PDEVICE_OBJECT DeviceObject
					  )
{
	return FALSE;
}

VOID
SfFastIoDetachDevice(
					 IN PDEVICE_OBJECT SourceDevice,
					 IN PDEVICE_OBJECT TargetDevice
					 )
{
	PSFILTER_DEVICE_EXTENSION devExt;

	PAGED_CODE();

	ASSERT(IS_MY_DEVICE_OBJECT( SourceDevice ));

	devExt = (PSFILTER_DEVICE_EXTENSION)SourceDevice->DeviceExtension;

	//
	//  Display name information
	//

	LOG_PRINT("SFilter!SfFastIoDetachDevice:                Detaching from volume      %p \"%wZ\"\n",
		TargetDevice,
		&devExt->DeviceName);

	//
	//  Detach from the file system's volume device object.
	//

	SfCleanupMountedDevice( SourceDevice );
	IoDetachDevice( TargetDevice );
	IoDeleteDevice( SourceDevice );
}

BOOLEAN
SfFastIoQueryNetworkOpenInfo(
							 IN PFILE_OBJECT FileObject,
							 IN BOOLEAN Wait,
							 OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
							 OUT PIO_STATUS_BLOCK IoStatus,
							 IN PDEVICE_OBJECT DeviceObject
							 )
{
	return FALSE;
}

BOOLEAN
SfFastIoMdlRead(
				IN PFILE_OBJECT FileObject,
				IN PLARGE_INTEGER FileOffset,
				IN ULONG Length,
				IN ULONG LockKey,
				OUT PMDL *MdlChain,
				OUT PIO_STATUS_BLOCK IoStatus,
				IN PDEVICE_OBJECT DeviceObject
				)
{
	return FALSE;
}


BOOLEAN
SfFastIoMdlReadComplete(
						IN PFILE_OBJECT FileObject,
						IN PMDL MdlChain,
						IN PDEVICE_OBJECT DeviceObject
						)
{
	return FALSE;
}

BOOLEAN
SfFastIoPrepareMdlWrite(
						IN PFILE_OBJECT FileObject,
						IN PLARGE_INTEGER FileOffset,
						IN ULONG Length,
						IN ULONG LockKey,
						OUT PMDL *MdlChain,
						OUT PIO_STATUS_BLOCK IoStatus,
						IN PDEVICE_OBJECT DeviceObject
						)
{
	return FALSE;
}

BOOLEAN
SfFastIoMdlWriteComplete(
						 IN PFILE_OBJECT FileObject,
						 IN PLARGE_INTEGER FileOffset,
						 IN PMDL MdlChain,
						 IN PDEVICE_OBJECT DeviceObject
						 )
{
	return FALSE;
}

BOOLEAN
SfFastIoReadCompressed(
					   IN PFILE_OBJECT FileObject,
					   IN PLARGE_INTEGER FileOffset,
					   IN ULONG Length,
					   IN ULONG LockKey,
					   OUT PVOID Buffer,
					   OUT PMDL *MdlChain,
					   OUT PIO_STATUS_BLOCK IoStatus,
					   OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
					   IN ULONG CompressedDataInfoLength,
					   IN PDEVICE_OBJECT DeviceObject
					   )
{
	return FALSE;
}

BOOLEAN
SfFastIoWriteCompressed(
						IN PFILE_OBJECT FileObject,
						IN PLARGE_INTEGER FileOffset,
						IN ULONG Length,
						IN ULONG LockKey,
						IN PVOID Buffer,
						OUT PMDL *MdlChain,
						OUT PIO_STATUS_BLOCK IoStatus,
						IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
						IN ULONG CompressedDataInfoLength,
						IN PDEVICE_OBJECT DeviceObject
						)
{
	return FALSE;
}

BOOLEAN
SfFastIoMdlReadCompleteCompressed(
								  IN PFILE_OBJECT FileObject,
								  IN PMDL MdlChain,
								  IN PDEVICE_OBJECT DeviceObject
								  )
{
	return FALSE;
}

BOOLEAN
SfFastIoMdlWriteCompleteCompressed(
								   IN PFILE_OBJECT FileObject,
								   IN PLARGE_INTEGER FileOffset,
								   IN PMDL MdlChain,
								   IN PDEVICE_OBJECT DeviceObject
								   )
{
	return FALSE;
}

BOOLEAN
SfFastIoQueryOpen(
				  IN PIRP Irp,
				  OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
				  IN PDEVICE_OBJECT DeviceObject
				  )
{
	return FALSE;
}

NTSTATUS
SfPreFsFilterPassThrough(
						 IN PFS_FILTER_CALLBACK_DATA Data,
						 OUT PVOID *CompletionContext
						 )
{
	UNREFERENCED_PARAMETER( Data );
	UNREFERENCED_PARAMETER( CompletionContext );

	ASSERT( IS_MY_DEVICE_OBJECT( Data->DeviceObject ) );

	return STATUS_SUCCESS;
}


VOID
SfPostFsFilterPassThrough (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
    )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( OperationStatus );
    UNREFERENCED_PARAMETER( CompletionContext );

    ASSERT( IS_MY_DEVICE_OBJECT( Data->DeviceObject ) );
}
