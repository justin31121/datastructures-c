#include <stdio.h>

#define UI_WINDOWS
#define UI_IMPLEMENTATION
#include "../thirdparty/luigi2.h"

int main() {
    bool maximize = true;
    UIWindow *windowMain = UIWindowCreate(0, maximize ? UI_WINDOW_MAXIMIZE : 0, "demo", 0, 0);
    return 0;
}
