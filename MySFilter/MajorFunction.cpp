#include <ntifs.h>
#include "BaseDef.h"
#include "BaseFun.h"
#include "MajorFunction.h"
#include "MySFilter.h"
// 本驱动对象全局指针
extern PDRIVER_OBJECT gSFilterDriverObject;

// CDO的全局指针
extern PDEVICE_OBJECT gSFilterControlDeviceObject;

/***********************************************************************
* 函数名称:SfPassThrough 
* 函数描述:无需处理的IRP，直接下发
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS 
SfPassThrough(
			  IN PDEVICE_OBJECT DeviceObject,
			  IN PIRP Irp
			  )
{
	// CDO不应该接收任何IRP
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

	// 设备对象应该是本驱动对象创建的
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	// 简单的跳过当前层,向下发IRP
	IoSkipCurrentIrpStackLocation(Irp);

	return IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject,Irp);
}

/***********************************************************************
* 函数名称:SfFsNotification 
* 函数描述:当文件系统发生变化时被调用
* 参数列表:
*		DeviceObject:设备对象
*		FsActive:激活或者关闭
* 返回值:状态值
***********************************************************************/
VOID
SfFsNotification (
				  IN PDEVICE_OBJECT DeviceObject,
				  IN BOOLEAN FsActive
				  )
{

	PAGED_CODE();

	// name用来接收设备名字
	UNICODE_STRING name;
	WCHAR nameBuf[MY_DEV_MAX_NAME];

	PAGED_CODE();

	RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));

	SfGetObjectName(DeviceObject,&name);

	LOG_PRINT("SfFsNotification: %s %wZ \n",
		FsActive ? "Activating file system" : "Deactivating file system",
		&name
		);

	// 如果是激活，绑定文件系统
	// 如果是关闭，解除绑定
	if (FsActive)
	{
		SfAttachToFileSystemDevice(DeviceObject,&name);
	}
	else
	{
		SfDetachFromFileSystemDevice(DeviceObject);
	}
}

/***********************************************************************
* 函数名称:SfFsControl 
* 函数描述:当收到IRP_MJ_FILE_SYSTEM_CONTROL时调用，处理卷的挂载信息
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfFsControl(
			IN PDEVICE_OBJECT DeviceObject,
			IN PIRP Irp
			)
{
	// 获得当前IRP栈指针
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));

	switch (irpSp->MinorFunction)
	{
	case IRP_MN_MOUNT_VOLUME:
		// 卷被挂载
		return SfFsControlMountVolume(DeviceObject,Irp);
	case IRP_MN_LOAD_FILE_SYSTEM:
		// 加载真实文件系统
		return SfFsControlLoadFileSystem( DeviceObject, Irp );
	case IRP_MN_USER_FS_REQUEST:
		{
			switch (irpSp->Parameters.FileSystemControl.FsControlCode)
			{
			case FSCTL_DISMOUNT_VOLUME:
				// 卸载卷，不删除过滤设备，不作任何处理，没有影响
				PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
				LOG_PRINT("DisAttached From Volume %wZ \n",&devExt->DeviceName);
			}
		}
	}
	
	// 别的控制IRP，不作处理
	return SfPassThrough(DeviceObject,Irp);
}

/***********************************************************************
* 函数名称:SfCreate 
* 函数描述:当收到IRP_MJ_CREATE时调用，处理打开文件IRP
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfCreate(
		 IN PDEVICE_OBJECT DeviceObject,
		 IN PIRP Irp
		 )
{
	NTSTATUS status;
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT(DeviceObject));
	SF_RET ret;
	ret = OnSfilterIrpPre(DeviceObject,Irp);
	if (ret == SF_IRP_PASS)
	{
		// 传递下去，不进行过滤
		return SfPassThrough(DeviceObject,Irp);
	}
	else
	{
		// 传递下去，进行过滤
		// 拷贝当前IRP
		IoCopyCurrentIrpStackLocationToNext(Irp);

		// 设置完成后的回调函数
		IoSetCompletionRoutine(Irp,
			SfCreateCompletion,
			NULL,
			TRUE,
			TRUE,
			TRUE);

		status = IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject,
			Irp);
		OnSfilterIrpPost(DeviceObject,Irp);
		return status;
	}
}

/***********************************************************************
* 函数名称:SfCreateCompletion 
* 函数描述:IRP_MJ_CREATE完成时的回调函数
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfCreateCompletion(
				   IN PDEVICE_OBJECT DeviceObject,
				   IN PIRP Irp,
				   IN PVOID Context
				   )
{
//	_asm int 3
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file = irpSp->FileObject;

	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		ASSERT(file != NULL);
		UNICODE_STRING name;
		WCHAR nameBuf[1024];
		RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));

		// 获得完整的路径，使用设备对象名称
	/*	SfGetFileName( irpSp->FileObject, 
			DeviceObject,
			&name);
	*/
		// 获得完整的路径，使用符号链接名称
		SfGetFileLinkName( irpSp->FileObject, 
			DeviceObject,
			&name);
		// 经过测试如果输出的字符串中含有中文，UNICODE_STRING会被截断
		// ANSI_STRING没有影响。。。
		ANSI_STRING aName;
		RtlUnicodeStringToAnsiString(&aName,&name,TRUE);


		/*
		// 这个文件\目录判断不太准确，以后再研究
		if ((irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) != 0)
		{
			// 打开的是目录
			LOG_PRINT("Open Succeed Dir name = %Z\n",&aName);
			//	SfAddDirToTable(file);
		}
		else
		{
			// 打开的是文件
			LOG_PRINT("Open Succeed File name = %Z\n",&aName);
		}
		*/

		if (Irp->IoStatus.Status == FILE_CREATED)
		{
			LOG_PRINT("Create Succeed! name = %Z\n",&aName);
		}
		else
		{
			LOG_PRINT("Opened Succeed! name = %Z\n",&aName);
		}
		
		// 释放空间
		RtlFreeAnsiString(&aName);
	}
	return Irp->IoStatus.Status;
}

/***********************************************************************
* 函数名称:SfCleanupClose 
* 函数描述:IRP_MJ_CLOSE的调用函数
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfCleanupClose(
			   IN PDEVICE_OBJECT DeviceObject,
			   IN PIRP Irp
			   )
{
	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));
	ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

	NTSTATUS status;
	SF_RET ret;
	ret = OnSfilterIrpPre(DeviceObject,Irp);
	if (ret == SF_IRP_PASS)
	{
		// 传递下去，不进行过滤
		return SfPassThrough(DeviceObject,Irp);
	}
	else
	{
		// 传递下去，进行过滤
		
		PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
		PFILE_OBJECT file = irpSp->FileObject;
		UNICODE_STRING name;
		WCHAR nameBuf[1024];
		RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));

		// 获得完整的路径，使用设备对象名称
	/*	SfGetFileName( irpSp->FileObject, 
			DeviceObject,
			&name);
	*/
		// 获得完整的路径，使用符号链接名称
		SfGetFileLinkName( irpSp->FileObject, 
			DeviceObject,
			&name);
		// 经过测试如果输出的字符串中含有中文，UNICODE_STRING会被截断
		// ANSI_STRING没有影响。。。
		ANSI_STRING aName;
		RtlUnicodeStringToAnsiString(&aName,&name,TRUE);
		LOG_PRINT("Close Succeed! name = %Z\n",&aName);
		// 释放空间
		RtlFreeAnsiString(&aName);

		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(((PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->AttachedToDeviceObject,
			Irp);
		OnSfilterIrpPost(DeviceObject,Irp);
		return status;
	}
}

/***********************************************************************
* 函数名称:SfRead 
* 函数描述:IRP_MJ_READ的调用函数
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfRead(
	   IN PDEVICE_OBJECT DeviceObject,
	   IN PIRP Irp
	   )
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file = irpSp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

	// 对于文件系统不作处理
	if (devExt->DevType == DEV_FILESYSTEM)
	{
		return SfPassThrough(DeviceObject,Irp);
	}

	SF_RET ret;
	ret = OnSfilterIrpPre(DeviceObject,Irp);
	if (ret == SF_IRP_GO_ON)
	{
		// 进行过滤
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp,
			SfReadCompletion,
			NULL,
			TRUE,
			TRUE,
			TRUE);
		status = IoCallDriver(devExt->AttachedToDeviceObject,
			Irp);
		OnSfilterIrpPost(DeviceObject,Irp);
		return status;
	}
	else
	{
		// 不进行过滤
		return SfPassThrough(DeviceObject,Irp);
	}
}

/***********************************************************************
* 函数名称:SfReadCompletion 
* 函数描述:IRP_MJ_READ完成的回调函数
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
*		Context:附加指针
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfReadCompletion(
				 IN PDEVICE_OBJECT DeviceObject,
				 IN PIRP Irp,
				 IN PVOID Context
				 )
{
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		// 读取成功
		VOID *Buf;
		ULONG len;
		if (Irp->MdlAddress != NULL)
		{
			Buf = MmGetSystemAddressForMdl(Irp->MdlAddress);
		}
		else
		{
			Buf = Irp->UserBuffer;
		}
		ASSERT(Buf != NULL);
		len = Irp->IoStatus.Information;

		// 输出信息
		UNICODE_STRING name;
		WCHAR nameBuf[1024];
		RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));

		// 获得完整的路径，使用设备对象名称
	/*	SfGetFileName( irpSp->FileObject, 
			DeviceObject,
			&name);
	*/
		// 获得完整的路径，使用符号链接名称
		SfGetFileLinkName( irpSp->FileObject, 
			DeviceObject,
			&name);
		// 经过测试如果输出的字符串中含有中文，UNICODE_STRING会被截断
		// ANSI_STRING没有影响。。。
		ANSI_STRING aName;
		RtlUnicodeStringToAnsiString(&aName,&name,TRUE);

		// 获得偏移量
		LARGE_INTEGER offset;
		offset.QuadPart = irpSp->Parameters.Read.ByteOffset.QuadPart;
		LOG_PRINT("Read Succeed! Name = %Z,Offset = %x%x, Len = %d, Buf = 0x%p\n",
			&aName,
			offset.HighPart,
			offset.LowPart,
			len,
			Buf);
	}
	return Irp->IoStatus.Status;
}

/***********************************************************************
* 函数名称:SfWrite 
* 函数描述:IRP_MJ_WRITE的调用函数
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
***********************************************************************/
NTSTATUS
SfWrite(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
		)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file = irpSp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

	// 对于文件系统不作处理
	if (devExt->DevType == DEV_FILESYSTEM)
	{
		return SfPassThrough(DeviceObject,Irp);
	}

	SF_RET ret;
	ret = OnSfilterIrpPre(DeviceObject,Irp);
	if (ret == SF_IRP_GO_ON)
	{
		// 进行过滤
		VOID *Buf;
		ULONG len;
		if (Irp->MdlAddress != NULL)
		{
			Buf = MmGetSystemAddressForMdl(Irp->MdlAddress);
		}
		else
		{
			Buf = Irp->UserBuffer;
		}
		ASSERT(Buf != NULL);
		len = irpSp->Parameters.Write.Length;

		// 输出信息
		UNICODE_STRING name;
		WCHAR nameBuf[1024];
		RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));

		// 获得完整的路径，使用设备对象名称
	/*	SfGetFileName( irpSp->FileObject, 
			DeviceObject,
			&name);
	*/
		// 获得完整的路径，使用符号链接名称
		SfGetFileLinkName( irpSp->FileObject, 
			DeviceObject,
			&name);
		// 经过测试如果输出的字符串中含有中文，UNICODE_STRING会被截断
		// ANSI_STRING没有影响。。。
		ANSI_STRING aName;
		RtlUnicodeStringToAnsiString(&aName,&name,TRUE);

		// 获得偏移量
		LARGE_INTEGER offset;
		offset.QuadPart = irpSp->Parameters.Write.ByteOffset.QuadPart;
		LOG_PRINT("Write Succeed! Name = %Z,Offset = %x%x, Len = %d, Buf = 0x%p\n",
			&aName,
			offset.HighPart,
			offset.LowPart,
			len,
			Buf);
		// 向下传递IRP
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(devExt->AttachedToDeviceObject,
			Irp);
		OnSfilterIrpPost(DeviceObject,Irp);
		return status;
	}
	else
	{
		// 不进行过滤
		return SfPassThrough(DeviceObject,Irp);
	}
}

/***********************************************************************
* 函数名称:SfSetInfo 
* 函数描述:IRP_MJ_SET_INFORMATION的调用函数
* 参数列表:
*		DeviceObject:设备对象
*		Irp:IRP
* 返回值:状态值
* 注:这里主要检测删除文件
***********************************************************************/
NTSTATUS
SfSetInfo(
		  IN PDEVICE_OBJECT DeviceObject,
		  IN PIRP Irp
		  )
{

	NTSTATUS status;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file = irpSp->FileObject;
	PSFILTER_DEVICE_EXTENSION devExt = (PSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(!IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject));

	// 对于文件系统不作处理
	if (devExt->DevType == DEV_FILESYSTEM)
	{
		return SfPassThrough(DeviceObject,Irp);
	}

	SF_RET ret;
	ret = OnSfilterIrpPre(DeviceObject,Irp);
	if (ret == SF_IRP_GO_ON)
	{
		if (irpSp->Parameters.SetFile.FileInformationClass == FileDispositionInformation && 
			*((BOOLEAN *)Irp->AssociatedIrp.SystemBuffer) == TRUE)
		{
			// 被设置删除状态
			// 输出信息
			UNICODE_STRING name;
			WCHAR nameBuf[1024];
			RtlInitEmptyUnicodeString(&name,nameBuf,sizeof(nameBuf));

			// 获得完整的路径，使用设备对象名称
		/*	SfGetFileName( irpSp->FileObject, 
				DeviceObject,
				&name);
		*/
			// 获得完整的路径，使用符号链接名称
			SfGetFileLinkName( irpSp->FileObject, 
				DeviceObject,
				&name);
			// 经过测试如果输出的字符串中含有中文，UNICODE_STRING会被截断
			// ANSI_STRING没有影响。。。
			ANSI_STRING aName;
			RtlUnicodeStringToAnsiString(&aName,&name,TRUE);

			LOG_PRINT("Delete Succeed! Name = %Z\n",
				&aName);
		}
		// 向下传递IRP
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(devExt->AttachedToDeviceObject,
			Irp);
		OnSfilterIrpPost(DeviceObject,Irp);
		return status;
	}
	else
	{
		// 不进行过滤
		return SfPassThrough(DeviceObject,Irp);
	}
}
