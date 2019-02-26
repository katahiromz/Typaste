// AutoType
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#include "stdafx.hpp"

static inline void MyKeybdEvent(WORD wVk, WORD wScan, DWORD dwFlags, ULONG_PTR dwExtra)
{
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = wVk;
    input.ki.wScan = wScan;
    input.ki.dwFlags = dwFlags;
    input.ki.time = 0;
    input.ki.dwExtraInfo = dwExtra;
    SendInput(1, &input, sizeof(input));
}

void WaitModifierRelease(DWORD dwDelay)
{
    while (GetAsyncKeyState(VK_SHIFT) < 0 ||
           GetAsyncKeyState(VK_CONTROL) < 0 ||
           GetAsyncKeyState(VK_MENU) < 0)
    {
        Sleep(dwDelay);
    }
}

void CtrlV(DWORD dwDelay)
{
    MyKeybdEvent(VK_LCONTROL, 0, 0, 0);
    Sleep(dwDelay);

    MyKeybdEvent(L'V', 0, 0, 0);
    Sleep(dwDelay);

    MyKeybdEvent(L'V', 0, KEYEVENTF_KEYUP, 0);
    Sleep(dwDelay);

    MyKeybdEvent(VK_LCONTROL, 0, KEYEVENTF_KEYUP, 0);
    Sleep(dwDelay);
}

void AutoType(const WCHAR *psz, DWORD dwDelay)
{
    const UINT cMaxKL = 10;
    HKL ahkl[cMaxKL];
    UINT chkl = GetKeyboardLayoutList(cMaxKL, ahkl);
    HKL hklOld = GetKeyboardLayout(0);

    for (; *psz ;++psz)
    {
        if (GetAsyncKeyState(VK_ESCAPE) < 0)
            break;

        switch (*psz)
        {
        case L'\r':
            continue;
        case L'\n':
            MyKeybdEvent(VK_RETURN, 0, 0, 0);
            Sleep(dwDelay);
            MyKeybdEvent(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
            Sleep(dwDelay);
            continue;
        case L'\t':
            MyKeybdEvent(VK_TAB, 0, 0, 0);
            Sleep(dwDelay);
            MyKeybdEvent(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
            Sleep(dwDelay);
            continue;
        }

        SHORT s = -1;
        UINT i;
        for (i = 0; i < chkl; ++i)
        {
            s = VkKeyScanExW(*psz, ahkl[i]);
            if (s != -1)
                break;
        }

        HKL hkl = NULL;
        if (s != -1)
        {
            for (INT k = 0; k < cMaxKL; ++k)
            {
                hkl = GetKeyboardLayout(0);
                if (hkl == ahkl[i])
                    break;

                MyKeybdEvent(VK_LSHIFT, 0, 0, 0);
                Sleep(dwDelay);
                MyKeybdEvent(VK_LMENU, 0, 0, 0);
                Sleep(dwDelay);
                MyKeybdEvent(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
                Sleep(dwDelay);
                MyKeybdEvent(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
                Sleep(dwDelay);
            }
        }

        BYTE vk = LOBYTE(s);
        BYTE flags = HIBYTE(s);

        if (s == -1)
        {
            HWND hwnd = GetForegroundWindow();
            DWORD tid = GetWindowThreadProcessId(hwnd, NULL);
            GUITHREADINFO info = { sizeof(info) };
            GetGUIThreadInfo(tid, &info);
            DWORD_PTR dwResult;
            SendMessageTimeoutW(info.hwndFocus, WM_CHAR, *psz, 0,
                                SMTO_ABORTIFHUNG, 2000, &dwResult);
            Sleep(dwDelay);
        }
        else
        {
            if (flags & 1)
            {
                MyKeybdEvent(VK_LSHIFT, 0, 0, 0);
                Sleep(dwDelay);
            }
            if (flags & 2)
            {
                MyKeybdEvent(VK_LCONTROL, 0, 0, 0);
                Sleep(dwDelay);
            }
            if (flags & 4)
            {
                MyKeybdEvent(VK_LMENU, 0, 0, 0);
                Sleep(dwDelay);
            }

            MyKeybdEvent(vk, 0, 0, 0);
            Sleep(dwDelay);
            MyKeybdEvent(vk, 0, KEYEVENTF_KEYUP, 0);
            Sleep(dwDelay);

            if (flags & 4)
            {
                MyKeybdEvent(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
                Sleep(dwDelay);
            }
            if (flags & 2)
            {
                MyKeybdEvent(VK_LCONTROL, 0, KEYEVENTF_KEYUP, 0);
                Sleep(dwDelay);
            }
            if (flags & 1)
            {
                MyKeybdEvent(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
                Sleep(dwDelay);
            }
        }
    }

    HKL hkl;
    for (INT k = 0; k < cMaxKL; ++k)
    {
        HKL hkl = GetKeyboardLayout(0);
        if (hkl == hklOld)
            break;

        MyKeybdEvent(VK_LSHIFT, 0, 0, 0);
        Sleep(dwDelay);
        MyKeybdEvent(VK_LMENU, 0, 0, 0);
        Sleep(dwDelay);
        MyKeybdEvent(VK_LMENU, 0, KEYEVENTF_KEYUP, 0);
        Sleep(dwDelay);
        MyKeybdEvent(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
        Sleep(dwDelay);
    }
}
