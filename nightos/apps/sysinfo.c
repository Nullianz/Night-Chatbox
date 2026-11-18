#include "night.h"
static const char *days[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
int main(void) {
    char buf[256], name[32];
    DWORD len = sizeof name;
    SYSTEMTIME t;
    GetLocalTime(&t);
    GetComputerNameA(name, &len);
    color(0x0B); print("System information\n"); color(0x07);
    wsprintfA(buf, "  Computer : %s\n  Date     : %s %04u-%02u-%02u %02u:%02u:%02u\n  Uptime   : %u ms\n",
              name, days[t.wDayOfWeek % 7], t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, GetTickCount());
    print(buf);
    /* heap test */
    HANDLE h = GetProcessHeap();
    int ok = 1;
    for (int i = 0; i < 100; i++) {
        unsigned char *p = HeapAlloc(h, HEAP_ZERO_MEMORY, 1000 + i * 37);
        if (!p || p[500]) { ok = 0; break; }
        p[999] = (unsigned char)i;
        HeapFree(h, 0, p);
    }
    wsprintfA(buf, "  Heap     : 100 alloc/free cycles %s\n", ok ? "passed" : "FAILED");
    print(buf);
    DWORD t0 = GetTickCount();
    Sleep(250);
    wsprintfA(buf, "  Sleep    : asked for 250 ms, got %u ms\n", GetTickCount() - t0);
    print(buf);
    return 0;
}
