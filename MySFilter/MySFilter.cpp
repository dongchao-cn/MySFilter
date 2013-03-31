#include <ntifs.h>
#include <ntdddisk.h>
#include <wdmsec.h>
#include "BaseDef.h"
#include "BaseFun.h"
#include "MySFilter.h"
#include "MajorFunction.h"
#include "FastIO.h"
#include "User.h"

// 本驱动对象全局指针
PDRIVER_OBJECT gSFilterDriverObject = NULL;

// CDO的全局指针
PDEVICE_OBJECT gSFilterControlDeviceObject = NULL;

// 存储CDO的符号链接名
UNICODE_STRING gCDOLinkNameStr;
WCHAR gCDOLinkNameBuf[MY_DEV_MAX_NAME];

// 互斥体
FAST_MUTEX gSfilterAttachLock;

// 目录存储指针
extern PDIR_NODE gListHead;

// 函数声明
VOID
DriverUnload (
			  IN PDRIVER_OBJECT DriverObject
			  );


extern "C" NTSTATUS DriverEntry(
					 IN PDRIVER_OBJECT DriverObject,
					 IN PUNICODE_STRING RegistryPath
					 )
{

	_asm int 3

	NTSTATUS status = STATUS_SUCCESS;

	// 保存驱动对象指针
	gSFilterDriverObject = DriverObject;

	// 卸载例程
	gSFilterDriverObject->DriverUnload = DriverUnload;

	// 初始化互斥体
	ExInitializeFastMutex(&gSfilterAttachLock);

	// 初始化目录链
	gListHead = NULL;

	// 调用回调函数
	UNICODE_STRING CDONameStr;
	WCHAR CDONameBuf[MY_DEV_MAX_NAME];
	UNICODE_STRING CDOSybLnkStr;
	WCHAR CDOSybLnkBuf[MY_DEV_MAX_NAME];
	RtlInitEmptyUnicodeString(&CDONameStr,CDONameBuf,sizeof(CDONameBuf));
	RtlInitEmptyUnicodeString(&CDOSybLnkStr,CDOSybLnkBuf,sizeof(CDOSybLnkBuf));

	status = OnSfilterDriverEntry(gSFilterDriverObject,RegistryPath,&CDONameStr,&CDOSybLnkStr);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("OnSfilterDriverEntry Failed!\n"));
		return status;
	}

	// 填写完成路径
	UNICODE_STRING CDOFullNameStr;
	WCHAR CDOFullNameBuf[MY_DEV_MAX_PATH];
	RtlInitEmptyUnicodeString(&CDOFullNameStr,CDOFullNameBuf,sizeof(CDOFullNameBuf));
	UNICODE_STRING path;
	RtlInitUnicodeString(&path,L"\\FileSystem\\Filters\\");
	RtlCopyUnicodeString(&CDOFullNameStr,&path);
	RtlAppendUnicodeStringToString(&CDOFullNameStr,&CDONameStr);
	
	// 生成CDO
	status = IoCreateDevice(DriverObject,
		0,
		&CDOFullNameStr,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&gSFilterControlDeviceObject);

	if (!NT_SUCCESS(status))
	{
		// XP之前的版本可能存在status == STATUS_OBJECT_PATH_NOT_FOUND,这里不作处理
		KdPrint(("IoCreateDevice Failed"));
		return status;
	}

	// 生成符号链接
	UNICODE_STRING CDOFullSybLnkStr;
	WCHAR CDOSybLnkFullBuf[MY_DEV_MAX_PATH];
	RtlInitEmptyUnicodeString(&CDOFullSybLnkStr,CDOSybLnkFullBuf,sizeof(CDOSybLnkFullBuf));
	RtlCopyUnicodeString(&CDOFullSybLnkStr,&path);
	RtlAppendUnicodeStringToString(&CDOFullSybLnkStr,&CDOSybLnkStr);

	status = IoCreateSymbolicLink(&CDOFullSybLnkStr,&CDOFullNameStr);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateSymbolicLink Failed"));
		// 删除生成的CDO
		IoDeleteDevice(gSFilterControlDeviceObject);
		return status;
	}
	
	// 填写全局设备连接名
	RtlInitEmptyUnicodeString(&gCDOLinkNameStr,gCDOLinkNameBuf,sizeof(gCDOLinkNameBuf));
	RtlCopyUnicodeString(&gCDOLinkNameStr,&CDOFullSybLnkStr);

	// 填写MajorFunction
	ULONG i;
	for (i = 0;i < IRP_MJ_MAXIMUM_FUNCTION;i++)
	{
		gSFilterDriverObject->MajorFunction[i] = SfPassThrough;
	}

	gSFilterDriverObject->MajorFunction[IRP_MJ_CREATE] = SfCreate;
	// 我不想处理这两个，真的。。
//	DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = SfCreate;
//	DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] = SfCreate;

	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SfFsControl;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = SfCleanupClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = SfRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = SfWrite;
	DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = SfSetInfo;

	// 设置FastIO
	PFAST_IO_DISPATCH fastIoDispatch;
	fastIoDispatch = (PFAST_IO_DISPATCH)ExAllocatePoolWithTag(NonPagedPool,sizeof(FAST_IO_DISPATCH),POOL_TAG);

	if (!fastIoDispatch)
	{
		KdPrint(("ExAllocatePoolWithTag For fastIoDispatch Failed"));
		IoDeleteDevice(gSFilterControlDeviceObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(fastIoDispatch,sizeof(FAST_IO_DISPATCH));

	fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
	fastIoDispatch->FastIoCheckIfPossible = SfFastIoCheckIfPossible;
	fastIoDispatch->FastIoRead = SfFastIoRead;
	fastIoDispatch->FastIoWrite = SfFastIoWrite;
	fastIoDispatch->FastIoQueryBasicInfo = SfFastIoQueryBasicInfo;
	fastIoDispatch->FastIoQueryStandardInfo = SfFastIoQueryStandardInfo;
	fastIoDispatch->FastIoLock = SfFastIoLock;
	fastIoDispatch->FastIoUnlockSingle = SfFastIoUnlockSingle;
	fastIoDispatch->FastIoUnlockAll = SfFastIoUnlockAll;
	fastIoDispatch->FastIoUnlockAllByKey = SfFastIoUnlockAllByKey;
	fastIoDispatch->FastIoDeviceControl = SfFastIoDeviceControl;
	fastIoDispatch->FastIoDetachDevice = SfFastIoDetachDevice;
	fastIoDispatch->FastIoQueryNetworkOpenInfo = SfFastIoQueryNetworkOpenInfo;
	fastIoDispatch->MdlRead = SfFastIoMdlRead;
	fastIoDispatch->MdlReadComplete = SfFastIoMdlReadComplete;
	fastIoDispatch->PrepareMdlWrite = SfFastIoPrepareMdlWrite;
	fastIoDispatch->MdlWriteComplete = SfFastIoMdlWriteComplete;
	fastIoDispatch->FastIoReadCompressed = SfFastIoReadCompressed;
	fastIoDispatch->FastIoWriteCompressed = SfFastIoWriteCompressed;
	fastIoDispatch->MdlReadCompleteCompressed = SfFastIoMdlReadCompleteCompressed;
	fastIoDispatch->MdlWriteCompleteCompressed = SfFastIoMdlWriteCompleteCompressed;
	fastIoDispatch->FastIoQueryOpen = SfFastIoQueryOpen;

	gSFilterDriverObject->FastIoDispatch = fastIoDispatch;


	// 几个特殊的回调函数
	FS_FILTER_CALLBACKS fsFilterCallbacks;
	fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
	fsFilterCallbacks.PreAcquireForSectionSynchronization = SfPreFsFilterPassThrough;
	fsFilterCallbacks.PostAcquireForSectionSynchronization = SfPostFsFilterPassThrough;
	fsFilterCallbacks.PreReleaseForSectionSynchronization = SfPreFsFilterPassThrough;
	fsFilterCallbacks.PostReleaseForSectionSynchronization = SfPostFsFilterPassThrough;
	fsFilterCallbacks.PreAcquireForCcFlush = SfPreFsFilterPassThrough;
	fsFilterCallbacks.PostAcquireForCcFlush = SfPostFsFilterPassThrough;
	fsFilterCallbacks.PreReleaseForCcFlush = SfPreFsFilterPassThrough;
	fsFilterCallbacks.PostReleaseForCcFlush = SfPostFsFilterPassThrough;
	fsFilterCallbacks.PreAcquireForModifiedPageWriter = SfPreFsFilterPassThrough;
	fsFilterCallbacks.PostAcquireForModifiedPageWriter = SfPostFsFilterPassThrough;
	fsFilterCallbacks.PreReleaseForModifiedPageWriter = SfPreFsFilterPassThrough;
	fsFilterCallbacks.PostReleaseForModifiedPageWriter = SfPostFsFilterPassThrough;

	status = FsRtlRegisterFileSystemFilterCallbacks( DriverObject, 
		&fsFilterCallbacks );

	if (!NT_SUCCESS( status )) 
	{
		DriverObject->FastIoDispatch = NULL;
		ExFreePool( fastIoDispatch );
		IoDeleteDevice( gSFilterControlDeviceObject );
		return status;
	}

	// 注册文件系统激活或停止的回调函数
	status = IoRegisterFsRegistrationChange(gSFilterDriverObject,SfFsNotification);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoRegisterFsRegistrationChange Failed"));
		gSFilterDriverObject->FastIoDispatch = NULL;
		ExFreePoolWithTag(fastIoDispatch,POOL_TAG);
		IoDeleteDevice(gSFilterControlDeviceObject);
		return status;
	}

	// RAW file system device 未处理，不知道是什么。。

	// 设置初始化全部完成
	ClearFlag(gSFilterDriverObject->Flags,DO_DEVICE_INITIALIZING);

	return STATUS_SUCCESS;
}

/***********************************************************************
* 函数名称:DriverUnload 
* 函数描述:驱动卸载函数（正常情况下文件驱动不应该被卸载）
* 参数列表:
*		空
* 返回值:空
***********************************************************************/
VOID
DriverUnload (
			  IN PDRIVER_OBJECT DriverObject
			  )
{
	LOG_PRINT("DriverUnload!");
	
	NTSTATUS status = STATUS_SUCCESS;
	ASSERT(DriverObject == gSFilterDriverObject);

	// 停止接受文件系统变化
	IoUnregisterFsRegistrationChange(gSFilterDriverObject,SfFsNotification);

	PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];
	ULONG numDevices;

	while(1)
	{
		// 设成死循环，直到所有的设备都清理完成

		// 获得本驱动产生的所有设备列表
		status = IoEnumerateDeviceObjectList(gSFilterDriverObject,
			devList,
			sizeof(devList),
			&numDevices);

		if (numDevices <= 0)
		{
			// 没有设备了，可以退出了
			break;
		}
		
		ULONG i;
		PSFILTER_DEVICE_EXTENSION devExt;
		for (i = 0;i < numDevices;i++)
		{
			devExt = (PSFILTER_DEVICE_EXTENSION)devList[i]->DeviceExtension;
			if (devExt != NULL)
			{
				// 不是CDO，取消其绑定的设备
				IoDetachDevice(devExt->AttachedToDeviceObject);
			}
		}

		// 取消之前可能还有已经发送出来的IRP，延时一段时间，确保所有IRP正常结束
		LARGE_INTEGER interval;
		interval.QuadPart = (1 * DELAY_ONE_SECOND);
		KeDelayExecutionThread(KernelMode,FALSE,&interval);

		// 清理驱动对象
		for (i = 0; i < numDevices;i++)
		{
			
			if (devList[i]->DeviceExtension != NULL)
			{
				// 不是CDO，清理
			//	KdPrint(("IoDeleteDevice %d",i));
				SfCleanupMountedDevice( devList[i] );
			} 
			else
			{
				if(devList[i] == gSFilterControlDeviceObject)
				{
				//	KdPrint(("IoDeleteDevice CDO"));
					// 是CDO
					gSFilterControlDeviceObject = NULL;
					// 删除符号链接
					IoDeleteSymbolicLink(&gCDOLinkNameStr);
				}
			}
			IoDeleteDevice( devList[i] );
			ObDereferenceObject( devList[i] );
		}
	}

	// 清理FastIO申请的空间
	ExFreePoolWithTag(gSFilterDriverObject->FastIoDispatch,POOL_TAG);
	gSFilterDriverObject->FastIoDispatch = NULL;

	// 调用用户回调函数
	OnSfilterDriverUnload();
}