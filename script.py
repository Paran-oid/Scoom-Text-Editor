import os

print(
    len("""#include <stdio.h>

int main(void) {
    printf("Hello World!\n");
    return 0;
}
"""))

path = "build/test.c"
size = os.path.getsize(path)

print(f"file size is {size}")
