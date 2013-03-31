#include <ntifs.h>
#include "BaseDef.h"
#include "BaseFun.h"
#include "MySFilter.h"

// 本驱动对象全局指针
extern PDRIVER_OBJECT gSFilterDriverObject;

// CDO的全局指针
extern PDEVICE_OBJECT gSFilterControlDeviceObject;

// 互斥体
extern FAST_MUTEX gSfilterAttachLock;

// 目录存储指针
PDIR_NODE gListHead;

/***********************************************************************
* 函数名称:SfCleanupMountedDevice 
* 函数描述:设备对象卸载时清理
* 参数列表:
*		DeviceObject:将要卸载的设备对象
* 返回值:空
***********************************************************************/
VOID
SfCleanupMountedDevice(
					   IN PDEVICE_OBJECT DeviceObject
					   )
{
	// 不作任何清理
	UNREFERENCED_PARAMETER(DeviceObject);
	return;
}

/***********************************************************************
* 函数名称:SfAttachDeviceToDeviceStack 
* 函数描述:绑定设备到设备栈
* 参数列表:
*		SourceDevice:本设备
*		TargetDevice:需要被绑定的设备
*		AttachedToDeviceObject:被绑定的设备
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfAttachDeviceToDeviceStack (
							 IN PDEVICE_OBJECT SourceDevice,
							 IN PDEVICE_OBJECT TargetDevice,
							 IN OUT PDEVICE_OBJECT *AttachedToDeviceObject
							 )
{
	// 因为是XP版本，直接调用IoAttachDeviceToDeviceStackSafe
	return IoAttachDeviceToDeviceStackSafe(SourceDevice,TargetDevice,AttachedToDeviceObject);
}

/***********************************************************************
* 函数名称:SfGetObjectName 
* 函数描述:获得设备名称
* 参数列表:
*		Object:设备对象
*		Name:名称
* 返回值:空
***********************************************************************/
VOID
SfGetObjectName(
				IN PVOID Object,
				IN OUT PUNICODE_STRING Name
				)
{
	/*
	NTSTATUS status;
	*
	*	错误：	绝对不能这样为nameInfo分配空间！！！
	*			ObQueryNameString中的nameInfo需要一段连续的空间来存放读取的名称，而这样分配的buffer不是连续的
	*
	UNICODE_STRING nameStr;
	WCHAR nameBuf[MY_DEV_MAX_NAME];
	RtlInitEmptyUnicodeString(&nameStr,nameBuf,sizeof(nameBuf));

	POBJECT_NAME_INFORMATION nameInfo;
	nameInfo = (POBJECT_NAME_INFORMATION)&nameStr;
	ULONG len;

	// 获得名称
	status = ObQueryNameString(Object,nameInfo,sizeof(nameBuf),&len);

	Name->Length = 0;
	if (NT_SUCCESS( status ))
	{
	RtlCopyUnicodeString( Name, &nameInfo->Name );
	}
	*/


	NTSTATUS status;
	CHAR nibuf[512];        //分配一块连续的内存即可
	POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
	ULONG retLength;

	status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength);

	Name->Length = 0;
	if (NT_SUCCESS( status )) 
	{
		RtlCopyUnicodeString( Name, &nameInfo->Name );
	}
}


/***********************************************************************
* 函数名称:SfAttachToFileSystemDevice 
* 函数描述:绑定设备到文件系统
* 参数列表:
*		DeviceObject:文件系统设备对象
*		DeviceName:文件系统设备名称
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfAttachToFileSystemDevice(
						   IN PDEVICE_OBJECT DeviceObject,
						   IN PUNICODE_STRING DeviceName
						   )
{
	NTSTATUS status = STATUS_SUCCESS;

	if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType))
	{
		// 不需要过滤的文件系统
		return STATUS_SUCCESS;
	}

	// 文件识别器不进行绑定
	UNICODE_STRING fsNameStr;
	WCHAR fsNameBuf[MY_DEV_MAX_NAME];
	RtlInitEmptyUnicodeString(&fsNameStr,fsNameBuf,sizeof(fsNameBuf));
	SfGetObjectName(DeviceObject->DriverObject,&fsNameStr);

	UNICODE_STRING fsrecNameStr;
	RtlInitUnicodeString(&fsrecNameStr,L"\\FileSystem\\Fs_Rec");
	
	if (RtlCompareUnicodeString(&fsNameStr,&fsrecNameStr,TRUE) == 0)
	{
		// 是识别器，不进行绑定
		return STATUS_SUCCESS;
	}

	// 生成新设备
	PDEVICE_OBJECT newDeviceObject;
	status = IoCreateDevice(gSFilterDriverObject,
		sizeof(SFILTER_DEVICE_EXTENSION),
		NULL,
		DeviceObject->DeviceType,
		0,
		FALSE,
		&newDeviceObject
		);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("SfAttachToFileSystemDevice:IoCreateDevice Failed\n"));
		return status;
	}

	// 设置标志位
	if (FlagOn(DeviceObject->Flags,DO_BUFFERED_IO))
	{
		SetFlag(newDeviceObject->Flags,DO_BUFFERED_IO);
	}
	if (FlagOn(DeviceObject->Flags,DO_DIRECT_IO))
	{
		SetFlag(newDeviceObject->Flags,DO_DIRECT_IO);
	}
	if (FlagOn(DeviceObject->Flags,FILE_DEVICE_SECURE_OPEN))
	{
		SetFlag(newDeviceObject->Flags,FILE_DEVICE_SECURE_OPEN);
	}

	PSFILTER_DEVICE_EXTENSION devExt;
	devExt = (PSFILTER_DEVICE_EXTENSION)newDeviceObject->DeviceExtension;

	// 绑定设备
	status = SfAttachDeviceToDeviceStack(newDeviceObject,
		DeviceObject,
		&devExt->AttachedToDeviceObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("SfAttachToFileSystemDevice:SfAttachDeviceToDeviceStack Failed\n"));
		SfCleanupMountedDevice(newDeviceObject);
		IoDeleteDevice(newDeviceObject);
		return status;
	}

	// 设置拓展信息
	devExt->TypeFlag = POOL_TAG;
	devExt->DevType = DEV_FILESYSTEM;
	devExt->DevObj = DeviceObject;
	RtlInitEmptyUnicodeString(&devExt->DeviceName,
		devExt->DeviceNameBuffer,
		sizeof(devExt->DeviceNameBuffer));
	RtlCopyUnicodeString(&devExt->DeviceName,DeviceName);
	
	// 设置初始化完成
	ClearFlag(newDeviceObject->Flags,DO_DEVICE_INITIALIZING);

	LOG_PRINT("Attached to File System %wZ ,File System Dev = %p ,MyDev = %p\n",
		&devExt->DeviceName,
		DeviceObject,
		newDeviceObject);
	
	// 枚举这个文件系统上的所有卷
	status = SfEnumerateFileSystemVolumes(DeviceObject,DeviceName);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("SfAttachToFileSystemDevice:SfEnumerateFileSystemVolumes Failed\n"));
		IoDetachDevice(devExt->AttachedToDeviceObject);
		SfCleanupMountedDevice(newDeviceObject);
		IoDeleteDevice(newDeviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}

/***********************************************************************
* 函数名称:SfEnumerateFileSystemVolumes 
* 函数描述:枚举这个文件系统上的所有卷
* 参数列表:
*		FSDeviceObject:文件系统设备对象
*		FSName:文件系统设备名称
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfEnumerateFileSystemVolumes(
							 IN PDEVICE_OBJECT FSDeviceObject,
							 IN PUNICODE_STRING FSName
							 )
{
	NTSTATUS status = STATUS_SUCCESS;
	// 获得上面的设备对象数目
	ULONG numDevices;
	PDEVICE_OBJECT *devList;
	status = IoEnumerateDeviceObjectList(FSDeviceObject->DriverObject,
		NULL,
		0,
		&numDevices);

	if (!NT_SUCCESS(status))
	{
		// 失败的原因应该是无buf
		ASSERT(status == STATUS_BUFFER_TOO_SMALL);

		// 分配内存
		devList = (PDEVICE_OBJECT *)ExAllocatePoolWithTag(NonPagedPool,
			numDevices * sizeof(DEVICE_OBJECT),
			POOL_TAG);

		if (devList == NULL)
		{
			// 内存不足
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		// 获得设备对象
		status = IoEnumerateDeviceObjectList(FSDeviceObject->DriverObject,
			devList,
			numDevices * sizeof(DEVICE_OBJECT),
			&numDevices);

		// 用来保存真实存储对象
		PDEVICE_OBJECT storageStackDeviceObject;

		ULONG i;
		for (i = 0;i < numDevices;i++)
		{
			// 清空真实存储对象
			storageStackDeviceObject = NULL;
			
			if ((devList[i] == FSDeviceObject) ||	// 设备对象是自己的时候，这个设备对象是个控制设备对象
				(devList[i]->DeviceType != FSDeviceObject->DeviceType) ||	// 类型和文件系统的类型不符
				SfIsAttachedToDevice( devList[i], NULL )	// 已经绑定过的设备对象
				)
			{
				// 不进行绑定
				continue;
			}
			
			// 获得最底层设备名字
			UNICODE_STRING name;
			WCHAR nameBuf[MY_DEV_MAX_NAME];
			RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));
			SfGetBaseDeviceObjectName(devList[i],&name);
			
			//如果有名字，就是个控制设备，不进行绑定
			if (name.Length > 0)
			{
				continue;
			}

			// 获得储存设备对象
			status = IoGetDiskDeviceObject(devList[i],&storageStackDeviceObject);

			if (!NT_SUCCESS(status))
			{
				continue;
			}

			// 卷影设备不做考虑。。。

			// 产生新设备对象
			PDEVICE_OBJECT newDeviceObject;
			status = IoCreateDevice(gSFilterDriverObject,
				sizeof(SFILTER_DEVICE_EXTENSION),
				NULL,
				devList[i]->DeviceType,
				0,
				FALSE,
				&newDeviceObject);
			if (!NT_SUCCESS(status))
			{
				continue;
			}

			// 设置设备拓展信息
			PSFILTER_DEVICE_EXTENSION newDevExt;
			newDevExt = (PSFILTER_DEVICE_EXTENSION)newDeviceObject->DeviceExtension;
			newDevExt->TypeFlag = POOL_TAG;
			newDevExt->DevType = DEV_VOLUME;
			newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
			newDevExt->DevObj = devList[i];
			// 获得符号链接名
			RtlInitEmptyUnicodeString(&newDevExt->SybDeviceName,
				newDevExt->SybDeviceNameBuffer,
				sizeof(newDevExt->SybDeviceNameBuffer));
			status = RtlVolumeDeviceToDosName(storageStackDeviceObject,&newDevExt->SybDeviceName);

			// 获得设备名
			RtlInitEmptyUnicodeString(&newDevExt->DeviceName,
				newDevExt->DeviceNameBuffer,
				sizeof(newDevExt->DeviceNameBuffer));
			SfGetObjectName(storageStackDeviceObject,
				&newDevExt->DeviceName);

			// 此时需要互斥体去控制访问
			// 获得互斥体
			ExAcquireFastMutex(&gSfilterAttachLock);

			// 到这里的设备都应该没有被加载过
			ASSERT(!SfIsAttachedToDevice(devList[i],NULL));

			// 绑定到卷上
			SfAttachToMountedDevice(devList[i],newDeviceObject);

			if (!NT_SUCCESS(status))
			{
				// 失败的原因只有可能是这个是被对象刚刚被加载，还未完成出事化
				// 此时它的Flags为DO_DEVICE_INITIALIZING
				// 可以等待一段时间之后再进行绑定
				SfCleanupMountedDevice(newDeviceObject);
				IoDeleteDevice(newDeviceObject);
			}

			// 释放互斥体
			ExReleaseFastMutex(&gSfilterAttachLock);

			// 引用数减1
			if (storageStackDeviceObject != NULL)
			{
				ObDereferenceObject(storageStackDeviceObject);
			}
			ObDereferenceObject( devList[i] );
		}

		// 对错误不作处理，全部返回STATUS_SUCCESS
		status = STATUS_SUCCESS;

		ExFreePoolWithTag(devList,POOL_TAG);
	}
	return status;
}

/***********************************************************************
* 函数名称:SfIsAttachedToDevice 
* 函数描述:查看这个设备对象的挂载对象
* 参数列表:
*		FSDeviceObject:文件系统设备对象
*		FSName:文件系统设备名称
* 返回值:是否成功
***********************************************************************/
BOOLEAN
SfIsAttachedToDevice(
					 PDEVICE_OBJECT DeviceObject,
					 PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
					 )
{
	NTSTATUS status;
	PDEVICE_OBJECT curDeivceObject;
	// 获得此设备的最顶层设备
	curDeivceObject = IoGetAttachedDeviceReference(DeviceObject);

	do 
	{
		if (IS_MY_DEVICE_OBJECT(curDeivceObject))
		{
			// 已经被挂载
			if (ARGUMENT_PRESENT(AttachedDeviceObject))
			{
				// 传进来的不是NULL
				*AttachedDeviceObject = curDeivceObject;
			}
			else
			{
				// 传进来的是NULL
				ObDereferenceObject( curDeivceObject );
			}
			return TRUE;
		}

		// 得到下层的设备
		PDEVICE_OBJECT nextDeviceObject;
		nextDeviceObject = IoGetLowerDeviceObject(curDeivceObject);
		ObDereferenceObject(curDeivceObject);

		curDeivceObject = nextDeviceObject;
	} while (curDeivceObject != NULL);

	// 到这里说明没有被挂载
	if (ARGUMENT_PRESENT(AttachedDeviceObject))
	{
		// 设置未找到设备，及未被挂载
		*AttachedDeviceObject = NULL;
	}

	return FALSE;
}

/***********************************************************************
* 函数名称:SfGetBaseDeviceObjectName 
* 函数描述:获得最底层设备名字
* 参数列表:
*		DeviceObject:设备对象
*		Name:名称
* 返回值:空
***********************************************************************/
VOID
SfGetBaseDeviceObjectName (
						   IN PDEVICE_OBJECT DeviceObject,
						   IN OUT PUNICODE_STRING Name
						   )
{
	// 获得最底层设备对象
	PDEVICE_OBJECT lowestDeviceObject;
	lowestDeviceObject = IoGetDeviceAttachmentBaseRef(DeviceObject);

	// 获得它的名字
	SfGetObjectName(lowestDeviceObject,Name);

	// 计数减1
	ObDereferenceObject(lowestDeviceObject);
}

/***********************************************************************
* 函数名称:SfDetachFromFileSystemDevice 
* 函数描述:卸载文件系统的绑定
* 参数列表:
*		DeviceObject:设备对象
* 返回值:空
***********************************************************************/
VOID
SfDetachFromFileSystemDevice (
							  IN PDEVICE_OBJECT DeviceObject
							  )
{
	PDEVICE_OBJECT attachedDevice;
	PSFILTER_DEVICE_EXTENSION devExt;

	attachedDevice = DeviceObject->AttachedDevice;

	while(attachedDevice != NULL)
	{
		if (IS_MY_DEVICE_OBJECT(attachedDevice))
		{
			// 是我们生成的设备对象
			devExt = (PSFILTER_DEVICE_EXTENSION)attachedDevice->DeviceExtension;
			LOG_PRINT("Detach From File System %wZ\n",&devExt->DeviceName);

			SfCleanupMountedDevice(attachedDevice);
			IoDetachDevice(DeviceObject);
			IoDeleteDevice(attachedDevice);
			return;
		}
		// DeviceObject一直保持为被挂载的设备
		DeviceObject = attachedDevice;
		attachedDevice = attachedDevice->AttachedDevice;
	}
}

/***********************************************************************
* 函数名称:SfAttachToMountedDevice 
* 函数描述:绑定设备到卷
* 参数列表:
*		DeviceObject:卷设备对象
*		SFilterDeviceObject:过滤设备对象
* 返回值:空
***********************************************************************/
NTSTATUS
SfAttachToMountedDevice (
						 IN PDEVICE_OBJECT DeviceObject,
						 IN PDEVICE_OBJECT SFilterDeviceObject
						 )
{
	NTSTATUS status;
	PSFILTER_DEVICE_EXTENSION newDevExt;
	newDevExt = (PSFILTER_DEVICE_EXTENSION)SFilterDeviceObject->DeviceExtension;

	// 应该是未被绑定的设备
	ASSERT(!SfIsAttachedToDevice ( DeviceObject, NULL ));

	// 查看是否需要绑定
	if (!OnSfilterAttachPre(SFilterDeviceObject,DeviceObject,NULL))
	{
		// 不需要绑定
		return STATUS_UNSUCCESSFUL;
	}

	if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) 
	{
		SetFlag( SFilterDeviceObject->Flags, DO_BUFFERED_IO );
	}

	if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) 
	{
		SetFlag( SFilterDeviceObject->Flags, DO_DIRECT_IO );
	}

	// 为了防止绑定不成功，卷刚被加载，绑定8次，间隔一段时间
	ULONG i;
	for (i = 0;i < 8;i++)
	{
		LARGE_INTEGER interval;
		status = SfAttachDeviceToDeviceStack(SFilterDeviceObject,
			DeviceObject,
			&newDevExt->AttachedToDeviceObject);

		if (NT_SUCCESS(status))
		{
			// 绑定成功，调用回调函数
			OnSfilterAttachPost(SFilterDeviceObject,
				DeviceObject,
				newDevExt->AttachedToDeviceObject,
				status);

			// 设置为初始化完成
			ClearFlag(SFilterDeviceObject->Flags,DO_DEVICE_INITIALIZING);

			LOG_PRINT("Attached To Volume %wZ(%wZ) ,MountDev = %p ,MyDev = %p\n",
				&newDevExt->DeviceName,
				&newDevExt->SybDeviceName,
				DeviceObject,
				SFilterDeviceObject);

			return STATUS_SUCCESS;
		}

		// 延时0.5秒再试
		interval.QuadPart = (500 * DELAY_ONE_MILLISECOND);
		KeDelayExecutionThread( KernelMode, FALSE, &interval );
	}
	
	// 失败调用回调函数
	OnSfilterAttachPost(SFilterDeviceObject,
		DeviceObject,
		newDevExt->AttachedToDeviceObject,
		status);

	return status;
}

/***********************************************************************
* 函数名称:SfFsControlMountVolume 
* 函数描述:发生加载卷IRP的调用函数
* 参数列表:
*		DeviceObject:本设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfFsControlMountVolume (
						IN PDEVICE_OBJECT DeviceObject,
						IN PIRP Irp
						)
{
	NTSTATUS status;

	// 获得当前栈地址
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
	ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));

	// 保存真实设备地址，以后再用
	PDEVICE_OBJECT storageStackDeviceObject;
	storageStackDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;

	// 生成新的设备
	PDEVICE_OBJECT newDeviceObject;
	status = IoCreateDevice(gSFilterDriverObject,
		sizeof(SFILTER_DEVICE_EXTENSION),
		NULL,
		DeviceObject->DeviceType,
		0,
		FALSE,
		&newDeviceObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("SfFsControlMountVolume IoCreateDevice Failed"));
		// 设置失败信息，完成IRP
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
		return status;
	}

	// 设置设备扩展信息
	PSFILTER_DEVICE_EXTENSION newDevExt = (PSFILTER_DEVICE_EXTENSION)newDeviceObject->DeviceExtension;
	newDevExt->StorageStackDeviceObject = storageStackDeviceObject;
	newDevExt->TypeFlag = POOL_TAG;
	RtlInitEmptyUnicodeString(&newDevExt->DeviceName,
		newDevExt->DeviceNameBuffer,
		sizeof(newDevExt->DeviceNameBuffer));
	SfGetObjectName(storageStackDeviceObject,
		&newDevExt->DeviceName);

	// 设置IRP完成例程，并且等待其完成
	KEVENT waitEvent;
	KeInitializeEvent(&waitEvent,
		NotificationEvent,
		FALSE);

	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp,
		SfFsControlCompletion,	// 完成例程
		&waitEvent,		// 传进去的参数
		TRUE,
		TRUE,
		TRUE);

	status = IoCallDriver(devExt->AttachedToDeviceObject,
		Irp);

	// 等待完成
	if (status = STATUS_PENDING)
	{
		status = KeWaitForSingleObject(&waitEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}

	// 卷加载完成，调用回调函数
	status = SfFsControlMountVolumeComplete(DeviceObject,
		Irp,
		newDeviceObject);

	return status;
}

/***********************************************************************
* 函数名称:SfFsControlCompletion 
* 函数描述:IRP完成的回调函数
* 参数列表:
*		DeviceObject:本设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfFsControlCompletion(
					  IN PDEVICE_OBJECT DeviceObject,
					  IN PIRP Irp,
					  IN PVOID Context
					  )
{
	// 设置时间完成，回到调用IRP中处理
	KeSetEvent((PKEVENT)Context,
		IO_NO_INCREMENT,
		FALSE);
	// 设置为需要更多的处理
	return STATUS_MORE_PROCESSING_REQUIRED;
}

/***********************************************************************
* 函数名称:SfFsControlMountVolumeComplete 
* 函数描述:挂载卷完成后调用
* 参数列表:
*		DeviceObject:原设备对象
*		Irp:IRP
*		NewDeviceObject:过滤设备对象
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfFsControlMountVolumeComplete (
								IN PDEVICE_OBJECT DeviceObject,
								IN PIRP Irp,
								IN PDEVICE_OBJECT NewDeviceObject
								)
{
	NTSTATUS status;
	PSFILTER_DEVICE_EXTENSION newDevExt = (PSFILTER_DEVICE_EXTENSION)NewDeviceObject->DeviceExtension;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PVPB vpb = newDevExt->StorageStackDeviceObject->Vpb;

	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		// IRP是成功的
		// 为了防止绑定两次，使用互斥体
		ExAcquireFastMutex(&gSfilterAttachLock);

		PDEVICE_OBJECT attachedDeviceObject;
		if (!SfIsAttachedToDevice(vpb->DeviceObject,&attachedDeviceObject))
		{
			// 未绑定过，绑定到卷
			status = SfAttachToMountedDevice(vpb->DeviceObject,NewDeviceObject);

			if (!NT_SUCCESS(status))
			{
				// 绑定失败
				SfCleanupMountedDevice(NewDeviceObject);
				IoDeleteDevice(NewDeviceObject);
			}
		}
		else
		{
			// 已经绑定过了
			SfCleanupMountedDevice(NewDeviceObject);
			IoDeleteDevice(NewDeviceObject);
			ObDereferenceObject(attachedDeviceObject);
		}
		ExReleaseFastMutex(&gSfilterAttachLock);
	}
	else
	{
		// IRP失败
		SfCleanupMountedDevice(NewDeviceObject);
		IoDeleteDevice(NewDeviceObject);
	}

	status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/***********************************************************************
* 函数名称:SfFsControlLoadFileSystem 
* 函数描述:发生加载真实文件系统IRP时调用
* 参数列表:
*		DeviceObject:本设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfFsControlLoadFileSystem (
						   IN PDEVICE_OBJECT DeviceObject,
						   IN PIRP Irp
						   )
{
	NTSTATUS status;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	LOG_PRINT("Loading File System, Detaching from %wZ \n",&devExt->DeviceName);

	// 创建事件，等待其完成
	KEVENT waitEvent;

	KeInitializeEvent( &waitEvent, 
		NotificationEvent, 
		FALSE );

	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp,
		SfFsControlCompletion,
		&waitEvent,
		TRUE,
		TRUE,
		TRUE );

	status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

	if (STATUS_PENDING == status) 
	{
		status = KeWaitForSingleObject( &waitEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL );
	}

	status = SfFsControlLoadFileSystemComplete( DeviceObject,
		Irp );
	return status;
}

/***********************************************************************
* 函数名称:SfFsControlLoadFileSystemComplete 
* 函数描述:发生加载真实文件系统IRP完成后
* 参数列表:
*		DeviceObject:本设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfFsControlLoadFileSystemComplete (
								   IN PDEVICE_OBJECT DeviceObject,
								   IN PIRP Irp
								   )
{
	NTSTATUS status;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	LOG_PRINT("Detaching from recognizer %wZ \n",&devExt->DeviceName);

	if (!NT_SUCCESS( Irp->IoStatus.Status ) && (Irp->IoStatus.Status != STATUS_IMAGE_ALREADY_LOADED))
	{
		// 加载失败，我们再次挂载到文件识别器上
		SfAttachDeviceToDeviceStack( DeviceObject, 
			devExt->AttachedToDeviceObject,
			&devExt->AttachedToDeviceObject );
	}
	else
	{
		// 加载成功
		SfCleanupMountedDevice(DeviceObject);
		IoDeleteDevice(DeviceObject);
	}

	status = Irp->IoStatus.Status;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return status;
}

/***********************************************************************
* 函数名称:SfAddDirToTable 
* 函数描述:记录打开的目录
* 参数列表:
*		file:文件对象
* 返回值:状态值
***********************************************************************/
VOID
SfAddDirToTable(PFILE_OBJECT file)
{
	
	if (gListHead == NULL)
	{
		// 链表为空
		gListHead = (PDIR_NODE)ExAllocatePoolWithTag(PagedPool,
			sizeof(DIR_NODE),
			POOL_TAG);
		RtlInitEmptyUnicodeString(&gListHead->DirPathStr,
			gListHead->DirPathBuf,
			sizeof(gListHead->DirPathBuf));

		RtlCopyUnicodeString(&gListHead->DirPathStr,&file->FileName);

		gListHead->referenceNum = 1;

		gListHead->nextNode = NULL;
		return;
	}

	PDIR_NODE nextDir = gListHead;
	while(nextDir->nextNode != NULL)
	{
		if (RtlCompareUnicodeString(&nextDir->DirPathStr,&file->FileName,TRUE) == 0)
		{
			// 链表中存在此目录
			nextDir->referenceNum++;
			return;
		}
		nextDir = nextDir->nextNode;
	}

	if (RtlCompareUnicodeString(&nextDir->DirPathStr,&file->FileName,TRUE) == 0)
	{
		// 链表中存在此目录
		nextDir->referenceNum++;
		return;
	}

	// 不存在此目录，添加新节点
	nextDir->nextNode = (PDIR_NODE)ExAllocatePoolWithTag(PagedPool,
		sizeof(DIR_NODE),
		POOL_TAG);
	nextDir = nextDir->nextNode;
	RtlInitEmptyUnicodeString(&nextDir->DirPathStr,
		nextDir->DirPathBuf,
		sizeof(nextDir->DirPathBuf));

	RtlCopyUnicodeString(&nextDir->DirPathStr,&file->FileName);

	nextDir->referenceNum = 1;

	nextDir->nextNode = NULL;

	return;
}

/***********************************************************************
* 函数名称:SfDelDirFromTable 
* 函数描述:删除打开的目录记录
* 参数列表:
*		file:文件对象
* 返回值:状态值
***********************************************************************/
VOID 
SfDelDirFromTable(PFILE_OBJECT file)
{
	if (gListHead == NULL)
	{
		// 链表为空
		return;
	}

	PDIR_NODE nextDir = gListHead;
	PDIR_NODE preDir = NULL;
	while(nextDir != NULL)
	{
		if (RtlCompareUnicodeString(&nextDir->DirPathStr,&file->FileName,TRUE) == 0)
		{
			// 找到目录
			if (preDir == NULL)
			{
				// 链表的第一个元素
				nextDir->referenceNum--;
				if (nextDir->referenceNum == 0)
				{
					// 无引用了，可以删除节点
					gListHead = gListHead->nextNode;
					ExFreePoolWithTag(nextDir,POOL_TAG);
				}
			}
			else
			{
				nextDir->referenceNum--;
				if (nextDir->referenceNum == 0)
				{
					// 无引用了，可以删除节点
					preDir->nextNode = nextDir->nextNode;
					ExFreePoolWithTag(nextDir,POOL_TAG);
				}
			}
			return;
		}
		// 不是这个目录
		preDir = nextDir;
		nextDir = nextDir->nextNode;
	}
	return;
}

/***********************************************************************
* 函数名称:SfGetFileName 
* 函数描述:获得完整的文件名
* 参数列表:
*		FileObject:打开的文件对象
*		DeviceObject:使用的设备对象
*		FileName:返回的文件名
* 返回值:状态值
* FileName在传递进来之前应该已经分配好内存
***********************************************************************/
VOID
SfGetFileName(
			  IN PFILE_OBJECT FileObject,
			  IN PDEVICE_OBJECT DeviceObject,
			  OUT PUNICODE_STRING FileName
			  )
{
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlCopyUnicodeString(FileName,&devExt->DeviceName);
	RtlAppendUnicodeStringToString(FileName,&FileObject->FileName);
}

/***********************************************************************
* 函数名称:SfGetFileLinkName 
* 函数描述:获得完整的符号链接文件名
* 参数列表:
*		FileObject:打开的文件对象
*		DeviceObject:使用的设备对象
*		FileName:返回的文件名
* 返回值:状态值
* FileName在传递进来之前应该已经分配好内存
***********************************************************************/
VOID
SfGetFileLinkName(
				  IN PFILE_OBJECT FileObject,
				  IN PDEVICE_OBJECT DeviceObject,
				  OUT PUNICODE_STRING FileName
				  )
{

	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RtlCopyUnicodeString(FileName,&devExt->SybDeviceName);
	RtlAppendUnicodeStringToString(FileName,&FileObject->FileName);
}