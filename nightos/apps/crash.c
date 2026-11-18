#include "night.h"
int main(void) {
    print("About to divide by zero on purpose...\n");
    volatile int zero = 0;
    volatile int x = 42 / zero;
    print("This line never runs.\n");
    return x;
}
