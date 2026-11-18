#include "night.h"
int main(void) {
    char buf[256];
    static const WORD rainbow[] = {0x0C, 0x0E, 0x0A, 0x0B, 0x09, 0x0D};
    const char *msg = "Hello from a Windows .exe running on NightOS!";
    for (int i = 0; msg[i]; i++) {
        char c[2] = {msg[i], 0};
        color(rainbow[i % 6]);
        print(c);
    }
    color(0x07);
    wsprintfA(buf, "\nCommand line: %s\nLoaded at address 0x%08X, uptime %u ms\n",
              GetCommandLineA(), (DWORD)GetModuleHandleA(0), GetTickCount());
    print(buf);
    return 0;
}
