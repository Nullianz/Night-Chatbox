#include "night.h"
int main(void) {
    char line[200], buf[256];
    DWORD w, n;
    print("Type your notes. An empty line saves them to notes.txt.\n");
    HANDLE f = CreateFileA("notes.txt", GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
    if (f == INVALID_HANDLE_VALUE) { print("Could not open notes.txt\n"); return 1; }
    SetFilePointer(f, 0, 0, FILE_END);
    for (;;) {
        color(0x0B); print("note> "); color(0x07);
        int len = read_line(line, sizeof line);
        if (!len) break;
        line[len++] = '\n';
        WriteFile(f, line, (DWORD)len, &w, 0);
    }
    CloseHandle(f);
    f = CreateFileA("notes.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    DWORD size = GetFileSize(f, 0);
    char *data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
    ReadFile(f, data, size, &n, 0);
    CloseHandle(f);
    wsprintfA(buf, "Saved. notes.txt is now %u bytes:\n", size);
    print(buf);
    color(0x0E); print(data); color(0x07);
    HeapFree(GetProcessHeap(), 0, data);
    return 0;
}
