// AutoType.cpp
// Copyright (C) 2019-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "Typaste.hpp"

// Emulate a key press or release
static inline void MyKeybdEvent(WORD wVk, WORD wScan, DWORD dwFlags, ULONG_PTR dwExtra)
{
#if (WINVER < 0x0500) // Old tech
    keybd_event(wVk, wScan, dwFlags, dwExtra);
#else
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = wVk;
    input.ki.wScan = wScan;
    input.ki.dwFlags = dwFlags;
    input.ki.time = 0;
    input.ki.dwExtraInfo = dwExtra;
    SendInput(1, &input, sizeof(input));
#endif
}

// Wait for release of modifier keys
void WaitModifierRelease(DWORD dwDelay)
{
    while (GetAsyncKeyState(VK_SHIFT) < 0 ||
           GetAsyncKeyState(VK_CONTROL) < 0 ||
           GetAsyncKeyState(VK_MENU) < 0)
    {
        Sleep(dwDelay);
    }
}

// Emulate key press and release
void EmulateKey(BYTE vk, BYTE flags, DWORD dwDelay)
{
    // Emulate the modifier keys press
    if (flags & HOTKEYF_SHIFT)
    {
        MyKeybdEvent(VK_LSHIFT, 0, 0, 0);
        Sleep(dwDelay);
    }
    if (flags & HOTKEYF_CONTROL)
    {
        MyKeybdEvent(VK_LCONTROL, 0, 0, 0);
        Sleep(dwDelay);
    }
    if (flags & HOTKEYF_ALT)
    {
        MyKeybdEvent(VK_LMENU, 0, 0, 0);
        Sleep(dwDelay);
    }

    // Emulate main key press
    MyKeybdEvent(vk, 0, 0, 0);
    Sleep(dwDelay);

    // Emulate main key release
    MyKeybdEvent(vk, 0, KEYEVENTF_KEYUP, 0);
    Sleep(dwDelay);

    // Emulate the modifier keys release (in reverse order)
    if (flags & HOTKEYF_ALT)
    {
        MyKeybdEvent(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
        Sleep(dwDelay);
    }
    if (flags & HOTKEYF_CONTROL)
    {
        MyKeybdEvent(VK_LCONTROL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(dwDelay);
    }
    if (flags & HOTKEYF_SHIFT)
    {
        MyKeybdEvent(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
        Sleep(dwDelay);
    }
}

// Emulate Ctrl+V or something
void CtrlV(DWORD dwDelay)
{
    EmulateKey('V', HOTKEYF_CONTROL, dwDelay);
}

// Switch the keyboard layout by pressing [Shift]+[Alt]
void SwitchKL(DWORD dwDelay)
{
    MyKeybdEvent(VK_LSHIFT, 0, 0, 0);
    Sleep(dwDelay);
    MyKeybdEvent(VK_LMENU, 0, 0, 0);
    Sleep(dwDelay);
    MyKeybdEvent(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
    Sleep(dwDelay);
    MyKeybdEvent(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
    Sleep(dwDelay);
}

// The auto typing function
void AutoType(LPCTSTR psz, DWORD dwDelay, LPCTSTR pszSound)
{
    const UINT cMaxKL = 10; // Who uses 10 keyboards in a moment?

    // Get the list of the keyboard layouts
    HKL ahKLs[cMaxKL];
    UINT chkl = GetKeyboardLayoutList(cMaxKL, ahKLs);

    // Remember the old keyboard layout
    HKL hOldKL = GetKeyboardLayout(0);

    // The typing loop
    for (; *psz; ++psz)
    {
        // Quit the loop if the user press Esc key
        if (GetAsyncKeyState(VK_ESCAPE) < 0)
            break;

        // Handle special characters
        switch (*psz)
        {
        case L'\r': // CR
            continue;
        case L'\n': // LF
            // Emulate Enter key
            MyKeybdEvent(VK_RETURN, 0, 0, 0);
            Sleep(dwDelay);
            MyKeybdEvent(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
            Sleep(dwDelay);
            continue;
        case L'\t': // TAB
            // Emulate Tab key
            MyKeybdEvent(VK_TAB, 0, 0, 0);
            Sleep(dwDelay);
            MyKeybdEvent(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
            Sleep(dwDelay);
            continue;
        }

        // Play the sound if necessary
        if (pszSound && *pszSound)
        {
            PlaySound(pszSound, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
            Sleep(dwDelay);
        }

        // Translate the character to a virtual key
        SHORT s = -1;
        UINT iKL;
        for (iKL = 0; iKL < chkl; ++iKL)
        {
            s = VkKeyScanEx(*psz, ahKLs[iKL]);
            if (s != -1)
                break; // Success!
        }

        // Choose the prefer keyboard layout
        HKL hKL;
        if (s != -1)
        {
            for (INT k = 0; k < cMaxKL; ++k)
            {
                hKL = GetKeyboardLayout(0);
                if (hKL == ahKLs[iKL])
                    break; // The keyboard layout does match

                // Switch the keyboard layout by pressing [Shift]+[Alt]
                SwitchKL(dwDelay);
            }
        }

        if (s == -1) // Unable to translate
        {
            // Send WM_CHAR message to the foreground focus control
            HWND hwnd = GetForegroundWindow();
            DWORD dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
            GUITHREADINFO info = { sizeof(info) };
            GetGUIThreadInfo(dwThreadId, &info);
            DWORD_PTR dwResult;
            SendMessageTimeout(info.hwndFocus, WM_CHAR, *psz, 0, SMTO_ABORTIFHUNG, 2000, &dwResult);
            Sleep(dwDelay);
        }
        else
        {
            BYTE vk = LOBYTE(s); // The translated virtual key code
            BYTE flags = HIBYTE(s); // The modifier flags
            // Emulate the key press and release
            EmulateKey(vk, flags, dwDelay);
        }
    }

    // Restore the keyboard layout
    for (INT k = 0; k < cMaxKL; ++k)
    {
        HKL hKL = GetKeyboardLayout(0);
        if (hKL == hOldKL)
            break; // The keyboard layout does match

        // Switch the keyboard layout by pressing [Shift]+[Alt]
        SwitchKL(dwDelay);
    }
}
