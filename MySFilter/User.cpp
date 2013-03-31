#include <ntifs.h>
#include "MySFilter.h"
#include "BaseFun.h"
WCHAR MajorFunTable[][50] = {
L"IRP_MJ_CREATE",
L"IRP_MJ_CREATE_NAMED_PIPE",
L"IRP_MJ_CLOSE",
L"IRP_MJ_READ",
L"IRP_MJ_WRITE",
L"IRP_MJ_QUERY_INFORMATION",
L"IRP_MJ_SET_INFORMATION",
L"IRP_MJ_QUERY_EA",
L"IRP_MJ_SET_EA",
L"IRP_MJ_FLUSH_BUFFERS",
L"IRP_MJ_QUERY_VOLUME_INFORMATION",
L"IRP_MJ_SET_VOLUME_INFORMATION",
L"IRP_MJ_DIRECTORY_CONTROL",
L"IRP_MJ_FILE_SYSTEM_CONTROL",
L"IRP_MJ_DEVICE_CONTROL",
L"IRP_MJ_INTERNAL_DEVICE_CONTROL",
L"IRP_MJ_SHUTDOWN",
L"IRP_MJ_LOCK_CONTROL",
L"IRP_MJ_CLEANUP",
L"IRP_MJ_CREATE_MAILSLOT",
L"IRP_MJ_QUERY_SECURITY",
L"IRP_MJ_SET_SECURITY",
L"IRP_MJ_POWER",
L"IRP_MJ_SYSTEM_CONTROL",
L"IRP_MJ_DEVICE_CHANGE",
L"IRP_MJ_QUERY_QUOTA",
L"IRP_MJ_SET_QUOTA",
L"IRP_MJ_PNP"
};
/***********************************************************************
* 函数名称:OnSfilterDriverEntry 
* 函数描述:DriverEntry的回调函数，填写设备名和符号链接名
* 参数列表:
*		DriverObject:驱动对象
*		RegistryPath:注册表路径
*		userNameString:设备名
*		syblnkString:符号链接名
* 返回值:状态值
***********************************************************************/
NTSTATUS OnSfilterDriverEntry(
							  IN PDRIVER_OBJECT DriverObject,
							  IN PUNICODE_STRING RegistryPath,
							  OUT PUNICODE_STRING userNameString,
							  OUT PUNICODE_STRING syblnkString
							  )
{
	NTSTATUS status = STATUS_SUCCESS;

	// 确定控制设备的名字和符号链接。
	UNICODE_STRING user_name,syb_name;
	RtlInitUnicodeString(&user_name,L"MySFilterCDO");
	RtlInitUnicodeString(&syb_name,L"MySFilterCDOSyb");
	RtlCopyUnicodeString(userNameString,&user_name);
	RtlCopyUnicodeString(syblnkString,&syb_name);

	return status;
}

/***********************************************************************
* 函数名称:OnSfilterDriverUnload 
* 函数描述:DriverUnload的回调函数
* 参数列表:
*		空
* 返回值:空
***********************************************************************/
VOID OnSfilterDriverUnload()
{
	// 没什么要做的...
}

/***********************************************************************
* 函数名称:OnSfilterAttachPre 
* 函数描述:判断是否绑定次设备
* 参数列表:
*		ourDevice:过滤设备
*		theDeviceToAttach:将要被绑定设备
*		DeviceName:被绑定设备的设备名称
* 返回值:状态值
***********************************************************************/
BOOLEAN OnSfilterAttachPre(
						   IN PDEVICE_OBJECT ourDevice,
						   IN PDEVICE_OBJECT theDeviceToAttach,
						   IN PUNICODE_STRING DeviceName
						   )
{
	// 所有的设备都绑定
	return TRUE;
}

/***********************************************************************
* 函数名称:OnSfilterAttachPost 
* 函数描述:绑定成功之后的回调函数
* 参数列表:
*		ourDevice:过滤设备
*		theDeviceToAttach:将要被绑定设备
*		theDeviceToAttached:被绑定设备
*		status:绑定状态
* 返回值:状态值
***********************************************************************/
VOID OnSfilterAttachPost(
						 IN PDEVICE_OBJECT ourDevice,
						 IN PDEVICE_OBJECT theDeviceToAttach,
						 IN PDEVICE_OBJECT theDeviceToAttached,
						 IN NTSTATUS status)
{
	// 无需处理
}

/***********************************************************************
* 函数名称:OnSfilterIrpPre 
* 函数描述:在处理IRP之前的调用函数
* 参数列表:
*		DeviceObject:当前设备
*		Irp:IRP
* 返回值:是否过滤
***********************************************************************/
SF_RET OnSfilterIrpPre(
					   IN PDEVICE_OBJECT DeviceObject,
					   IN PIRP Irp)
{
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT file = irpSp->FileObject;

	if (file == NULL)
	{
		// 不是文件对象，不做处理
		return SF_IRP_PASS;
	}

	// 是文件对象


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

	UNICODE_STRING filterPathStr;
	RtlInitUnicodeString(&filterPathStr,L"C:\\123\\");
	
	USHORT oldLen = name.Length;
	name.Length = filterPathStr.Length;

	if (RtlCompareUnicodeString(&name,&filterPathStr,TRUE) == 0)
	{
		// 需要过滤的目录
		name.Length = oldLen;
	/*	KdPrint(("SF_IRP_GO_ON\n"));
		// 经过测试如果输出的字符串中含有中文，UNICODE_STRING会被截断
		// ANSI_STRING没有影响。。。
			ANSI_STRING aName;
		RtlUnicodeStringToAnsiString(&aName,&file->FileName,TRUE);

		LOG_PRINT("IRP: file name = %Z, major function = %S\n",
			&aName,
			MajorFunTable[irpSp->MajorFunction]);
		RtlFreeAnsiString(&aName);
	*/
	
		return SF_IRP_GO_ON;
	}
	name.Length = oldLen;
	return SF_IRP_PASS;
}

/***********************************************************************
* 函数名称:OnSfilterIrpPost 
* 函数描述:在处理IRP之后的调用函数
* 参数列表:
*		DeviceObject:当前设备
*		Irp:IRP
* 返回值:空
***********************************************************************/
VOID OnSfilterIrpPost(
					  IN PDEVICE_OBJECT DeviceObject,
					  IN PIRP Irp)
{
	return;
}