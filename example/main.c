#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <deadline.h>

int main(void)
{
    bool quit = false;
    while (!quit)
    {
        char *buffer = readline(">> ");

        if (!strcmp(buffer, "exit"))
            quit = true;

        free(buffer);
    }

    return 0;
}
