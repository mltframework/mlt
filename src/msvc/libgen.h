#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdlib.h>
#include <windows.h>

/* Structure to hold path information */
struct path_info {
    /* This points to end of the UNC prefix and drive letter, if any. */
    char* prefix_end;

    /* These point to the directory separator in front of the last non-empty component. */
    char* base_sep_begin;
    char* base_sep_end;

    /* This points to the last directory separator sequence if no other non-separator characters follow it. */
    char* term_sep_begin;

    /* This points to the end of the string. */
    char* path_end;
};

/* Macro to check if a character is a directory separator */
#define IS_DIR_SEP(c) ((c) == '/' || (c) == '\\')

/* Function to extract directory name from a path */
char* dirname(char* path);

/* Function to extract base name from a path */
char* basename(char* path);
