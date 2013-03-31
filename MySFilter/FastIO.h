
// FastIOÀý³Ì¶¨Òå
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
						);

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
			 );

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
			  );

BOOLEAN
SfFastIoQueryBasicInfo(
					   IN PFILE_OBJECT FileObject,
					   IN BOOLEAN Wait,
					   OUT PFILE_BASIC_INFORMATION Buffer,
					   OUT PIO_STATUS_BLOCK IoStatus,
					   IN PDEVICE_OBJECT DeviceObject
					   );

BOOLEAN
SfFastIoQueryStandardInfo(
						  IN PFILE_OBJECT FileObject,
						  IN BOOLEAN Wait,
						  OUT PFILE_STANDARD_INFORMATION Buffer,
						  OUT PIO_STATUS_BLOCK IoStatus,
						  IN PDEVICE_OBJECT DeviceObject
						  );

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
			 );

BOOLEAN
SfFastIoUnlockSingle(
					 IN PFILE_OBJECT FileObject,
					 IN PLARGE_INTEGER FileOffset,
					 IN PLARGE_INTEGER Length,
					 PEPROCESS ProcessId,
					 ULONG Key,
					 OUT PIO_STATUS_BLOCK IoStatus,
					 IN PDEVICE_OBJECT DeviceObject
					 );

BOOLEAN
SfFastIoUnlockAll(
				  IN PFILE_OBJECT FileObject,
				  PEPROCESS ProcessId,
				  OUT PIO_STATUS_BLOCK IoStatus,
				  IN PDEVICE_OBJECT DeviceObject
				  );

BOOLEAN
SfFastIoUnlockAllByKey(
					   IN PFILE_OBJECT FileObject,
					   PVOID ProcessId,
					   ULONG Key,
					   OUT PIO_STATUS_BLOCK IoStatus,
					   IN PDEVICE_OBJECT DeviceObject
					   );

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
					  );

VOID
SfFastIoDetachDevice(
					 IN PDEVICE_OBJECT SourceDevice,
					 IN PDEVICE_OBJECT TargetDevice
					 );

BOOLEAN
SfFastIoQueryNetworkOpenInfo(
							 IN PFILE_OBJECT FileObject,
							 IN BOOLEAN Wait,
							 OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
							 OUT PIO_STATUS_BLOCK IoStatus,
							 IN PDEVICE_OBJECT DeviceObject
							 );

BOOLEAN
SfFastIoMdlRead(
				IN PFILE_OBJECT FileObject,
				IN PLARGE_INTEGER FileOffset,
				IN ULONG Length,
				IN ULONG LockKey,
				OUT PMDL *MdlChain,
				OUT PIO_STATUS_BLOCK IoStatus,
				IN PDEVICE_OBJECT DeviceObject
				);


BOOLEAN
SfFastIoMdlReadComplete(
						IN PFILE_OBJECT FileObject,
						IN PMDL MdlChain,
						IN PDEVICE_OBJECT DeviceObject
						);

BOOLEAN
SfFastIoPrepareMdlWrite(
						IN PFILE_OBJECT FileObject,
						IN PLARGE_INTEGER FileOffset,
						IN ULONG Length,
						IN ULONG LockKey,
						OUT PMDL *MdlChain,
						OUT PIO_STATUS_BLOCK IoStatus,
						IN PDEVICE_OBJECT DeviceObject
						);

BOOLEAN
SfFastIoMdlWriteComplete(
						 IN PFILE_OBJECT FileObject,
						 IN PLARGE_INTEGER FileOffset,
						 IN PMDL MdlChain,
						 IN PDEVICE_OBJECT DeviceObject
						 );

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
					   );

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
						);

BOOLEAN
SfFastIoMdlReadCompleteCompressed(
								  IN PFILE_OBJECT FileObject,
								  IN PMDL MdlChain,
								  IN PDEVICE_OBJECT DeviceObject
								  );

BOOLEAN
SfFastIoMdlWriteCompleteCompressed(
								   IN PFILE_OBJECT FileObject,
								   IN PLARGE_INTEGER FileOffset,
								   IN PMDL MdlChain,
								   IN PDEVICE_OBJECT DeviceObject
								   );

BOOLEAN
SfFastIoQueryOpen(
				  IN PIRP Irp,
				  OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
				  IN PDEVICE_OBJECT DeviceObject
				  );

NTSTATUS
SfPreFsFilterPassThrough (
						  IN PFS_FILTER_CALLBACK_DATA Data,
						  OUT PVOID *CompletionContext
						  );

VOID
SfPostFsFilterPassThrough (
						   IN PFS_FILTER_CALLBACK_DATA Data,
						   IN NTSTATUS OperationStatus,
						   IN PVOID CompletionContext
						   );

