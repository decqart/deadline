#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <read_line.h>

int main(void)
{
    bool quit = false;
    while (!quit)
    {
        printf(">> ");
        char *buffer = read_line();

        if (!strcmp(buffer, "exit"))
            quit = true;

        free(buffer);
    }
    exit_read_line();
    return 0;
}
