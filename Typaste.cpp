// Typaste.cpp --- Typing + Paste
// Copyright (C) 2019-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "Typaste.hpp"

// The default delay value (in milliseconds)
#define DEFAULT_DELAY 20

// The default hot-key value
#define DEFALUT_HOTKEY MAKEWORD('V', HOTKEYF_CONTROL) // Ctrl+V

// The hot-key ID. It must be a WORD
#define HOTKEY_ID 0xDEDD

// The global variables
HINSTANCE   g_hInst             = NULL;            // The instance handle of this application
HWND        g_hwndMain          = NULL;            // The main window handle
DWORD       g_dwDelayToType     = DEFAULT_DELAY;   // The delay to type (in milliseconds)
DWORD       g_dwDelayToStart    = 0;               // The delay to start (in milliseconds)
WORD        g_wHotKey           = DEFALUT_HOTKEY;  // The hot-key calue
HICON       g_hIcon             = NULL;            // The application main icon (normal)
HICON       g_hIconSm           = NULL;            // The application main icon (small)
INT         g_nExitCode         = IDCANCEL;        // The exit code
BOOL        g_bControlIME       = TRUE;            // Does the application control IME?
TCHAR       g_szNone[64]        = TEXT("(None)");  // "(None)"
tstring_t   g_strSound;                            // The sound filename

// Register a hot-key
BOOL MyRegisterHotKey(HWND hwnd)
{
    UINT vk = LOBYTE(g_wHotKey), flags = HIBYTE(g_wHotKey);

    // Convert HOTKEYF_* flags to MOD_* flags
    UINT nMod = 0;
    if (flags & HOTKEYF_ALT)
        nMod |= MOD_ALT;
    if (flags & HOTKEYF_CONTROL)
        nMod |= MOD_CONTROL;
    if (flags & HOTKEYF_SHIFT)
        nMod |= MOD_SHIFT;

    // Un-register now
    UnregisterHotKey(hwnd, HOTKEY_ID);

    // Register the hot-key
    if (RegisterHotKey(hwnd, HOTKEY_ID, nMod, vk))
        return TRUE; // Success!

    // Failed. Show error message
    TCHAR szText[MAX_PATH];
    LoadString(g_hInst, IDS_FAILHOTKEY, szText, _countof(szText));
    MessageBox(hwnd, szText, NULL, MB_ICONERROR);

    return FALSE; // Failed
}

// WM_INITDIALOG
BOOL Settings_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Set the main icon
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSm);

    // Set the delay values
    SetDlgItemInt(hwnd, edt1, g_dwDelayToType, FALSE);
    SetDlgItemInt(hwnd, edt3, g_dwDelayToStart, FALSE);

    // Set hot-key value
    SendDlgItemMessage(hwnd, edt2, HKM_SETRULES, HKCOMB_A | HKCOMB_NONE | HKCOMB_S, 0);
    SendDlgItemMessage(hwnd, edt2, HKM_SETHOTKEY, g_wHotKey, 0);

    // Set value range
    SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELONG(10000, 0));
    SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELONG(10000, 0));

    // Build a wildcard pattern for WAV files
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, _countof(szPath));
    PathRemoveFileSpec(szPath);
    PathAppend(szPath, TEXT("*.wav"));

    // Load "(None)"
    LoadString(g_hInst, IDS_NONE, g_szNone, _countof(g_szNone));

    // Add "(None)" to the combo box
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    ComboBox_AddString(hCmb1, g_szNone);

    // Add the items to the combo box
    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            ComboBox_AddString(hCmb1, find.cFileName);
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    // Add g_strSound if any
    if (ComboBox_FindStringExact(hCmb1, -1, g_strSound.c_str()) == CB_ERR)
    {
        ComboBox_AddString(hCmb1, g_strSound.c_str());
    }
    // Set g_strSound
    ComboBox_SetText(hCmb1, g_strSound.c_str());

    // Control IME?
    CheckDlgButton(hwnd, chx1, g_bControlIME ? BST_CHECKED : BST_UNCHECKED);

    // Set foreground
    SetForegroundWindow(hwnd);

    return TRUE; // Default focus
}

#define APP_REGKEY TEXT("SOFTWARE\\Katayama Hirofumi MZ\\Typaste")

// Erase settings
BOOL Settings_Delete(HWND hwnd)
{
    LONG nError = RegDeleteKey(HKEY_CURRENT_USER, APP_REGKEY);
    return (ERROR_SUCCESS == nError);
}

// Load settings
BOOL Settings_Load(HWND hwnd)
{
    // Load the default values
    g_dwDelayToType = DEFAULT_DELAY;
    g_dwDelayToStart = 0;
    g_wHotKey = DEFALUT_HOTKEY;
    g_bControlIME = TRUE;
    LoadString(g_hInst, IDS_NONE, g_szNone, _countof(g_szNone));
    g_strSound = g_szNone;

    // Open the application registry key
    HKEY hKey = NULL;
    RegOpenKeyEx(HKEY_CURRENT_USER, APP_REGKEY, 0, KEY_READ, &hKey);
    if (!hKey)
        return FALSE;

    //
    // Set the registry values
    //
    DWORD dwData, cbData;
    TCHAR szText[MAX_PATH];
    LSTATUS error;

    cbData = sizeof(DWORD);
    error = RegQueryValueEx(hKey, TEXT("Delay"), NULL, NULL, (LPBYTE)&dwData, &cbData);
    if (!error)
    {
        g_dwDelayToType = dwData;
    }
    cbData = sizeof(DWORD);
    error = RegQueryValueEx(hKey, TEXT("DelayToStart"), NULL, NULL, (LPBYTE)&dwData, &cbData);
    if (!error)
    {
        g_dwDelayToStart = dwData;
    }

    cbData = sizeof(DWORD);
    error = RegQueryValueEx(hKey, TEXT("HotKey"), NULL, NULL, (LPBYTE)&dwData, &cbData);
    if (!error)
    {
        g_wHotKey = (WORD)dwData;
    }

    cbData = sizeof(szText);
    error = RegQueryValueEx(hKey, TEXT("Sound"), NULL, NULL, (LPBYTE)szText, &cbData);
    if (!error)
    {
        StrTrim(szText, TEXT(" \t\r\n"));
        g_strSound = szText;
        if (g_strSound.empty())
            g_strSound = g_szNone;
    }

    cbData = sizeof(DWORD);
    error = RegQueryValueEx(hKey, TEXT("ControlIME"), NULL, NULL, (LPBYTE)&dwData, &cbData);
    if (!error)
    {
        g_bControlIME = !!dwData;
    }

    // Close the registry key
    RegCloseKey(hKey);
    return TRUE;
}

// Save settings
BOOL Settings_Save(HWND hwnd)
{
    // Create or open the application registry key
    HKEY hAppKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, APP_REGKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hAppKey, NULL);
    if (!hAppKey)
        return FALSE;

    //
    // Set the registry values
    //
    RegSetValueEx(hAppKey, TEXT("Delay"), 0, REG_DWORD, (BYTE *)&g_dwDelayToType, sizeof(DWORD));
    RegSetValueEx(hAppKey, TEXT("DelayToStart"), 0, REG_DWORD, (BYTE *)&g_dwDelayToStart, sizeof(DWORD));
    RegSetValueEx(hAppKey, TEXT("ControlIME"), 0, REG_DWORD, (BYTE *)&g_bControlIME, sizeof(g_bControlIME));

    DWORD dwData = g_wHotKey;
    RegSetValueEx(hAppKey, TEXT("HotKey"), 0, REG_DWORD, (BYTE *)&dwData, sizeof(dwData));

    DWORD cbData = (g_strSound.size() + 1) * sizeof(TCHAR);
    RegSetValueEx(hAppKey, TEXT("Sound"), 0, REG_SZ, (BYTE *)g_strSound.c_str(), cbData);

    // Close the registry key
    RegCloseKey(hAppKey);

    return TRUE;
}

// IDD_SETTINGS WM_COMMAND IDOK
void Settings_OnOK(HWND hwnd)
{
    //
    // Update settings from dialog
    //
    g_dwDelayToType = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
    g_dwDelayToStart = GetDlgItemInt(hwnd, edt3, NULL, FALSE);
    g_wHotKey = (WORD)SendDlgItemMessage(hwnd, edt2, HKM_GETHOTKEY, 0, 0);
    g_bControlIME = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);

    TCHAR szText[MAX_PATH];
    GetDlgItemText(hwnd, cmb1, szText, _countof(szText));
    StrTrim(szText, TEXT(" \t\r\n"));
    g_strSound = szText;

    // Save settings
    Settings_Save(hwnd);

    // End the settings dialog
    EndDialog(hwnd, IDOK);
}

// IDD_SETTINGS WM_COMMAND psh2
void Settings_OnBrowse(HWND hwnd)
{
    // Load title
    TCHAR szTitle[256];
    LoadString(g_hInst, IDS_CHOOSESOUNDFILE, szTitle, _countof(szTitle));

    // Build the filter string
    TCHAR szFilter[MAX_PATH];
    LoadString(g_hInst, IDS_SOUNDFILTER, szFilter, _countof(szFilter));
    for (LPTSTR pch = szFilter; *pch; ++pch)
    {
        if (*pch == TEXT('|'))
        {
            *pch = 0;
            continue;
        }
#ifndef UNICODE
        if (IsDBCSLeadByte(*pch))
            ++pch;
#endif
    }

    // Open the "Open file" dialog and wait for the user
    OPENFILENAME ofn;
    TCHAR szPath[MAX_PATH] = TEXT("");
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szPath;
    ofn.nMaxFile = _countof(szPath);
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                OFN_HIDEREADONLY | OFN_LONGNAMES;
    ofn.lpstrDefExt = TEXT("wav");
    if (GetOpenFileName(&ofn))
    {
        // Set the path
        SetDlgItemText(hwnd, cmb1, szPath);
    }
}

// IDD_SETTINGS WM_COMMAND psh3
void Settings_OnPlay(HWND hwnd)
{
    TCHAR szPath[MAX_PATH];
    GetDlgItemText(hwnd, cmb1, szPath, _countof(szPath));
    PlaySound(szPath, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
}

// IDD_SETTINGS WM_COMMAND
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
    case psh2:
        Settings_OnBrowse(hwnd);
        break;
    case psh3:
        Settings_OnPlay(hwnd);
        break;
    }
}

// The dialog procedure of settings dialog IDD_SETTINGS
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

// IDD_MAIN WM_INITDIALOG
BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Remember the main window handle
    g_hwndMain = hwnd;

    // Load settings
    Settings_Load(hwnd);

    // Register the hot-key
    MyRegisterHotKey(hwnd);

    // Set the icons to the dialog
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSm);

    return TRUE; // Default focus
}

// IDD_MAIN WM_COMMAND psh2
void OnOpenReadMe(HWND hwnd)
{
    // Load ReadMe filename
    TCHAR szText[64];
    LoadString(g_hInst, IDS_README, szText, _countof(szText));

    // Build a path for "ReadMe.txt"
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    PathRemoveFileSpec(szPath);
    PathAppend(szPath, szText);

    // Open "ReadMe.txt"
    ShellExecute(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// IDD_MAIN WM_COMMAND
void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    // hwnd is a modeless dialog. We use DestroyWindow instead of EndDialog for this hwnd.
    switch (id)
    {
    case IDOK:
        g_nExitCode = IDOK;
        DestroyWindow(hwnd);
        break;
    case IDCANCEL:
        g_nExitCode = IDCANCEL;
        DestroyWindow(hwnd);
        break;
    case psh1:
        g_nExitCode = psh1;
        DestroyWindow(hwnd);
        break;
    case psh2:
        OnOpenReadMe(hwnd);
        break;
    }
}

#ifdef UNICODE
    #define CF_GENERICTEXT CF_UNICODETEXT
#else
    #define CF_GENERICTEXT CF_TEXT
#endif

// Normal Ctrl+V or something
void NormalCtrlV(HWND hwnd)
{
    // Un-register the hot-key
    UnregisterHotKey(hwnd, HOTKEY_ID);

    // Wait for release of modifier keys
    WaitModifierRelease(g_dwDelayToType);

    // Press Ctrl+V or something
    CtrlV(g_dwDelayToType);

    // Re-register the hot-key
    MyRegisterHotKey(hwnd);
}

// IDD_MAIN WM_HOTKEY
void OnHotKey(HWND hwnd, int idHotKey, UINT fuModifiers, UINT vk)
{
    // Sanity check
    if (idHotKey != HOTKEY_ID)
        return;

    // Wait for start
    Sleep(g_dwDelayToStart);

    // Is the clipboard text available?
    if (!IsClipboardFormatAvailable(CF_GENERICTEXT) || !OpenClipboard(hwnd))
    {
        // Text data in clipboard is not available.
        NormalCtrlV(hwnd);
        return;
    }

    // Get the clipbard text
    LPTSTR pszClone = NULL;
    if (HGLOBAL hText = GetClipboardData(CF_GENERICTEXT))
    {
        if (LPTSTR pszText = (LPTSTR)GlobalLock(hText))
        {
            pszClone = _tcsdup(pszText);
            GlobalUnlock(hText);
        }
    }

    // Close the clipboard
    CloseClipboard();

    if (!pszClone)
    {
        // Text data in clipboard is not available.
        NormalCtrlV(hwnd);
        return;
    }

    // Wait for release of modifier keys
    WaitModifierRelease(g_dwDelayToType);

    // Prepare for szSoundFile
    TCHAR szSoundFile[MAX_PATH];
    LPCTSTR pszSound = g_strSound.c_str();
    if (g_strSound == g_szNone)
    {
        szSoundFile[0] = 0;
    }
    else if (PathIsRelative(pszSound))
    {
        GetModuleFileName(NULL, szSoundFile, _countof(szSoundFile));
        PathRemoveFileSpec(szSoundFile);
        PathAppend(szSoundFile, pszSound);
    }
    else
    {
        lstrcpyn(szSoundFile, pszSound, _countof(szSoundFile));
    }

#ifndef IMC_GETOPENSTATUS
    #define IMC_GETOPENSTATUS 0x0005
#endif
#ifndef IMC_SETOPENSTATUS
    #define IMC_SETOPENSTATUS 0x0006
#endif
    // This is the target window
    HWND hwndTarget = GetForegroundWindow();

    // Is there an IME window?
    HWND hwndIme = ImmGetDefaultIMEWnd(hwndTarget);

    // Is the IME ON?
    BOOL bImeOn = (BOOL)SendMessage(hwndIme, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);

    // Close IME if necessary
    if (g_bControlIME && hwndIme && bImeOn)
    {
        SendMessage(hwndIme, WM_IME_CONTROL, IMC_SETOPENSTATUS, FALSE);
        Sleep(300);
    }

    // Start auto typing
#ifndef UNICODE
    #error ANSI version is not supported yet.
#endif
    AutoType(pszClone, g_dwDelayToType, szSoundFile);

    // Re-open IME if necessary
    if (g_bControlIME && hwndIme && bImeOn)
    {
        SendMessage(hwndIme, WM_IME_CONTROL, IMC_SETOPENSTATUS, TRUE);
        Sleep(300);
    }

    // Clean up
    free(pszClone);
}

// IDD_MAIN WM_DESTROY
void OnDestroy(HWND hwnd)
{
    Settings_Save(hwnd);
    PostQuitMessage(0);
}

#ifndef HANDLE_WM_HOTKEY
    #define HANDLE_WM_HOTKEY(hwnd,wParam,lParam,fn) \
        ((fn)((hwnd),(int)(wParam),(UINT)LOWORD(lParam),(UINT)HIWORD(lParam)),(LRESULT)0)
#endif

// The main dialog IDD_MAIN procedure
INT_PTR CALLBACK
MainDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_HOTKEY, OnHotKey);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    default:
        break;
    }
    return 0;
}

// The main function of Win32 application
INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    // Remember the application instance
    g_hInst = hInstance;

    // Find the application main window if any
    TCHAR szText[MAX_PATH];
    LoadString(g_hInst, IDS_TITLETEXT, szText, _countof(szText));
    if (HWND hwnd = FindWindow(TEXT("#32770"), szText))
    {
        /* Already exists. Move focus to there */
        SetForegroundWindow(hwnd);
        return 0;
    }

    // Initialize common controls
    InitCommonControls();

    // Load main icons
    g_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    g_hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON,
                                 GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    // The loop
    for (;;)
    {
        // Create the main dialog (as a modeless dialog)
        HWND hwnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialogProc);
        if (!hwnd)
        {
            TCHAR szText[MAX_PATH];
            LoadString(g_hInst, IDS_FAILDIALOG, szText, _countof(szText));
            MessageBox(NULL, szText, NULL, MB_ICONERROR);
            return -2;
        }

        // Show it now
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);

        // The message loop
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            // Process the modeless dialog message
            if (IsDialogMessage(hwnd, &msg))
                continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_nExitCode != psh1)
            break; // Exit now

        // Show settings dialog (as a modal dialog)
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), NULL, SettingsDialogProc);
    }

    return 0;
}
