/* Minimal Win32 declarations for NightOS demo programs (written from the public API docs). */
#ifndef NIGHT_WIN32_H
#define NIGHT_WIN32_H
#define WINAPI __stdcall
#define IMPORT __declspec(dllimport)
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef int BOOL;
typedef void *HANDLE;
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_END 2
#define HEAP_ZERO_MEMORY 8
#define MB_OK 0
#define MB_YESNO 4
#define MB_ICONINFORMATION 0x40
#define IDYES 6
typedef struct { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME;

IMPORT void WINAPI ExitProcess(DWORD);
IMPORT HANDLE WINAPI GetStdHandle(DWORD);
IMPORT BOOL WINAPI WriteFile(HANDLE, const void *, DWORD, DWORD *, void *);
IMPORT BOOL WINAPI ReadFile(HANDLE, void *, DWORD, DWORD *, void *);
IMPORT BOOL WINAPI ReadConsoleA(HANDLE, void *, DWORD, DWORD *, void *);
IMPORT BOOL WINAPI SetConsoleTextAttribute(HANDLE, WORD);
IMPORT BOOL WINAPI SetConsoleTitleA(const char *);
IMPORT char *WINAPI GetCommandLineA(void);
IMPORT DWORD WINAPI GetTickCount(void);
IMPORT void WINAPI Sleep(DWORD);
IMPORT HANDLE WINAPI CreateFileA(const char *, DWORD, DWORD, void *, DWORD, DWORD, HANDLE);
IMPORT BOOL WINAPI CloseHandle(HANDLE);
IMPORT DWORD WINAPI GetFileSize(HANDLE, DWORD *);
IMPORT DWORD WINAPI SetFilePointer(HANDLE, long, long *, DWORD);
IMPORT DWORD WINAPI GetLastError(void);
IMPORT HANDLE WINAPI GetProcessHeap(void);
IMPORT void *WINAPI HeapAlloc(HANDLE, DWORD, DWORD);
IMPORT BOOL WINAPI HeapFree(HANDLE, DWORD, void *);
IMPORT HANDLE WINAPI GetModuleHandleA(const char *);
IMPORT void WINAPI GetLocalTime(SYSTEMTIME *);
IMPORT BOOL WINAPI GetComputerNameA(char *, DWORD *);
IMPORT int WINAPI lstrlenA(const char *);
IMPORT int WINAPI MessageBoxA(HANDLE, const char *, const char *, DWORD);
IMPORT BOOL WINAPI MessageBeep(DWORD);
IMPORT int __cdecl wsprintfA(char *, const char *, ...);
IMPORT HANDLE WINAPI CreateThread(void *, DWORD, void *, void *, DWORD, DWORD *);

/* tiny helpers */
static HANDLE out_h;
__attribute__((unused)) static void print(const char *s) {
    DWORD w;
    if (!out_h) out_h = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(out_h, s, (DWORD)lstrlenA(s), &w, 0);
}
__attribute__((unused)) static void color(WORD c) { if (!out_h) out_h = GetStdHandle(STD_OUTPUT_HANDLE); SetConsoleTextAttribute(out_h, c); }
__attribute__((unused)) static int read_line(char *buf, int max) {
    DWORD n = 0;
    ReadFile(GetStdHandle(STD_INPUT_HANDLE), buf, (DWORD)max - 1, &n, 0);
    while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) n--;
    buf[n] = 0;
    return (int)n;
}
#endif
