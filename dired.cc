/*
**
** dired - a directory editor modelled after GNU Emacs Dired mode.
**
** Written in C++ using the termcap\(3\) library
**
** dired.C 1.47  Delta\'d: 14:27:39 10/5/92  Mike Lijewski, CNSF
**
** Copyright \(c\) 1991, 1992 Cornell University
** All rights reserved.
**
** Redistribution and use in source and binary forms are permitted
** provided that: \(1\) source distributions retain this entire copyright
** notice and comment, and \(2\) distributions including binaries display
** the following acknowledgement:  ``This product includes software
** developed by Cornell University\'\' in the documentation or other
** materials provided with the distribution and in all advertising
** materials mentioning features or use of this software. Neither the
** name of the University nor the names of its contributors may be used
** to endorse or promote products derived from this software without
** specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED ``AS IS\'\' AND WITHOUT ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef _IBMR2
#include <libc.h>
#endif
#include <new.h>
#include <osfcn.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#ifdef __EMX__
#include <io.h>
#include <fcntl.h>
#define chdir _chdir2
#endif

#include "dired.h"
#include "display.h"

/*
** exception handler for new\(\) - called once in main\(\)
*/

static void free_store_exception()
{
    error("exiting, memory exhausted, sorry");
}

/*
** dired - Edit the given directory. We must have read and execute
**         permission to edit a directory; calling routines must
**         guarantee this.
*/

void dired(const char *dirname)
{
    if (chdir(dirname) < 0)
        error("File %s, line %d: couldn't chdir() to `%s'",
              __FILE__, __LINE__, dirname);

    char *fullname = current_directory();
    if (fullname == 0)
        error("File %s, line %d: current_directory() failed on `%s'.",
              __FILE__, __LINE__, dirname);

    DirList *dir_list = get_directory_listing(fullname);
    if (dir_list == 0)
        error("File %s, line %d: couldn't read directory `%s'",
              __FILE__, __LINE__, fullname);

    //
    // We track the CWD and PWD variables, if they\'re defined, so that
    // applications such as emacs which use them will work properly.
    //
    if (getenv("CWD"))
    {
        static String str;
        static String ostr;
        str = String("CWD=") + fullname;
        if (putenv(str) < 0)
            error("File %s, line %d: putenv(%s) failed.",
                  __FILE__, __LINE__, fullname);
        ostr = str;
    }

    if (getenv("PWD"))
    {
        static String str;
        static String ostr;
        str = String("PWD=") + fullname;
        if (putenv(str) < 0)
            error("File %s, line %d: putenv(%s) failed.",
                  __FILE__, __LINE__, fullname);
        ostr = str;
    }

    dir_stack->push(dir_list);
    initial_listing(dir_list);
    update_modeline(modeline_prefix, fullname);
    dir_list->saveYXPos(0, goal_column(dir_list));

    if (dir_list->currLine()->length() > columns())    
        leftshift_current_line(dir_list);
    else
        move_cursor(dir_list->savedYPos(), dir_list->savedXPos());

    synch_display();

    read_commands(dir_list);  // main command loop
}

int main(int argc, char *argv[])
{
    char *dirname;

#ifdef __EMX__
    _wildcard(&argc, &argv);
    _response(&argc, &argv);
    setmode(1, O_BINARY);
#endif

    //
    // Process options - the only options we accept are -t -u or -c.
    //
    if (argc < 2)
        // edit current directory
        dirname = ".";
    else
    {
        while(**++argv == '-')
        {
            if (strcmp(*argv, "-t") == 0)
            {
                set_sort_order(MODIFICATION_TIME);
                continue;
            }
            if (strcmp(*argv, "-u") == 0)
            {
                set_sort_order(ACCESS_TIME);
                continue;
            }
            if (strcmp(*argv, "-c") == 0)
            {
                set_sort_order(INODE_CHANGE_TIME);
                continue;
            }
        }
        dirname = *argv ? *argv : ".";
    }

    if (!isatty(0) || !isatty(1))
    {
        (void)fputs("stdin & stdout must be terminals\n", stderr);
        exit(EXIT_FAILURE);
    }

    //
    // If you don\'t have SIGINTERRUPT then signals almost surely interrupt
    // read\(2\).  If you do have it, you\'ll need this to ensure that signals
    // interrupt slow system calls -- we\'re only interested in read\(2\).
    //
#if defined(SIGINTERRUPT) && defined(SIGTSTP)
    if (siginterrupt(SIGTSTP, 1) < 0 || siginterrupt(SIGWINCH, 1) < 0)
    {
        perror("siginterrupt()");
        exit(1);
    }
#endif

    set_new_handler(free_store_exception);
    init_display();
    set_signals();

    if (!is_directory(dirname))
        error("File %s, line %d: `%s' isn't a directory",
              __FILE__, __LINE__, dirname);

    if (!read_and_exec_perm(dirname))
        error("File %s, line %d: need read & exec permission to edit `%s'",
              __FILE__, __LINE__, dirname);

    dir_stack = new DirStack();

    dired(dirname);

    return 0;
}
