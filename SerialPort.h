#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <windowsx.h>

#ifndef HANDLE_WM_DEVICECHANGE
#define HANDLE_WM_DEVICECHANGE(hwnd, wParam, lParam, fn)                       \
    (LRESULT)(DWORD)(BOOL)(fn)((hwnd), (UINT)(wParam), (DWORD)(lParam))
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
BOOL SerialPortArrivalDevice(DWORD_PTR dwData, PINT pCom, PTSTR *pStr);
BOOL SerialPortRemoveDevice(DWORD_PTR dwData, PINT pCom, PTSTR *pStr);
