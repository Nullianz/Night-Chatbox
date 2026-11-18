#include "night.h"
int main(void) {
    MessageBeep(0);
    int r = MessageBoxA(0, "This box was opened by a Windows program\ncalling MessageBoxA from user32.dll.\n\nDo you like NightOS?",
                        "Hello!", MB_YESNO | MB_ICONINFORMATION);
    MessageBoxA(0, r == IDYES ? "Thanks! :)" : "We'll keep working on it.", "Your answer", MB_OK);
    print(r == IDYES ? "You clicked Yes.\n" : "You clicked No.\n");
    return 0;
}
