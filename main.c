#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME

#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
#include <dbt.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

#include "main.h"

#pragma comment(lib, "setupapi.lib")

#ifndef SPDRP_INSTALL_STATE
#define SPDRP_INSTALL_STATE (0x00000022)
#endif

#define HANDLE_WM_DEVICECHANGE(hwnd, wParam, lParam, fn)                       \
    ((fn)((hwnd), (DWORD)(wParam), (PDEV_BROADCAST_PORT)lParam), 0)

#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

static INT_PTR CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL MainDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void MainDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void MainDlg_OnDeviceChange(HWND hwnd, DWORD event,
                                   PDEV_BROADCAST_PORT p);
static void MainDlg_OnClose(HWND hwnd);

static HANDLE ghInstance;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      WCHAR *pszCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc;
    WNDCLASSEX wcx;

    ghInstance = hInstance;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    wcx.cbSize = sizeof(wcx);
    if (!GetClassInfoEx(NULL, MAKEINTRESOURCE(32770), &wcx))
        return 0;

    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_ICO_MAIN));
    wcx.lpszClassName = TEXT("SelectSeClass");
    if (!RegisterClassEx(&wcx))
        return 0;

    return DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL,
                     (DLGPROC)MainDlgProc);
}

static INT_PTR CALLBACK MainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    switch (uMsg) {
        HANDLE_DLGMSG(hwnd, WM_INITDIALOG, MainDlg_OnInitDialog);
        HANDLE_DLGMSG(hwnd, WM_COMMAND, MainDlg_OnCommand);
        HANDLE_DLGMSG(hwnd, WM_DEVICECHANGE, MainDlg_OnDeviceChange);
        HANDLE_DLGMSG(hwnd, WM_CLOSE, MainDlg_OnClose);
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
        HeapFree(GetProcessHeap(), 0, *pValue);
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
        HeapFree(GetProcessHeap(), 0, *pValue);
        *pValue = NULL;
    }
    return ret;
}

static HDEVINFO GetDevicesEnum(BOOL bPresent)
{
    static GUID guidSerial = {0x86E0D1E0L, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08,
                              0x00,        0x3E,   0x30,   0x1F, 0x73};
    DWORD dwFlags = bPresent ? DIGCF_DEVICEINTERFACE | DIGCF_PRESENT
                             : DIGCF_DEVICEINTERFACE;
    return SetupDiGetClassDevs(&guidSerial, NULL, NULL, dwFlags);
}

static BOOL EnumDevices(HDEVINFO hDevInfo, DWORD idx, BOOL bCheckPresent,
                        int *pCom, PTSTR *pStr)
{
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
            HeapFree(GetProcessHeap(), 0, *pStr);
            *pStr = NULL;
            if (GetDeviceRegPropStr(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME,
                                    pStr)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void SetPortListComStr(HWND hwnd, LONG id, int iCom, PTSTR szStr)
{
    HWND hList = GetDlgItem(hwnd, id);
    int start = 0;
    int last = ComboBox_GetCount(hList) - 1;
    while (start <= last) {
        int cur = (start + last) / 2;
        int val = ComboBox_GetItemData(hList, cur);
        if (val < iCom) {
            start = cur + 1;
        } else if (val > iCom) {
            last = cur - 1;
        } else {
            ComboBox_DeleteString(hList, cur);
            ComboBox_InsertString(hList, cur, szStr);
            ComboBox_SetItemData(hList, cur, iCom);
            return;
        }
    }
    ComboBox_InsertString(hList, start, szStr);
    ComboBox_SetItemData(hList, start, iCom);
}

static BOOL MainDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HDEVINFO hDevInfo = GetDevicesEnum(FALSE);
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        int iCom = -1;
        PTSTR szName = NULL;
        for (DWORD i = 0; EnumDevices(hDevInfo, i, TRUE, &iCom, &szName); i++) {
            SetPortListComStr(hwnd, ID_PORTLIST, iCom, szName);
            HeapFree(GetProcessHeap(), 0, szName);
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
#if 0
    DEV_BROADCAST_DEVICEINTERFACE di = {
        dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE),
        dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
        dbcc_classguid = {0x25dbce51, 0x6c8f, 0x4a72, 0x8a, 0x6d, 0xb5, 0x4c,
                          0x2b, 0x4f, 0xc8, 0x35},
    };
    PHDEVNOTIFY hDeviceNotify =
        RegisterDeviceNotification(hwnd, &di, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hDeviceNotify != NULL) {
        // OK
    }
#endif
    return TRUE;
}

static void SelectPort(HWND hwnd)
{
    HWND hList = GetDlgItem(hwnd, ID_PORTLIST);
    int iCom = ComboBox_GetItemData(hList, ComboBox_GetCurSel(hList));
    TCHAR szBuf[8];
    _sntprintf(szBuf, 8, TEXT("COM%d"), iCom);
    HWND hEdit = GetDlgItem(hwnd, ID_PORT);
    Edit_SetText(hEdit, szBuf);
}

static void MainDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case ID_PORTLIST:
        switch (codeNotify) {
        case CBN_SELCHANGE:
            SelectPort(hwnd);
            break;
        }
        break;
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, TRUE);
        return;
    }
}

static void MainDlg_OnDeviceChange(HWND hwnd, DWORD event,
                                   PDEV_BROADCAST_PORT p)
{
    switch (event) {
    case DBT_DEVICEARRIVAL:
        if (p != NULL && p->dbcp_devicetype == DBT_DEVTYP_PORT) {
            int com;
            if (_stscanf(p->dbcp_name, TEXT("COM%d"), &com) != -1) {
                HDEVINFO hDevInfo = GetDevicesEnum(TRUE);
                if (hDevInfo != INVALID_HANDLE_VALUE) {
                    int iCom = -1;
                    PTSTR szName = NULL;
                    for (DWORD i = 0;
                         EnumDevices(hDevInfo, i, FALSE, &iCom, &szName); i++) {
                        if (iCom == com) {
                            SetPortListComStr(hwnd, ID_PORTLIST, iCom, szName);
                            HeapFree(GetProcessHeap(), 0, szName);
                            break;
                        }
                        HeapFree(GetProcessHeap(), 0, szName);
                    }
                    SetupDiDestroyDeviceInfoList(hDevInfo);
                }
            }
        }
        break;
    case DBT_DEVICEREMOVECOMPLETE:
        if (p != NULL && p->dbcp_devicetype == DBT_DEVTYP_PORT) {
            int com;
            if (_stscanf(p->dbcp_name, TEXT("COM%d"), &com) != -1) {
                SetPortListComStr(hwnd, ID_PORTLIST, com, p->dbcp_name);
            }
        }
        break;
    }
}

static void MainDlg_OnClose(HWND hwnd) { EndDialog(hwnd, 0); }
