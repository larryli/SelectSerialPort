#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>

#ifndef HANDLE_WM_DEVICECHANGE
#define HANDLE_WM_DEVICECHANGE(hwnd, wParam, lParam, fn)                       \
    ((fn)((hwnd), (DWORD)(wParam), (PVOID)lParam), 0)
#endif

#define SERIALPORT_ARRIVALDEVICE 0x8000
#define SERIALPORT_REMOVEDEVICE 0x8004

typedef PVOID HSERIALPORT;

HSERIALPORT SerialPortCreateEnum(BOOL bPresent);
BOOL SerialPortEnumDevice(HSERIALPORT hSerialPort, DWORD idx,
                          BOOL bCheckPresent, PINT pCom, PTSTR *pStr);
VOID SerialPortDestroyEnum(HSERIALPORT hSerialPort);
VOID SerialPortFreeString(PTSTR pStr);

BOOL SerialPortRegisterNotification(HWND hwnd);
BOOL SerialPortArrivalDevice(PVOID lParam, PINT pCom, PTSTR *pStr);
BOOL SerialPortRemoveDevice(PVOID lParam, PINT pCom, PTSTR *pStr);
