#include "night.h"
static int to_int(const char *s) { int v = 0; while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0'); return v; }
int main(void) {
    char line[32], buf[128];
    SetConsoleTitleA("Guess the Number");
    DWORD seed = GetTickCount() * 2654435761u;
    int secret = (int)(seed >> 16) % 100 + 1, tries = 0;
    color(0x0E); print("I'm thinking of a number from 1 to 100.\n"); color(0x07);
    for (;;) {
        print("Your guess (or q to quit): ");
        if (!read_line(line, sizeof line) || line[0] == 'q') { print("Bye!\n"); return 0; }
        int g = to_int(line); tries++;
        if (g < secret) print("Higher...\n");
        else if (g > secret) print("Lower...\n");
        else {
            color(0x0A);
            wsprintfA(buf, "Correct! It was %d. You got it in %d tries.\n", secret, tries);
            print(buf); color(0x07);
            return 0;
        }
    }
}
