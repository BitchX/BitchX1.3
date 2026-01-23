/*
 * win32_compat.h - Windows 11 compatibility layer for BitchX
 *
 * This header provides modern Windows API compatibility for the BitchX
 * IRC client, replacing legacy Winsock 1.1 and Win9x-era APIs with
 * their modern Windows 10/11 equivalents.
 *
 * Include this header BEFORE any other Windows headers.
 */

#ifndef BITCHX_WIN32_COMPAT_H
#define BITCHX_WIN32_COMPAT_H

#ifdef WINNT

/* Target Windows 10/11 APIs */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A00000B  /* Windows 11 (21H2) */
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif

/* Prevent winsock.h from being included (we use winsock2.h) */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

/* Modern Winsock 2 headers - MUST come before windows.h */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>

/* Windows 11 DPI and theming APIs */
#include <shellscalingapi.h>  /* Per-Monitor DPI */
#include <dwmapi.h>           /* Desktop Window Manager (dark mode, etc.) */
#include <uxtheme.h>          /* Visual styles */
#include <commctrl.h>         /* Common Controls 6.0 */
#include <shlobj.h>           /* Known folder paths */

/* Link required libraries */
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "comctl32.lib")

/*
 * BSD socket compatibility layer
 *
 * BitchX's network code uses Unix/BSD socket conventions.
 * Map them to their Winsock2 equivalents.
 */

/* Socket type compatibility */
typedef SOCKET bx_socket_t;
#define BX_INVALID_SOCKET INVALID_SOCKET
#define BX_SOCKET_ERROR   SOCKET_ERROR

/* close() -> closesocket() */
#ifndef close
#define close(s) closesocket(s)
#endif

/* ioctl -> ioctlsocket */
#ifndef ioctl
#define ioctl(s, cmd, argp) ioctlsocket(s, cmd, argp)
#endif

/* errno -> WSAGetLastError() for socket operations */
#define bx_socket_errno() WSAGetLastError()

/* Common errno values mapped to Winsock errors */
#ifndef EWOULDBLOCK
#define EWOULDBLOCK     WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS     WSAEINPROGRESS
#endif
#ifndef EALREADY
#define EALREADY        WSAEALREADY
#endif
#ifndef ENOTSOCK
#define ENOTSOCK        WSAENOTSOCK
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED    WSAECONNREFUSED
#endif
#ifndef ECONNRESET
#define ECONNRESET      WSAECONNRESET
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT       WSAETIMEDOUT
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH    WSAEHOSTUNREACH
#endif
#ifndef ENETUNREACH
#define ENETUNREACH     WSAENETUNREACH
#endif
#ifndef EADDRINUSE
#define EADDRINUSE      WSAEADDRINUSE
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL   WSAEADDRNOTAVAIL
#endif

/* Unix-style functions not available on Windows */
#ifndef sleep
#define sleep(s) Sleep((s) * 1000)
#endif

#ifndef usleep
#define usleep(us) Sleep((us) / 1000)
#endif

/* getpid on Windows */
#ifndef getpid
#define getpid() ((int)GetCurrentProcessId())
#endif

/* Signal handling compatibility */
#ifndef SIGHUP
#define SIGHUP    1
#endif
#ifndef SIGPIPE
#define SIGPIPE   13
#endif
#ifndef SIGALRM
#define SIGALRM   14
#endif
#ifndef SIGCHLD
#define SIGCHLD   17
#endif

/* Directory separator */
#define BX_PATH_SEPARATOR '\\'
#define BX_PATH_SEPARATOR_STR "\\"

/* snprintf / vsnprintf - available natively in MSVC 2015+ */
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

/* strcasecmp / strncasecmp */
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

/* Other POSIX compatibility */
#ifndef access
#define access _access
#endif
#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif

/*
 * Windows 11 Dark Mode Support
 *
 * Windows 11 introduced "Mica" and "Acrylic" backdrop materials,
 * and apps can respond to system dark/light theme preferences.
 */

/* DWM attribute for dark mode (Windows 10 20H1+ / Windows 11) */
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

/* DWM Mica/Acrylic backdrop (Windows 11 22H2+) */
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

/* Backdrop type values */
#define DWMSBT_DISABLE    1
#define DWMSBT_MAINWINDOW 2   /* Mica */
#define DWMSBT_TRANSIENT  3   /* Acrylic */
#define DWMSBT_TABBEDWINDOW 4 /* Tabbed Mica */

/* Window corner preference (Windows 11) */
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#define DWMWCP_DEFAULT    0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND      2
#define DWMWCP_ROUNDSMALL 3

/*
 * Winsock initialization/cleanup helpers
 */
static inline int bx_winsock_init(void)
{
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

static inline void bx_winsock_cleanup(void)
{
    WSACleanup();
}

/*
 * Windows 11 DPI helper functions
 */
static inline UINT bx_get_dpi_for_window(HWND hwnd)
{
    /* GetDpiForWindow is available on Windows 10 1607+ */
    return GetDpiForWindow(hwnd);
}

static inline int bx_scale_for_dpi(int value, UINT dpi)
{
    return MulDiv(value, dpi, 96);
}

/*
 * Dark mode detection
 */
static inline BOOL bx_is_dark_mode_enabled(void)
{
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(DWORD);

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL,
                         (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    /* AppsUseLightTheme == 0 means dark mode is ON */
    return (value == 0);
}

/*
 * Apply Windows 11 dark mode to a window
 */
static inline void bx_enable_dark_mode(HWND hwnd, BOOL enable)
{
    BOOL darkMode = enable;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &darkMode, sizeof(darkMode));
}

/*
 * Apply Windows 11 rounded corners
 */
static inline void bx_set_window_corners(HWND hwnd, int preference)
{
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                          &preference, sizeof(preference));
}

/*
 * Apply Windows 11 Mica backdrop
 */
static inline void bx_set_mica_backdrop(HWND hwnd)
{
    int backdrop = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                          &backdrop, sizeof(backdrop));
}

/*
 * Get user's known folder path (replacement for SHGetFolderPath)
 * Returns a newly allocated string that must be freed with CoTaskMemFree()
 */
static inline PWSTR bx_get_appdata_path(void)
{
    PWSTR path = NULL;
    SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, NULL, &path);
    return path;
}

#endif /* WINNT */

#endif /* BITCHX_WIN32_COMPAT_H */
