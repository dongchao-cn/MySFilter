// IRP的调用例程
NTSTATUS
SfPassThrough(
			  IN PDEVICE_OBJECT DeviceObject,
			  IN PIRP Irp
			  );

NTSTATUS
SfCreate(
		 IN PDEVICE_OBJECT DeviceObject,
		 IN PIRP Irp
		 );

NTSTATUS
SfCreateCompletion(
				   IN PDEVICE_OBJECT DeviceObject,
				   IN PIRP Irp,
				   IN PVOID Context
				   );

NTSTATUS
SfCleanupClose(
			   IN PDEVICE_OBJECT DeviceObject,
			   IN PIRP Irp
			   );

NTSTATUS
SfFsControl(
			IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp
			);

VOID
SfFsNotification (
				  IN PDEVICE_OBJECT DeviceObject,
				  IN BOOLEAN FsActive
				  );

NTSTATUS
SfRead(
	   IN PDEVICE_OBJECT DeviceObject,
	   IN PIRP Irp
	   );

NTSTATUS
SfReadCompletion(
				   IN PDEVICE_OBJECT DeviceObject,
				   IN PIRP Irp,
				   IN PVOID Context
				   );

NTSTATUS
SfWrite(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
		);

NTSTATUS
SfWriteCompletion(
				 IN PDEVICE_OBJECT DeviceObject,
				 IN PIRP Irp,
				 IN PVOID Context
				 );

NTSTATUS
SfSetInfo(
		  IN PDEVICE_OBJECT DeviceObject,
		  IN PIRP Irp
		  );
