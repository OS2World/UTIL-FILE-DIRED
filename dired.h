/*
** dired.h - global declarations
**
** dired.h 1.50   Delta'd: 17:32:47 10/1/92   Mike Lijewski, CNSF
**
** Copyright (c) 1991, 1992 Cornell University
** All rights reserved.
**
** Redistribution and use in source and binary forms are permitted
** provided that: (1) source distributions retain this entire copyright
** notice and comment, and (2) distributions including binaries display
** the following acknowledgement:  ``This product includes software
** developed by Cornell University'' in the documentation or other
** materials provided with the distribution and in all advertising
** materials mentioning features or use of this software. Neither the
** name of the University nor the names of its contributors may be used
** to endorse or promote products derived from this software without
** specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef __DIRED_H
#define __DIRED_H

#include <stdio.h>
#include <string.h>

//
// Take care of compilers which can't deal with 'delete []' form of delete.
//
#ifdef  OLDDELETE
#define DELETE delete
#else
#define DELETE delete []
#endif

#include "classes.h"

// the four ways we sort a directory listing
enum sort_order { ALPHABETICALLY, MODIFICATION_TIME,
                  ACCESS_TIME,    INODE_CHANGE_TIME };

//
// GLOBALS
//
extern DirStack *dir_stack;                 // our directory stack
extern sort_order how_to_sort;              // default sort order
extern const char *const ls_cmd[4];         // command to get long listing
extern const char *const modeline_prefix;   // modeline prefix
extern const char *const version;           // our version number
extern const char *const help_file[];       // the help message
extern const int HELP_FILE_DIM;             // number of entries in help_file
extern int win_size_changed;                // has window size changed

// for POSIX compatibility
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

//
// FUNCTION PROTOTYPES
//

// from `commands.C'
extern void read_commands(DirList *);

// from `dired.C'
extern void dired(const char *dirname);

// from `utilities.C'
extern void        cleanup(int);
extern char       *current_directory();
extern void        display_string(const char *str, size_t len = 0); 
extern void        eat_a_character(const char *msg);
extern void        error(const char *str);
extern void        error(const char *fmt, int index);
extern void        error(const char *fmt, const char *file, int line);
extern void        error(const char *fmt, const char *file, int line,
                         const char *str);
extern void        exec_with_system(const char *cmd, int prompt = 1);
extern int         execute(const char *file, const char *argv[], int closem=1);
extern const char *expand_tilde(const char *str);
extern char       *fgetline(FILE *fp);
extern DirList    *get_directory_listing(char *dirname);
extern const char *get_file_name(DirList *list);
extern int         get_key(DirList *dl);
extern int         goal_column(DirList *l);
extern void        initialize();
extern void        initial_listing(DirList *dl);
extern int         is_directory(const char *dir);
extern int         is_regular_file(const char *file);
extern void        leftshift_current_line(DirList *dl);
extern int         lines_displayed(DirList *dl);
extern void        message(const char *fmt, const char *str = 0);
#ifdef COMPLETION
extern const char *prompt(const char *msg, int do_completion = 0);
#else
extern const char *prompt(const char *msg);
#endif
extern int         read_and_exec_perm(const char *dir);
extern int         read_from_keybd();
extern void        redisplay();
extern void        rightshift_current_line(DirList *dl);
extern void        set_signals();
#ifdef NO_STRCHR
extern char       *strchr(const char *s, char charwanted);
extern char       *strrchr(const char *s, char charwanted);
#endif
#ifdef NO_STRSTR
extern char       *strstr(const char *, const char *);
#endif
extern void        update_modeline(const char * = 0, const char * = 0);
extern void        unset_signals();
extern void        winch(int);

//
// INLINES
//

// max - the maximum of two integer arguments.
inline int max(int x, int y) { return x >= y ? x : y; }

// set_sort_order - sets the sorting order.
inline void set_sort_order(sort_order order) { how_to_sort = order; }

// the_sort_order - returns the sorting order
inline sort_order the_sort_order(void) { return how_to_sort; }

//
// MISCELLANY
//

// a number of g++ implementations seem not to have this defined
#if defined(__GNUG__) && !defined(NO_STRSTR)
extern "C" {
	extern char *strstr(const char *, const char *);
}
#endif

#endif /* __DIRED_H */
