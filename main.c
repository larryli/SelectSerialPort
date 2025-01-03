#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOSERVICE
#define NOMCX
#define NOIME

#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

#include "SerialPort.h"
#include "main.h"

static INT_PTR CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL MainDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void MainDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL MainDlg_OnDeviceChange(HWND hwnd, UINT nEventType,
                                   DWORD_PTR dwData);
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
    wcx.lpszClassName = TEXT("SelSerialPortClass");
    if (!RegisterClassEx(&wcx))
        return 0;

    return (int)DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL,
                          (DLGPROC)MainDlgProc);
}

static INT_PTR CALLBACK MainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    switch (uMsg) {
        HANDLE_MSG(hwnd, WM_INITDIALOG, MainDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, MainDlg_OnCommand);
        HANDLE_MSG(hwnd, WM_DEVICECHANGE, MainDlg_OnDeviceChange);
        HANDLE_MSG(hwnd, WM_CLOSE, MainDlg_OnClose);
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
        int val = (int)ComboBox_GetItemData(hList, cur);
        if (val < iCom) {
            start = cur + 1;
        } else if (val > iCom) {
            last = cur - 1;
        } else {
            BOOL bSel = ComboBox_GetCurSel(hList) == cur;
            ComboBox_DeleteString(hList, cur);
            ComboBox_InsertString(hList, cur, szStr);
            ComboBox_SetItemData(hList, cur, iCom);
            if (bSel) {
                ComboBox_SetCurSel(hList, cur);
            }
            return;
        }
    }
    ComboBox_InsertString(hList, start, szStr);
    ComboBox_SetItemData(hList, start, iCom);
}

static BOOL MainDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HSERIALPORT hSerialPort = SerialPortCreateEnum(FALSE);
    if (hSerialPort != INVALID_HANDLE_VALUE) {
        int iCom;
        PTSTR szName;
        for (DWORD i = 0;
             SerialPortEnumDevice(hSerialPort, i, TRUE, &iCom, &szName); i++) {
            SetPortListComStr(hwnd, ID_PORTLIST, iCom, szName);
            SerialPortFreeString(szName);
        }
        SerialPortDestroyEnum(hSerialPort);
    }
#if 0
    SerialPortRegisterNotification(hwnd);
#endif
    return TRUE;
}

static void SelectPort(HWND hwnd)
{
    HWND hList = GetDlgItem(hwnd, ID_PORTLIST);
    int iCom = (int)ComboBox_GetItemData(hList, ComboBox_GetCurSel(hList));
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

static BOOL MainDlg_OnDeviceChange(HWND hwnd, UINT nEventType, DWORD_PTR dwData)
{
    INT iCom;
    PTSTR szName;
    switch (nEventType) {
    case SERIALPORT_ARRIVALDEVICE:
        if (SerialPortArrivalDevice(dwData, &iCom, &szName)) {
            SetPortListComStr(hwnd, ID_PORTLIST, iCom, szName);
            SerialPortFreeString(szName);
        }
        break;
    case SERIALPORT_REMOVEDEVICE:
        if (SerialPortRemoveDevice(dwData, &iCom, &szName)) {
            SetPortListComStr(hwnd, ID_PORTLIST, iCom, szName);
            SerialPortFreeString(szName);
        }
        break;
    }
    return TRUE;
}

static void MainDlg_OnClose(HWND hwnd) { EndDialog(hwnd, 0); }
