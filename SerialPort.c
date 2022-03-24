#include "SerialPort.h"

#include <dbt.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

#pragma comment(lib, "setupapi.lib")

#ifndef SPDRP_INSTALL_STATE
#define SPDRP_INSTALL_STATE (0x00000022)
#endif

static LONG GetRegValueStr(HKEY hKey, PTSTR pName, PTSTR *pValue);
static BOOL GetDeviceRegPropVar(HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp,
                                DWORD *pValue);
static BOOL GetDeviceRegPropStr(HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp,
                                PTSTR *pValue);

HSERIALPORT SerialPortCreateEnum(BOOL bPresent)
{
    static GUID guidSerial = {0x86E0D1E0L, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08,
                              0x00,        0x3E,   0x30,   0x1F, 0x73};
    DWORD dwFlags = bPresent ? DIGCF_DEVICEINTERFACE | DIGCF_PRESENT
                             : DIGCF_DEVICEINTERFACE;
    return (HSERIALPORT)SetupDiGetClassDevs(&guidSerial, NULL, NULL, dwFlags);
}

BOOL SerialPortEnumDevice(HSERIALPORT hSerialPort, DWORD idx,
                          BOOL bCheckPresent, PINT pCom, PTSTR *pStr)
{
    HDEVINFO hDevInfo = (HDEVINFO)hSerialPort;
    SP_DEVINFO_DATA devInfoData = {
        .cbSize = sizeof(SP_DEVINFO_DATA),
    };
    if (SetupDiEnumDeviceInfo(hDevInfo, idx, &devInfoData) == FALSE) {
        return FALSE;
    }
    HKEY hDevKey = SetupDiOpenDevRegKey(
        hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hDevKey == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    LONG ret = GetRegValueStr(hDevKey, TEXT("PortName"), pStr);
    RegCloseKey(hDevKey);
    if (ret == ERROR_SUCCESS) {
        if (_stscanf(*pStr, TEXT("COM%d"), pCom) != -1) {
            DWORD dwInstallState;
            if (bCheckPresent &&
                GetDeviceRegPropVar(hDevInfo, &devInfoData, SPDRP_INSTALL_STATE,
                                    &dwInstallState) == FALSE) {
                return TRUE;
            }
            SerialPortFreeString(*pStr);
            *pStr = NULL;
            if (GetDeviceRegPropStr(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME,
                                    pStr)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

VOID SerialPortDestroyEnum(HSERIALPORT hSerialPort)
{
    SetupDiDestroyDeviceInfoList((HDEVINFO)hSerialPort);
}

VOID SerialPortFreeString(PTSTR pStr) { HeapFree(GetProcessHeap(), 0, pStr); }

BOOL SerialPortRegisterNotification(HWND hwnd)
{
    DEV_BROADCAST_DEVICEINTERFACE di = {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
        .dbcc_classguid = {0x25dbce51, 0x6c8f, 0x4a72, 0x8a, 0x6d, 0xb5, 0x4c,
                           0x2b, 0x4f, 0xc8, 0x35},
    };
    PHDEVNOTIFY hDeviceNotify =
        RegisterDeviceNotification(hwnd, &di, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hDeviceNotify != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL SerialPortArrivalDevice(PVOID lParam, PINT pCom, PTSTR *pStr)
{
    PDEV_BROADCAST_PORT p = (PDEV_BROADCAST_PORT)lParam;
    if (p == NULL || p->dbcp_devicetype != DBT_DEVTYP_PORT) {
        return FALSE;
    }
    int com;
    if (_stscanf(p->dbcp_name, TEXT("COM%d"), &com) != -1) {
        HSERIALPORT hSerialPort = SerialPortCreateEnum(TRUE);
        if (hSerialPort != INVALID_HANDLE_VALUE) {
            for (DWORD i = 0;
                 SerialPortEnumDevice(hSerialPort, i, FALSE, pCom, pStr); i++) {
                if (*pCom == com) {
                    SerialPortDestroyEnum(hSerialPort);
                    return TRUE;
                }
                SerialPortFreeString(*pStr);
            }
            SerialPortDestroyEnum(hSerialPort);
        }
    }
    return FALSE;
}

BOOL SerialPortRemoveDevice(PVOID lParam, PINT pCom, PTSTR *pStr)
{
    PDEV_BROADCAST_PORT p = (PDEV_BROADCAST_PORT)lParam;
    if (p == NULL || p->dbcp_devicetype != DBT_DEVTYP_PORT) {
        return FALSE;
    }
    if (_stscanf(p->dbcp_name, TEXT("COM%d"), pCom) != -1) {
        ULONG size = (_tcslen(p->dbcp_name) + 1) * sizeof(TCHAR);
        *pStr = (PTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if (*pStr == NULL) {
            return FALSE;
        }
        _tcscpy(*pStr, p->dbcp_name);
        return TRUE;
    }
    return FALSE;
}

static LONG GetRegValueStr(HKEY hKey, PTSTR pName, PTSTR *pValue)
{
    DWORD cbValue = 0;
    LONG ret = RegQueryValueEx(hKey, pName, NULL, NULL, NULL, &cbValue);
    if (ret != ERROR_SUCCESS) {
        return ret;
    }
    *pValue = (PTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbValue);
    if (*pValue == NULL) {
        return GetLastError();
    }
    ret = RegQueryValueEx(hKey, pName, NULL, NULL, (PBYTE)*pValue, &cbValue);
    if (ret != ERROR_SUCCESS) {
        SerialPortFreeString(*pValue);
        *pValue = NULL;
    }
    return ret;
}

static BOOL GetDeviceRegPropVar(HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp,
                                DWORD *pValue)
{
    return SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, dwProp,
                                            NULL, (PBYTE)pValue, sizeof(DWORD),
                                            NULL);
}

static BOOL GetDeviceRegPropStr(HDEVINFO hDevInfo,
                                PSP_DEVINFO_DATA pDevInfoData, DWORD dwProp,
                                PTSTR *pValue)
{
    DWORD cbValue = 0;
    SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, dwProp, NULL, NULL,
                                     0, &cbValue);
    if (cbValue == 0) {
        return FALSE;
    }
    *pValue = (PTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbValue);
    if (*pValue == NULL) {
        return FALSE;
    }
    BOOL ret = SetupDiGetDeviceRegistryProperty(
        hDevInfo, pDevInfoData, dwProp, NULL, (PBYTE)*pValue, cbValue, NULL);
    if (ret == FALSE) {
        SerialPortFreeString(*pValue);
        *pValue = NULL;
    }
    return ret;
}
