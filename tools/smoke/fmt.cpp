// Checks printf %c handling of the C library (libnix vs clib2).
#include <stdio.h>
#include <string.h>
int main()
{
    char buf[64];
    char c = 'G';
    sprintf(buf, "SC%c%02d%c%c.INI", c, 1, 'E', 'A');
    FILE* f = fopen("RAM:fmt.txt", "w");
    if (f) {
        fprintf(f, "result='%s' len=%d\n", buf, (int)strlen(buf));
        fclose(f);
    }
    return 0;
}
