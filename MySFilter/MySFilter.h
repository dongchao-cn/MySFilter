/***********************************************************************
所有的回调函数
***********************************************************************/
#ifndef _MYSFILTER_H
#define _MYSFILTER_H
#include "BaseDef.h"

//extern 
SF_RET OnSfilterIrpPre(
					   IN PDEVICE_OBJECT DeviceObject,
					   IN PIRP Irp);

//extern 
VOID OnSfilterIrpPost(
					  IN PDEVICE_OBJECT DeviceObject,
					  IN PIRP Irp);

//extern 
NTSTATUS OnSfilterDriverEntry(
							  IN PDRIVER_OBJECT DriverObject,
							  IN PUNICODE_STRING RegistryPath,
							  OUT PUNICODE_STRING userNameString,
							  OUT PUNICODE_STRING syblnkString
							  );

//extern 
VOID OnSfilterDriverUnload();

//extern 
NTSTATUS OnSfilterCDODispatch(
							  IN PDEVICE_OBJECT DeviceObject,
							  IN PIRP Irp);

//extern
BOOLEAN OnSfilterAttachPre(
						   IN PDEVICE_OBJECT ourDevice,
						   IN PDEVICE_OBJECT theDeviceToAttach,
						   IN PUNICODE_STRING DeviceName
						   );

//extern 
VOID OnSfilterAttachPost(
						 IN PDEVICE_OBJECT ourDevice,
						 IN PDEVICE_OBJECT theDeviceToAttach,
						 IN PDEVICE_OBJECT theDeviceToAttached,
						 IN NTSTATUS status);

//extern
BOOLEAN OnSfFastIoDeviceControl(
								IN PFILE_OBJECT FileObject,
								IN PVOID InputBuffer OPTIONAL,
								IN ULONG InputBufferLength,
								OUT PVOID OutputBuffer OPTIONAL,
								IN ULONG OutputBufferLength,
								IN ULONG IoControlCode,
								OUT PIO_STATUS_BLOCK IoStatus,
								IN PDEVICE_OBJECT DeviceObject);

#endif