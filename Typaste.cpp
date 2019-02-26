#include "stdafx.hpp"

#define DEFAULT_DELAY 20
#define DEFALUT_HOTKEY MAKEWORD('V', HOTKEYF_CONTROL)
#define HOTKEY_ID 0xDEDD

HWND s_hwndMain = NULL;
DWORD s_dwDelay = DEFAULT_DELAY;
WORD s_wHotKey = DEFALUT_HOTKEY;
HICON s_hIcon = NULL;
HICON s_hIconSm = NULL;
INT s_nExitCode = IDCANCEL;

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

    SetDlgItemInt(hwnd, edt1, s_dwDelay, FALSE);

    SendDlgItemMessage(hwnd, ctl1, HKM_SETRULES, HKCOMB_A | HKCOMB_NONE | HKCOMB_S, 0);
    SendDlgItemMessage(hwnd, ctl1, HKM_SETHOTKEY, s_wHotKey, 0);

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
    s_dwDelay = DEFAULT_DELAY;
    s_wHotKey = DEFALUT_HOTKEY;

    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_CURRENT_USER,
                  L"SOFTWARE\\Katayama Hirofumi MZ\\Typaste", 0,
                  KEY_READ,
                  &hKey);
    if (!hKey)
        return FALSE;

    DWORD dwData, cbData;
    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"Delay", NULL, NULL, (LPBYTE)&dwData, &cbData))
    {
        s_dwDelay = dwData;
    }

    cbData = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"HotKey", NULL, NULL, (LPBYTE)&dwData, &cbData))
    {
        s_wHotKey = (WORD)dwData;
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

    RegSetValueExW(hAppKey, L"Delay", 0, REG_DWORD, (BYTE *)&s_dwDelay, sizeof(DWORD));

    DWORD dwData = s_wHotKey;
    RegSetValueExW(hAppKey, L"HotKey", 0, REG_DWORD, (BYTE *)&dwData, sizeof(DWORD));

    RegCloseKey(hAppKey);
    RegCloseKey(hCompanyKey);
}

void Settings_OnOK(HWND hwnd)
{
    s_dwDelay = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
    s_wHotKey = (WORD)SendDlgItemMessage(hwnd, ctl1, HKM_GETHOTKEY, 0, 0);

    Settings_Save(hwnd);

    EndDialog(hwnd, IDOK);
}

void Settings_OnPsh1(HWND hwnd)
{
    WCHAR szText[128];
    LoadString(GetModuleHandle(NULL), 1, szText, ARRAYSIZE(szText));

    if (IDOK == MessageBoxW(hwnd, szText, s_szName, MB_ICONINFORMATION))
    {
        Settings_Delete(hwnd);
        Settings_Load(hwnd);
        Settings_Save(hwnd);

        EndDialog(hwnd, psh1);
    }
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
    }
}

void OnHotKey(HWND hwnd, int idHotKey, UINT fuModifiers, UINT vk)
{
    if (idHotKey != HOTKEY_ID)
        return;

    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) ||
        !OpenClipboard(hwnd))
    {
        UnregisterHotKey(hwnd, HOTKEY_ID);
        WaitModifierRelease(s_dwDelay);
        CtrlV(s_dwDelay);
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
        WaitModifierRelease(s_dwDelay);
        CtrlV(s_dwDelay);
        MyRegisterHotKey(hwnd);
        return;
    }

    CloseClipboard();

    WaitModifierRelease(s_dwDelay);
    AutoType(pszClone, s_dwDelay);
    free(pszClone);
}

void OnDestroy(HWND hwnd)
{
    Settings_Save(hwnd);
    PostQuitMessage(0);
}

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
