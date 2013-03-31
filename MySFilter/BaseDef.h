#ifndef _BASEDEF_H
#define _BASEDEF_H

#define MY_DEV_MAX_NAME 128
#define MY_DEV_MAX_PATH 128
#define DEVOBJ_LIST_SIZE 64

// 延时定义
#define DELAY_ONE_MICROSECOND   (-10)
#define DELAY_ONE_MILLISECOND   (DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND        (DELAY_ONE_MILLISECOND*1000)

// 分配内存的标志，内存泄露检测
#define POOL_TAG 'tlFS'

#define LOG_PRINT(msg, ...) DbgPrint(msg, ##__VA_ARGS__)

/*
// 本驱动对象全局指针
PDRIVER_OBJECT gSFilterDriverObject = NULL;

// CDO的全局指针
PDEVICE_OBJECT gSFilterControlDeviceObject = NULL;
*/

typedef enum{
	DEV_FILESYSTEM = 0,	// 文件系统对象
	DEV_VOLUME = 1		// 卷对象
} DEV_TYPE;

// 所有设备的设备拓展
typedef struct _SFILTER_DEVICE_EXTENSION 
{
	ULONG TypeFlag;

	DEV_TYPE DevType;

	// 存储文件系统指针或者卷指针
	PDEVICE_OBJECT DevObj;

	//
	//  Pointer to the file system device object we are attached to
	//

	PDEVICE_OBJECT AttachedToDeviceObject;

	//
	//  Pointer to the real (disk) device object that is associated with
	//  the file system device object we are attached to
	//

	PDEVICE_OBJECT StorageStackDeviceObject;

	//
	//  Name for this device.  If attached to a Volume Device Object it is the
	//  name of the physical disk drive.  If attached to a Control Device
	//  Object it is the name of the Control Device Object.
	//

	UNICODE_STRING DeviceName;

	//
	//  Buffer used to hold the above unicode strings
	//

	WCHAR DeviceNameBuffer[MY_DEV_MAX_NAME];


	// 卷设备的符号链接名
	UNICODE_STRING SybDeviceName;

	//
	//  Buffer used to hold the above unicode strings
	//

	WCHAR SybDeviceNameBuffer[MY_DEV_MAX_NAME];

} SFILTER_DEVICE_EXTENSION, *PSFILTER_DEVICE_EXTENSION;



// 几个测试宏

/***********************************************************************
* 宏名称:IS_MY_DEVICE_OBJECT 
* 函数描述:判断是否是本驱动生成的设备对象，并且不是CDO
* 参数列表:
*		_devObj:设备对象
* 判断依据:
*		设备对象指针不为空
*		驱动对象为gSFilterDriverObject
*		设备拓展不为空
*		设备拓展的标志为POOL_TAG
* 返回值:BOOL
***********************************************************************/
#define IS_MY_DEVICE_OBJECT(_devObj) \
	(((_devObj) != NULL) && \
	((_devObj)->DriverObject == gSFilterDriverObject) && \
	((_devObj)->DeviceExtension != NULL) && \
	((*(ULONG *)(_devObj)->DeviceExtension) == POOL_TAG))

/***********************************************************************
* 宏名称:IS_MY_CONTROL_DEVICE_OBJECT 
* 函数描述:判断是否是本驱动生成的CDO
* 参数列表:
*		_devObj:设备对象
* 判断依据:
*		设备对象为gSFilterControlDeviceObject
*		驱动对象为gSFilterDriverObject
*		没有设备拓展
* 返回值:BOOL
***********************************************************************/
#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
	(((_devObj) == gSFilterControlDeviceObject) ? \
	(ASSERT(((_devObj)->DriverObject == gSFilterDriverObject) && \
	((_devObj)->DeviceExtension == NULL)), TRUE) : \
	FALSE)

/***********************************************************************
* 宏名称:IS_DESIRED_DEVICE_TYPE 
* 函数描述:需要过滤的设备对象类型
* 参数列表:
*		_type:设备类型
* 判断依据:
*		_type的值
* 返回值:BOOL
***********************************************************************/
#define IS_DESIRED_DEVICE_TYPE(_type) \
	((_type) == FILE_DEVICE_DISK_FILE_SYSTEM)
/*
#define IS_DESIRED_DEVICE_TYPE(_type) \
	(((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
	((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
	((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))
*/

typedef enum{
	SF_IRP_GO_ON = 0,		// 传递下去，进行过滤
	SF_IRP_PASS = 1			// 传递下去，不进行过滤
} SF_RET;

// 存储目录信息
#define MAX_DIR_PATH 1024

typedef struct _DIR_NODE
{
	_DIR_NODE *nextNode;
	UNICODE_STRING DirPathStr;
	WCHAR DirPathBuf[MAX_DIR_PATH];
	ULONG referenceNum;
}DIR_NODE,*PDIR_NODE;

#endif