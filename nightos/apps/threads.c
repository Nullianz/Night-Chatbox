#include "night.h"
/* Uses a function NightOS doesn't implement yet, to show how that's handled. */
static DWORD WINAPI worker(void *p) { (void)p; return 0; }
int main(void) {
    print("Starting a thread...\n");
    CreateThread(0, 0, (void *)worker, 0, 0, 0);
    print("Thread started.\n");
    return 0;
}
