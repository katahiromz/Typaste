// Typaste.cpp --- Typing + Paste
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "Typaste.hpp"

#define DEFAULT_DELAY 20
#define DEFALUT_HOTKEY MAKEWORD('V', HOTKEYF_CONTROL)
#define HOTKEY_ID 0xDEDD

HWND s_hwndMain = NULL;
DWORD s_dwDelayToType = DEFAULT_DELAY;
DWORD s_dwDelayToStart = 0;
WORD s_wHotKey = DEFALUT_HOTKEY;
HICON s_hIcon = NULL;
HICON s_hIconSm = NULL;
INT s_nExitCode = IDCANCEL;
std::wstring s_strSound;
BOOL s_bControlIME = TRUE;

static const TCHAR s_szName[] = TEXT("Typaste");

BOOL MyRegisterHotKey(HWND hwnd)
{
    UINT vk = LOBYTE(s_wHotKey);
    UINT flags = HIBYTE(s_wHotKey);

    UINT nMod = 0;
    if (flags & HOTKEYF_ALT)
        nMod |= MOD_ALT;
    if (flags & HOTKEYF_CONTROL)
        nMod |= MOD_CONTROL;
    if (flags & HOTKEYF_SHIFT)
        nMod |= MOD_SHIFT;

    UnregisterHotKey(hwnd, HOTKEY_ID);
    if (!RegisterHotKey(hwnd, HOTKEY_ID, nMod, vk))
    {
        MessageBoxW(hwnd, L"RegisterHotKey failed", NULL, MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

BOOL Settings_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)s_hIcon);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)s_hIconSm);

    SetDlgItemInt(hwnd, edt1, s_dwDelayToType, FALSE);
    SetDlgItemInt(hwnd, edt3, s_dwDelayToStart, FALSE);

    SendDlgItemMessage(hwnd, edt2, HKM_SETRULES, HKCOMB_A | HKCOMB_NONE | HKCOMB_S, 0);
    SendDlgItemMessage(hwnd, edt2, HKM_SETHOTKEY, s_wHotKey, 0);

    SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELONG(10000, 0));
    SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELONG(10000, 0));

    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    LPWSTR pch = wcsrchr(szPath, L'\\');
    *pch = 0;
    lstrcatW(szPath, L"\\*.wav");

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    WIN32_FIND_DATAW find;
    HANDLE hFind = FindFirstFile(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            ComboBox_AddString(hCmb1, find.cFileName);
        } while (FindNextFileW(hFind, &find));
        FindClose(hFind);
    }

    if (ComboBox_FindStringExact(hCmb1, -1, s_strSound.c_str()) == CB_ERR)
    {
        ComboBox_AddString(hCmb1, s_strSound.c_str());
    }
    ComboBox_SetText(hCmb1, s_strSound.c_str());

    CheckDlgButton(hwnd, chx1, s_bControlIME ? BST_CHECKED : BST_UNCHECKED);

    SetForegroundWindow(hwnd);
    return TRUE;
}

BOOL Settings_Delete(HWND hwnd)
{
    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_CURRENT_USER,
                  L"SOFTWARE\\Katayama Hirofumi MZ\\Typaste", 0,
                  KEY_READ,
                  &hKey);
    if (!hKey)
        return FALSE;

    RegDeleteKeyW(hKey, L"Delay");
    RegDeleteKeyW(hKey, L"HotKey");
    RegCloseKey(hKey);

    LONG nError = RegDeleteKeyW(HKEY_CURRENT_USER,
                                L"SOFTWARE\\Katayama Hirofumi MZ\\Typaste");
    return (ERROR_SUCCESS == nError);
}

BOOL Settings_Load(HWND hwnd)
{
    s_dwDelayToType = DEFAULT_DELAY;
    s_dwDelayToStart = 0;
    s_wHotKey = DEFALUT_HOTKEY;
    s_bControlIME = TRUE;

    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_CURRENT_USER,
                  L"SOFTWARE\\Katayama Hirofumi MZ\\Typaste", 0,
                  KEY_READ,
                  &hKey);
    if (!hKey)
        return FALSE;

    DWORD dwData, cbData;
    WCHAR szText[MAX_PATH];

    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"Delay", NULL, NULL, (LPBYTE)&dwData, &cbData))
    {
        s_dwDelayToType = dwData;
    }
    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"DelayToStart", NULL, NULL, (LPBYTE)&dwData, &cbData))
    {
        s_dwDelayToStart = dwData;
    }

    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"HotKey", NULL, NULL, (LPBYTE)&dwData, &cbData))
    {
        s_wHotKey = (WORD)dwData;
    }

    cbData = sizeof(szText);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"Sound", NULL, NULL, (LPBYTE)szText, &cbData))
    {
        StrTrimW(szText, L" \t\r\n");
        s_strSound = szText;
    }

    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"ControlIME", NULL, NULL, (LPBYTE)&dwData, &cbData))
    {
        s_bControlIME = !!dwData;
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOL Settings_Save(HWND hwnd)
{
    HKEY hCompanyKey = NULL;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ", 0, NULL, 0,
                    KEY_ALL_ACCESS, NULL, &hCompanyKey, NULL);
    if (!hCompanyKey)
        return FALSE;

    HKEY hAppKey = NULL;
    RegCreateKeyExW(hCompanyKey, L"Typaste", 0, NULL, 0,
                    KEY_ALL_ACCESS, NULL, &hAppKey, NULL);
    if (!hAppKey)
    {
        RegCloseKey(hCompanyKey);
        return FALSE;
    }

    RegSetValueExW(hAppKey, L"Delay", 0, REG_DWORD, (BYTE *)&s_dwDelayToType, sizeof(DWORD));
    RegSetValueExW(hAppKey, L"DelayToStart", 0, REG_DWORD, (BYTE *)&s_dwDelayToStart, sizeof(DWORD));
    RegSetValueExW(hAppKey, L"ControlIME", 0, REG_DWORD, (BYTE *)&s_bControlIME, sizeof(s_bControlIME));

    DWORD dwData = s_wHotKey;
    RegSetValueExW(hAppKey, L"HotKey", 0, REG_DWORD, (BYTE *)&dwData, sizeof(DWORD));

    DWORD cbData = (s_strSound.size() + 1) * sizeof(WCHAR);
    RegSetValueExW(hAppKey, L"Sound", 0, REG_SZ, (BYTE *)s_strSound.c_str(), cbData);

    RegCloseKey(hAppKey);
    RegCloseKey(hCompanyKey);
    return TRUE;
}

void Settings_OnOK(HWND hwnd)
{
    s_dwDelayToType = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
    s_dwDelayToStart = GetDlgItemInt(hwnd, edt3, NULL, FALSE);
    s_wHotKey = (WORD)SendDlgItemMessage(hwnd, edt2, HKM_GETHOTKEY, 0, 0);
    s_bControlIME = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);

    WCHAR szText[MAX_PATH];
    GetDlgItemText(hwnd, cmb1, szText, ARRAYSIZE(szText));
    StrTrimW(szText, L" \t\r\n");
    s_strSound = szText;

    Settings_Save(hwnd);

    EndDialog(hwnd, IDOK);
}

#ifndef ARRAYSIZE
    #define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

void Settings_OnPsh1(HWND hwnd)
{
    WCHAR szText[128];
    LoadStringW(NULL, IDS_DELETESETTINGS, szText, ARRAYSIZE(szText));

    if (IDOK == MessageBoxW(hwnd, szText, s_szName, MB_ICONINFORMATION))
    {
        Settings_Delete(hwnd);
        Settings_Load(hwnd);
        Settings_Save(hwnd);

        EndDialog(hwnd, psh1);
    }
}

void Settings_OnPsh2(HWND hwnd)
{
    OPENFILENAMEW ofn;
    WCHAR szPath[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.lpstrFilter = L"Sound File (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szPath;
    ofn.nMaxFile = ARRAYSIZE(szPath);
    ofn.lpstrTitle = L"Choose a sound file";
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                OFN_HIDEREADONLY | OFN_LONGNAMES;
    ofn.lpstrDefExt = L"wav";
    if (GetOpenFileNameW(&ofn))
    {
        SetDlgItemTextW(hwnd, cmb1, szPath);
    }
}

void Settings_OnPsh3(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    GetDlgItemTextW(hwnd, cmb1, szPath, _countof(szPath));
    PlaySoundW(szPath, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
}

void Settings_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        Settings_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case psh1:
        Settings_OnPsh1(hwnd);
        break;
    case psh2:
        Settings_OnPsh2(hwnd);
        break;
    case psh3:
        Settings_OnPsh3(hwnd);
        break;
    }
}

INT_PTR CALLBACK
SettingsDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Settings_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Settings_OnCommand);
    default:
        break;
    }
    return 0;
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    s_hwndMain = hwnd;

    Settings_Load(hwnd);
    MyRegisterHotKey(hwnd);

    SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)s_hIcon);
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)s_hIconSm);

    return TRUE;
}

void OnPsh2(HWND hwnd)
{
    WCHAR szPath[MAX_PATH], szText[64];

    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    LPWSTR pch = wcsrchr(szPath, L'\\');
    *pch = 0;

    LoadStringW(NULL, IDS_README, szText, 64);
    wcscat(szPath, szText);

    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        s_nExitCode = IDOK;
        DestroyWindow(hwnd);
        break;
    case IDCANCEL:
        s_nExitCode = IDCANCEL;
        DestroyWindow(hwnd);
        break;
    case psh1:
        s_nExitCode = psh1;
        DestroyWindow(hwnd);
        break;
    case psh2:
        OnPsh2(hwnd);
        break;
    }
}

void OnHotKey(HWND hwnd, int idHotKey, UINT fuModifiers, UINT vk)
{
    if (idHotKey != HOTKEY_ID)
        return;

    Sleep(s_dwDelayToStart);

    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) ||
        !OpenClipboard(hwnd))
    {
        UnregisterHotKey(hwnd, HOTKEY_ID);
        WaitModifierRelease(s_dwDelayToType);
        CtrlV(s_dwDelayToType);
        MyRegisterHotKey(hwnd);
        return;
    }

    LPWSTR pszClone = NULL;
    if (HGLOBAL hText = GetClipboardData(CF_UNICODETEXT))
    {
        if (LPWSTR pszText = (LPWSTR)GlobalLock(hText))
        {
            pszClone = _wcsdup(pszText);
            GlobalUnlock(hText);
        }
    }

    if (!pszClone)
    {
        UnregisterHotKey(hwnd, HOTKEY_ID);
        WaitModifierRelease(s_dwDelayToType);
        CtrlV(s_dwDelayToType);
        MyRegisterHotKey(hwnd);
        return;
    }

    CloseClipboard();

    WaitModifierRelease(s_dwDelayToType);

    WCHAR szSound[MAX_PATH];
    LPWSTR pch;
    LPCWSTR pszSound = s_strSound.c_str();
    if (PathIsRelative(pszSound))
    {
        GetModuleFileNameW(NULL, szSound, ARRAYSIZE(szSound));
        pch = wcsrchr(szSound, L'\\');
        if (!pch)
            pch = wcsrchr(szSound, L'/');
        if (pch)
        {
            *pch = 0;
            PathAppendW(szSound, pszSound);
        }
    }
    else
    {
        lstrcpynW(szSound, pszSound, ARRAYSIZE(szSound));
    }

#ifndef IMC_GETOPENSTATUS
    #define IMC_GETOPENSTATUS 0x0005
#endif
#ifndef IMC_SETOPENSTATUS
    #define IMC_SETOPENSTATUS 0x0006
#endif
    HWND hwndTarget = GetForegroundWindow();
    HWND hwndIme = ImmGetDefaultIMEWnd(hwndTarget);
    BOOL bImeOn = (BOOL)SendMessageW(hwndIme, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);

    if (s_bControlIME && hwndIme)
    {
        if (bImeOn)
        {
            SendMessageW(hwndIme, WM_IME_CONTROL, IMC_SETOPENSTATUS, FALSE);
            Sleep(300);
        }
    }

    AutoType(pszClone, s_dwDelayToType, szSound);

    if (s_bControlIME && hwndIme)
    {
        if (bImeOn)
        {
            SendMessageW(hwndIme, WM_IME_CONTROL, IMC_SETOPENSTATUS, TRUE);
            Sleep(300);
        }
    }

    free(pszClone);
}

void OnDestroy(HWND hwnd)
{
    Settings_Save(hwnd);
    PostQuitMessage(0);
}

#ifndef HANDLE_WM_HOTKEY
    #define HANDLE_WM_HOTKEY(hwnd,wParam,lParam,fn) \
        ((fn)((hwnd),(int)(wParam),(UINT)LOWORD(lParam),(UINT)HIWORD(lParam)),(LRESULT)0)
#endif

INT_PTR CALLBACK
MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_HOTKEY, OnHotKey);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    {
        WCHAR szText[64];
        LoadStringW(NULL, IDS_TITLETEXT, szText, ARRAYSIZE(szText));
        if (HWND hwnd = FindWindowW(L"#32770", szText))
        {
            /* already exists */
            SetForegroundWindow(hwnd);
            return 0;
        }
    }

    InitCommonControls();

    s_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    s_hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    for (;;)
    {
        HWND hwnd = CreateDialog(hInstance, MAKEINTRESOURCE(1), NULL,
                                 MainDialogProc);
        if (!hwnd)
        {
            MessageBoxW(NULL, L"CreateDialog failed", NULL, MB_ICONERROR);
            return -2;
        }

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);

        // message loop
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (IsDialogMessage(hwnd, &msg))
                continue;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (s_nExitCode != psh1)
            break;

        DialogBox(hInstance, MAKEINTRESOURCE(2), NULL, SettingsDialogProc);
    }

    return 0;
}
