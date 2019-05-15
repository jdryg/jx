#include <jx/monitor.h>
#include <jx/sys.h>
#include <bx/platform.h>

#if BX_PLATFORM_WINDOWS
#include <Windows.h>
#include <SetupApi.h>
#pragma comment(lib, "Setupapi.lib")
#endif

namespace jx
{
#if BX_PLATFORM_WINDOWS
#ifndef MAX_DEVICE_ID_LEN
#define MAX_DEVICE_ID_LEN 200
#endif

#define NAME_SIZE 128

static const GUID GUID_CLASS_MONITOR = { 0x4d36e96e, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };
static HMONITOR s_PrimaryMonitorHandle = NULL;

static BOOL CALLBACK monitorEnumProc(_In_  HMONITOR hMonitor, _In_  HDC /*hdcMonitor*/, _In_  LPRECT /*lprcMonitor*/, _In_  LPARAM /*dwData*/)
{
	// Use this function to identify the monitor of interest: MONITORINFO contains the Monitor RECT.
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);

	GetMonitorInfoA(hMonitor, &mi);

	JX_TRACE("Monitor %s", mi.szDevice);
	JX_TRACE("- Rect: t=%d, b=%d, l=%d, r=%d", mi.rcMonitor.top, mi.rcMonitor.bottom, mi.rcMonitor.left, mi.rcMonitor.right);
	JX_TRACE("- Work: t=%d, b=%d, l=%d, r=%d", mi.rcWork.top, mi.rcWork.bottom, mi.rcWork.left, mi.rcWork.right);

	if (s_PrimaryMonitorHandle != NULL) {
		s_PrimaryMonitorHandle = hMonitor;
	} else if ((mi.dwFlags & MONITORINFOF_PRIMARY) != 0) {
		s_PrimaryMonitorHandle = hMonitor;
	}

	return TRUE;
}

static BOOL DisplayDeviceFromHMonitor(HMONITOR hMonitor, DISPLAY_DEVICE& ddMonOut)
{
	MONITORINFOEX mi;
	mi.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfoA(hMonitor, &mi);

	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);

	DWORD devIdx = 0; // device index
	while (EnumDisplayDevicesA(0, devIdx, &dd, 0)) {
		devIdx++;
		if (0 != strcmp(dd.DeviceName, mi.szDevice)) {
			continue;
		}

		DISPLAY_DEVICE ddMon;
		ZeroMemory(&ddMon, sizeof(ddMon));
		ddMon.cb = sizeof(ddMon);
		DWORD MonIdx = 0;
		while (EnumDisplayDevicesA(dd.DeviceName, MonIdx, &ddMon, 0)) {
			MonIdx++;

			ddMonOut = ddMon;
			return TRUE;
		}

		ZeroMemory(&dd, sizeof(dd));
		dd.cb = sizeof(dd);
	}

	return FALSE;
}

static void get2ndSlashBlock(const char* str, char* secondBlock)
{
	const char* firstSlash = strchr(str, '\\');
	const char* secondSlash = strchr(firstSlash + 1, '\\');
	__int64 len = secondSlash - firstSlash - 1;
	memcpy(secondBlock, firstSlash + 1, len);
	secondBlock[len] = '\0';
}

// Assumes hEDIDRegKey is valid
static bool GetMonitorSizeFromEDID(const HKEY hEDIDRegKey, short& WidthMm, short& HeightMm)
{
	DWORD dwType, AcutalValueNameLength = NAME_SIZE;
	TCHAR valueName[NAME_SIZE];

	BYTE EDIDdata[1024];
	DWORD edidsize = sizeof(EDIDdata);

	for (LONG i = 0, retValue = ERROR_SUCCESS; retValue != ERROR_NO_MORE_ITEMS; ++i) {
		retValue = RegEnumValue(hEDIDRegKey, i, &valueName[0], &AcutalValueNameLength, NULL, &dwType, EDIDdata, &edidsize);
		if (retValue != ERROR_SUCCESS || 0 != strcmp(valueName, "EDID")) {
			continue;
		}

		WidthMm = ((EDIDdata[68] & 0xF0) << 4) + EDIDdata[66];
		HeightMm = ((EDIDdata[68] & 0x0F) << 8) + EDIDdata[67];

		return true; // valid EDID found
	}

	return false; // EDID not found
}

static bool GetSizeForDevID(const char* TargetDevID, short& WidthMm, short& HeightMm)
{
	HDEVINFO devInfo = SetupDiGetClassDevsEx(
		&GUID_CLASS_MONITOR, //class GUID
		NULL, //enumerator
		NULL, //HWND
		DIGCF_PRESENT | DIGCF_PROFILE, // Flags //DIGCF_ALLCLASSES|
		NULL, // device info, create a new one.
		NULL, // machine name, local machine
		NULL);// reserved

	if (NULL == devInfo) {
		return false;
	}

	bool bRes = false;

	for (ULONG i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i) {
		SP_DEVINFO_DATA devInfoData;
		memset(&devInfoData, 0, sizeof(devInfoData));
		devInfoData.cbSize = sizeof(devInfoData);

		if (SetupDiEnumDeviceInfo(devInfo, i, &devInfoData)) {
			TCHAR Instance[MAX_DEVICE_ID_LEN];
			SetupDiGetDeviceInstanceId(devInfo, &devInfoData, Instance, MAX_PATH, NULL);

			if (!strstr(Instance, TargetDevID)) {
				continue;
			}

			HKEY hEDIDRegKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (!hEDIDRegKey || (hEDIDRegKey == INVALID_HANDLE_VALUE)) {
				continue;
			}

			bRes = GetMonitorSizeFromEDID(hEDIDRegKey, WidthMm, HeightMm);

			RegCloseKey(hEDIDRegKey);
		}
	}

	SetupDiDestroyDeviceInfoList(devInfo);

	return bRes;
}

bool getMonitorDimensions(uint16_t* width_mm, uint16_t* height_mm)
{
	// Identify the HMONITOR of interest via the callback MyMonitorEnumProc
	EnumDisplayMonitors(NULL, NULL, monitorEnumProc, NULL);

	JX_WARN(s_PrimaryMonitorHandle != NULL, "Failed to retrieve primary monitor.\n");
	if (s_PrimaryMonitorHandle == NULL) {
		*width_mm = 1;
		*height_mm = 1;
		return false;
	}

	DISPLAY_DEVICE ddMon;
	if (DisplayDeviceFromHMonitor(s_PrimaryMonitorHandle, ddMon) == FALSE) {
		*width_mm = 1;
		*height_mm = 1;
		return false;
	}

	char deviceID[256];
	get2ndSlashBlock(ddMon.DeviceID, deviceID);

	short WidthMm, HeightMm;
	bool success = GetSizeForDevID(deviceID, WidthMm, HeightMm);
	if (success) {
		*width_mm = (uint16_t)WidthMm;
		*height_mm = (uint16_t)HeightMm;
	} else {
		*width_mm = 1;
		*height_mm = 1;
	}

	return success;
}
#else // BX_PLATFORM_WINDOWS
bool getMonitorDimensions(uint16_t* width_mm, uint16_t* height_mm)
{
	*width_mm = 1;
	*height_mm = 1;
	return false;
}
#endif
}