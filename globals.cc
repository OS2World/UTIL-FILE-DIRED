/*
** globals.C - global definitions
**
** globals.C 1.3   Delta\'d: 14:50:08 9/22/92   Mike Lijewski, CNSF
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

#include "dired.h"
#include "version.h"

// the stack of directories being edited
DirStack *dir_stack;

// The default sort order is ALPHABETICALLY.
sort_order how_to_sort = ALPHABETICALLY;

//
// commands to get long directory listings:
//
//    ls_cmd\[0\] gives alphabetical listing
//    ls_cmd\[1\] gives listing sorted by modification time
//    ls_cmd\[2\] gives listing sorted by access time
//    ls_cmd\[3\] gives listing sorted by inode-change time
//
const char *const ls_cmd[4] =
#ifdef L_AND_G
  { "ls -agl ", "ls -aglt ", "ls -agltu ", "ls -acglt " };
#else
  { "ls -al ", "ls -alt ", "ls -altu ", "ls -aclt " };
#endif

// the modeline prefix
const char *const modeline_prefix = "----- Dired: ";

// the help messages
const char *const help_file[] = {
    " CURSOR MOVEMENT COMMANDS:",
    "",
    "    ?  H               Display this help.",
    "    q                  Back up directory tree if possible, else quit.",
    "    Q                  Exit immediately.",
    "    j  n  ^N  SPC  CR  Forward  one line.",
    "    DOWN_ARROW_KEY             \"        .",
    "    k  p  ^P  ^Y       Backward one line.",
    "    UP_ARROW_KEY               \"        .",
    "    ^F  ^V             Forward  one window.",
    "    b  ^B  ESC-V       Backward one window.",
    "    ^D                 Forward  one half-window.",
    "    ^U                 Backward one half-window.",
    "    <                  Go to first line of listing.",
    "    >                  Go to last line of listing.",
    "    /                  Search forward for string.",
    "    \\                  Search backward for string.",
    "",
    " COMMANDS WHICH OPERATE ON THE CURRENT FILE:",
    "",
    "    c                  Copy current file - prompts for destination file.",
    "    d                  Delete current file - prompts for affirmation.",
    "    e  f               Edit the current file with $EDITOR (default `vi').",
    "                       or the current directory with `dired'.",
    "    m  v               View current file with $PAGER (default `more').",
    "    r                  Rename current file.",
    "    C                  Compress current file.",
    "    E                  Prompt for and edit a directory.",
    "    G                  Change the group of the current file.",
    "    L                  Link current file to another file.",
    "    M                  Change the mode of the current file.",
    "    P                  Print current file with $DIREDPRT (default `lpr').",
    "    O                  Prompt for a new sorting order (a, c, t, or u).",
    "    R  g               Rereads the current directory and updates the display.",
#ifndef NO_SYMLINKS
    "    S                  Create symbolic link to current file.",
#endif
    "    U                  Uncompress current file.",
    "",
    " MISCELLANEOUS COMMANDS:",
    "",
    "    !                  starts up a shell.",
    "    ! cmd              executes a shell command - prompts for command.",
    "                       A `%' in \"cmd\" is replaced by the current",
    "                       filename before execution.",
    "    !!                 reexecutes previous shell command.",
    "    ![                 reexecutes previous shell command, reexpanding",
    "                       any `%' to the now current file name.",
    "    ^L                 Repaint screen.",
    "    ^G                 Abort from a prompt.",
    "    CR                 Signifies end-of-response when in a prompt.",
    "    V                  Print out version string."
};

// number of entries in help_file
const int HELP_FILE_DIM = int(sizeof(help_file) / sizeof(help_file[0]));

// has window size changed -- should really be a sig_atomic_t
int win_size_changed;
