/*
 * Copyright (c) 2023, Baran Avci
 *
 * Under the MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

//VT100 escape sequences
#define DEL 127
#define clear_cursor_down() fputs("\033[J", stdout)
#define save_cursor_state() fputs("\0337", stdout)
#define restore_cursor_state() fputs("\0338", stdout)
#define move_cursor_to(x, y) printf("\033[%d;%dH", y, x)

#define IS_TRAILING_BYTE(b) ((b & 0xc0) == 0x80)

static struct {
    int width;
    int height;
} terminal;

static struct {
    int x;
    int y;
} cursor;

static void get_cursor_pos(int *x, int *y)
{
    char buf[16] = {0};
    fputs("\033[6n", stdout);
    fflush(stdout);

    int ch = 0;
    for (int i = 0; i < 15 && ch != 'R'; ++i)
    {
        //TODO: replace with a std libc function
        read(STDIN_FILENO, &ch, 1);
        buf[i] = ch;
    }
    *y = atoi(&buf[2]);
    *x = atoi(&buf[strcspn(buf, ";")+1]);
}

static void get_term_size(void)
{
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    terminal.width = ws.ws_col;
    terminal.height = ws.ws_row;
}

static void move_left(void)
{
    if (cursor.x == 1 && terminal.height != 1)
    {
        cursor.x = terminal.width;
        cursor.y--;
        return;
    }

    cursor.x--;
}

static void move_right(void)
{
    if (cursor.x == terminal.width)
    {
        if (cursor.y == terminal.height)
        {
            putchar('\n');
            restore_cursor_state();
            int new_x = 0, new_y = 0;
            get_cursor_pos(&new_x, &new_y);
            move_cursor_to(new_x, new_y-1);
            save_cursor_state();
            fflush(stdout);
            cursor.y--;
        }
        cursor.x = 1;
        cursor.y++;
        return;
    }

    cursor.x++;
}

static inline void re_pos_cursor(size_t line_size)
{
    restore_cursor_state();
    get_cursor_pos(&cursor.x, &cursor.y);
    for (size_t i = 0; i < line_size; ++i)
        move_right();
}

static inline size_t utf8len(const char *str)
{
    size_t len = 0;
    while (*str)
    {
        if (!IS_TRAILING_BYTE(*str))
            len++;
        str++;
    }
    return len;
}

static volatile sig_atomic_t skip = false;
static volatile sig_atomic_t re_render = false;

static void sig_handler(int sig)
{
    if (sig == SIGINT)
        skip = true;
    else if (sig == SIGWINCH)
        re_render = true;
}

static struct termios new_set, old_set;

static void init_deadline(void)
{
    static bool inited = false;
    if (!inited)
    {
        struct sigaction sigact = {
            .sa_handler = sig_handler,
            .sa_flags = 0
        };

        sigaction(SIGINT, &sigact, NULL);
        sigaction(SIGWINCH, &sigact, NULL);

        tcgetattr(STDIN_FILENO, &old_set);
        new_set = old_set;
        new_set.c_lflag &= (~ICANON & ~ECHO);
        inited = true;
    }

    tcsetattr(STDIN_FILENO, TCSADRAIN, &new_set);
}

static void write_line(const char *prompt, const char *buffer, size_t size)
{
    restore_cursor_state();
    clear_cursor_down();
    fputs(prompt, stdout);
    fwrite(buffer, 1, size, stdout);
    move_cursor_to(cursor.x, cursor.y);
}

char *readline(const char *prompt)
{
    size_t prompt_len = utf8len(prompt);
    char *buffer = malloc(256);
    if (buffer == NULL) return NULL;
    size_t size = 256;
    size_t pos = 0;
    size_t diff = 0;

    init_deadline();

    save_cursor_state();
    fputs(prompt, stdout);
    get_cursor_pos(&cursor.x, &cursor.y);
    while (!feof(stdin))
    {
        int ch = getchar();
        get_term_size();

        if (skip)
        {
            skip = false;
            fputs("^C", stdout);
            buffer[0] = '\0';
            break;
        }
        else if (re_render)
        {
            re_render = false;
            re_pos_cursor(utf8len(buffer)+prompt_len);
            write_line(prompt, buffer, pos);
            continue;
        }

        if (ch == DEL)
        {
            if (!(pos-diff)) continue;

            move_left();

            size_t rm_amount = 1;
            while (IS_TRAILING_BYTE(buffer[pos-diff-rm_amount]))
                rm_amount++;

            for (int i = 0; i < rm_amount; ++i)
            {
                pos--;
                for (size_t i = pos-diff; i < pos; ++i)
                    buffer[i] = buffer[i+1];
            }
            write_line(prompt, buffer, pos);
            continue;
        }
        else if (ch == '\033')
        {
            char esc[3] = {0};
            fread(esc, 1, 2, stdin);
            if (esc[1] == 'C' && (pos-diff) < pos)
            {
                diff--;
                while (buffer[pos-diff] && IS_TRAILING_BYTE(buffer[pos-diff]))
                    diff--;
                move_right();
            }
            else if (esc[1] == 'D' && (pos-diff) != 0)
            {
                diff++;
                while (IS_TRAILING_BYTE(buffer[pos-diff]))
                    diff++;
                move_left();
            }
            move_cursor_to(cursor.x, cursor.y);
            continue;
        }

        if (!IS_TRAILING_BYTE(ch))
            move_right(); //for added characters

        if (ch == '\n') break;

        for (size_t i = pos; pos-diff < i; --i)
            buffer[i] = buffer[i-1];
        buffer[pos-diff] = ch;
        pos++;
        if (pos == size)
        {
            size <<= 1;
            buffer = realloc(buffer, size);
            if (buffer == NULL) return NULL;
        }

        write_line(prompt, buffer, pos);
    }
    buffer[pos] = '\0';
    putchar('\n');

    tcsetattr(STDIN_FILENO, TCSANOW, &old_set);
    return buffer;
}
