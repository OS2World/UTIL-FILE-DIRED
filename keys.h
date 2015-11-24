/*
** keys.h - contains definitions of all the keyboard keys which
**          invoke commands.
**
** keys.h 1.22   Delta'd: 15:10:54 9/22/92   Mike Lijewski, CNSF
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

#ifndef __KEYS_H
#define __KEYS_H

const int KEY_q       = 'q';   // pop previous directory off stack
const int KEY_Q       = 'Q';   // quit
const int KEY_CTL_D   = 0x4;   // forward half window - ASCII CTL-D
const int KEY_CTL_U   = 0x15;  // backward half window - ASCII CTL-U
const int KEY_TOP     = '<';   // go to first file in listing
const int KEY_BOT     = '>';   // go to last file in listing
const int KEY_CTL_L   = '\f';  // repaint screen - CTR-L
const int KEY_TAB     = '\t';  // TAB performs filename completion
const int KEY_c       = 'c';   // copy current file
const int KEY_d       = 'd';   // delete current file w/affirmation
const int KEY_E       = 'E';   // prompt for a directory and edit it
const int KEY_r       = 'r';   // rename current file
const int KEY_C       = 'C';   // compress current file
const int KEY_G       = 'G';   // change group of current file
const int KEY_L       = 'L';   // link current file to another
const int KEY_M       = 'M';   // change mode of current file
const int KEY_P       = 'P';   // print current file
const int KEY_S       = 'S';   // symbolically link current file to another
const int KEY_U       = 'U';   // uncompress current file
const int KEY_V       = 'V';   // print out version string
const int KEY_BKSP    = '\b';  // backspace works as expected while in a prompt
const int KEY_SLASH   = '/';   // search forward
const int KEY_BKSLASH = '\\';  // search backward
const int KEY_BANG    = '!';   // run shell command
const int KEY_O       = 'O';   // change sort order
const int KEY_ESC     = 0x1B;  // for GNU Emacs compatibility - ASCII-ESC

// display help
const int KEY_QM = '?';
const int KEY_H  = 'H';

// forward one line
const int KEY_j          = 'j';
const int KEY_n          = 'n';
const int KEY_CTL_N      = 0xE;   // ASCII CTL-N
const int KEY_SPC        = ' ';   // a space
const int KEY_CR         = '\r';  // carriage return
const int KEY_ARROW_DOWN = 300;

// backward one line
const int KEY_k        = 'k';
const int KEY_p        = 'p';
const int KEY_CTL_P    = 0x10; // ASCII CTL-P
const int KEY_CTL_Y    = 0x19; // ASCII CTL-Y
const int KEY_ARROW_UP = 301;  // an arbitrary value

// forward one window
const int KEY_CTL_F = 6;    // ASCII CTL-F
const int KEY_CTL_V = 0x16; // ASCII CTL-V

// backward one window
const int KEY_b     = 'b';
const int KEY_CTL_B = 0x2;  // ASCII CTL-B
const int KEY_CTL_Z = 0x1A; // ASCII CTL-Z

// abort from a prompt - CTL-G
//
// Can't use '\a' here due to some C compilers not recognizing this
// as the terminal bell.
//
const int KEY_ABORT = 0x7;

// edit current file or directory
const int KEY_e = 'e';
const int KEY_f = 'f';

// view current file
const int KEY_m = 'm';
const int KEY_v = 'v';

// reread current directory
const int KEY_g = 'g';
const int KEY_R = 'R';

// for simulating SIGINT and SIGQUIT
const int KEY_INT  = 0x3;  // ASCII CTL-C
const int KEY_QUIT = 0x1C; // ASCII CTL-\

#endif /* __KEYS_H */

extern int          get_key(char *key);
