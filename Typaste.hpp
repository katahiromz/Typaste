#include "targetver.h"
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tchar.h>
#include <string>
#include "AutoType.hpp"
#include "resource.h"

// Backward compatibility for BCC55
#ifndef _countof
    #define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

// The generic string class
#ifdef UNICODE
    typedef std::wstring tstring_t;
#else
    typedef std::string tstring_t;
#endif

// The generic text clipboard format
#ifdef UNICODE
    #define CF_GENERICTEXT CF_UNICODETEXT
#else
    #define CF_GENERICTEXT CF_TEXT
#endif
