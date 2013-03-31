VOID
SfCleanupMountedDevice(
					   IN PDEVICE_OBJECT DeviceObject
					   );

NTSTATUS
SfAttachDeviceToDeviceStack (
							 IN PDEVICE_OBJECT SourceDevice,
							 IN PDEVICE_OBJECT TargetDevice,
							 IN OUT PDEVICE_OBJECT *AttachedToDeviceObject
							 );

NTSTATUS
SfAttachToFileSystemDevice(
						   IN PDEVICE_OBJECT DeviceObject,
						   IN PUNICODE_STRING DeviceName
						   );

VOID
SfDetachFromFileSystemDevice (
							  IN PDEVICE_OBJECT DeviceObject
							  );

NTSTATUS
SfAttachToMountedDevice (
						 IN PDEVICE_OBJECT DeviceObject,
						 IN PDEVICE_OBJECT SFilterDeviceObject
						 );

VOID
SfCleanupMountedDevice(
					   IN PDEVICE_OBJECT DeviceObject
					   );


NTSTATUS
SfEnumerateFileSystemVolumes(
							 IN PDEVICE_OBJECT FSDeviceObject,
							 IN PUNICODE_STRING FSName
							 );

VOID
SfGetObjectName(
				IN PVOID Object,
				IN OUT PUNICODE_STRING Name
				);

VOID
SfGetBaseDeviceObjectName(
						  IN PDEVICE_OBJECT DeviceObject,
						  IN OUT PUNICODE_STRING DeviceName
						  );

BOOLEAN
SfIsAttachedToDevice(
					 PDEVICE_OBJECT DeviceObject,
					 PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
					 );

VOID
SfReadDriverParameters(
					   IN PUNICODE_STRING RegistryPath
					   );

NTSTATUS
SfIsShadowCopyVolume (
					  IN PDEVICE_OBJECT StorageStackDeviceObject,
					  OUT PBOOLEAN IsShadowCopy
					  );


NTSTATUS
SfFsControlMountVolume (
						IN PDEVICE_OBJECT DeviceObject,
						IN PIRP Irp
						);

NTSTATUS
SfFsControlMountVolumeComplete (
								IN PDEVICE_OBJECT DeviceObject,
								IN PIRP Irp,
								IN PDEVICE_OBJECT NewDeviceObject
								);

NTSTATUS
SfFsControlCompletion(
					  IN PDEVICE_OBJECT DeviceObject,
					  IN PIRP Irp,
					  IN PVOID Context
					  );

NTSTATUS
SfFsControlLoadFileSystem (
						   IN PDEVICE_OBJECT DeviceObject,
						   IN PIRP Irp
						   );

NTSTATUS
SfFsControlLoadFileSystemComplete (
								   IN PDEVICE_OBJECT DeviceObject,
								   IN PIRP Irp
								   );

VOID 
SfAddDirToTable(
				PFILE_OBJECT file
				);

VOID
SfDelDirFromTable(
				  PFILE_OBJECT file
				  );

VOID
SfGetFileName(
			  IN PFILE_OBJECT FileObject,
			  IN PDEVICE_OBJECT DeviceObject,
			  OUT PUNICODE_STRING FileName
			  );

VOID
SfGetFileLinkName(
				  IN PFILE_OBJECT FileObject,
				  IN PDEVICE_OBJECT DeviceObject,
				  OUT PUNICODE_STRING FileName
				  );