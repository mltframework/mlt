/*
 * io.c -- melt input/output
 * Copyright (C) 2002-2026 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* System header files */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#else
// MinGW defines struct timespec in pthread.h
#include <pthread.h>
// for nanosleep()
#include <framework/mlt_types.h>
#include <windows.h>
#endif

/* Application header files */
#include "io.h"

char *chomp(char *input)
{
    if (input != NULL) {
        int length = strlen(input);
        if (length && input[length - 1] == '\n')
            input[length - 1] = '\0';
        if (length > 1 && input[length - 2] == '\r')
            input[length - 2] = '\0';
    }
    return input;
}

char *trim(char *input)
{
    if (input != NULL) {
        int length = strlen(input);
        int first = 0;
        while (first < length && isspace(input[first]))
            first++;
        memmove(input, input + first, length - first + 1);
        length = length - first;
        while (length > 0 && isspace(input[length - 1]))
            input[--length] = '\0';
    }
    return input;
}

char *strip_quotes(char *input)
{
    if (input != NULL) {
        char *ptr = strrchr(input, '\"');
        if (ptr != NULL)
            *ptr = '\0';
        if (input[0] == '\"')
            strcpy(input, input + 1);
    }
    return input;
}

int *get_int(int *output, int use)
{
    int *value = NULL;
    char temp[132];
    *output = use;
    if (trim(chomp(fgets(temp, 132, stdin))) != NULL) {
        if (strcmp(temp, ""))
            *output = atoi(temp);
        value = output;
    }
    return value;
}

/** This stores the previous settings
*/

#ifndef _WIN32
static struct termios oldtty;
#else
static DWORD oldtty;
#endif
static int mode = 0;

/** This is called automatically on application exit to restore the 
	previous tty settings.
*/

void term_exit(void)
{
    if (mode == 1) {
#ifndef _WIN32
        tcsetattr(0, TCSANOW, &oldtty);
#else
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
        if (h) {
            SetConsoleMode(h, oldtty);
        }
#endif
        mode = 0;
    }
}

/** Init terminal so that we can grab keys without blocking.
*/

void term_init()
{
#ifndef _WIN32
    struct termios tty;

    tcgetattr(0, &tty);
    oldtty = tty;

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag |= OPOST;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
    tty.c_cflag &= ~(CSIZE | PARENB);
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    tcsetattr(0, TCSANOW, &tty);
#else
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD tty = 0;
    if (!h || h == INVALID_HANDLE_VALUE || !GetConsoleMode(h, &tty))
        return;
    oldtty = tty;
    SetConsoleMode(h, tty & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
#endif

    mode = 1;

    atexit(term_exit);
}

/** Check for a keypress without blocking infinitely.
	Returns: ASCII value of keypress or -1 if no keypress detected.
*/

int term_read()
{
#ifndef _WIN32
    int n = 1;
    unsigned char ch;
    struct timeval tv;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 40000;
    n = select(1, &rfds, NULL, NULL, &tv);
    if (n > 0) {
        n = read(0, &ch, 1);
        tcflush(0, TCIFLUSH);
        if (n == 1)
            return ch;
        return n;
    }
#else
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD rec;
    DWORD count = 0;

    /* ReadConsole blocks while the handle is signaled for a non-key record
       (focus, mouse, menu). melt then never reaches its SDL event loop, so
       playback-window resize and keyboard transport run only in the progress
       paths, which skip this read. Poll and return -1 when no key is waiting. */
    if (h && h != INVALID_HANDLE_VALUE && GetFileType(h) == FILE_TYPE_CHAR) {
        while (PeekConsoleInput(h, &rec, 1, &count) && count > 0) {
            if (!ReadConsoleInput(h, &rec, 1, &count) || count == 0)
                break;
            if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
                unsigned char ch = rec.Event.KeyEvent.uChar.AsciiChar;
                if (ch)
                    return ch;
            }
        }
    }
    {
        struct timespec tm = {0, 40000000};
        nanosleep(&tm, NULL);
    }
    return -1;
#endif
    return -1;
}

char get_keypress()
{
    char value = '\0';
    int pressed = 0;

    fflush(stdout);

    term_init();
    while ((pressed = term_read()) == -1)
        ;
    term_exit();

    value = (char) pressed;

    return value;
}

void wait_for_any_key(char *message)
{
    if (message == NULL)
        printf("Press any key to continue: ");
    else
        printf("%s", message);

    get_keypress();

    printf("\n\n");
}

void beep()
{
    printf("%c", 7);
    fflush(stdout);
}
