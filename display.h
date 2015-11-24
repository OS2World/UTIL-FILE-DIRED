/*
** external definitions needed for interfacing with display.C
**
** display.h 1.15   Delta'd: 15:10:49 9/22/92   Mike Lijewski, CNSF
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

#ifndef __DISPLAY_H
#define __DISPLAY_H

//
// termcap capabilities we'll try to use
//
extern char *AL;               // insert blank line before cursor
extern char *ALN;              // insert N blank lines before cursor
extern int   AM;               // automatic margins?
extern char *BC;               // backspace, if not BS
extern int   BS;               // ASCII backspace works
extern char *CD;               // clear to end of display
extern char *CE;               // clear to end of line
extern char *CL;               // clear screen
extern int   CO;               // number of columns
extern char *CM;               // cursor motion
extern char *CR;               // cursor beginning of line
extern char *CS;               // set scroll region
extern int   DA;               // backing store off top?
extern int   DB;               // backing store off bottom?
extern char *DC;               // delete character at cursor
extern char *DL;               // delete line cursor is on
extern char *DLN;              // delete N lines at cursor
extern char *DM;               // string to enter delete mode
extern char *DO;               // cursor down
extern char *ED;               // string to end delete mode
extern int   HC;               // hardcopy terminal?
extern char *HO;               // cursor home
extern char *IS;               // initialize terminal
extern char *KD;               // down arrow key
extern char *KE;               // de-initialize keypad
extern char *KS;               // initialize keypad (for arrow keys)
extern char *KU;               // up arrrow key
extern char *LE;               // cursor back one column
extern int   LI;               // number of rows
extern char *LL;               // cursor to lower left
extern char *MR;               // reverse mode
extern char *ME;               // end reverse mode
extern int   OS;               // terminal overstrikes?
extern char  PC;               // pad character
extern char *PCstr;            // pad string
extern char *SE;               // end standout mode
extern char *SF;               // scroll screen up one line
extern char *SO;               // enter standout mode
extern char *SR;               // scroll screen down one line
extern char *TE;               // end cursor addressing mode
extern char *TI;               // enter cursor addressing mode
extern char *UP;               // cursor up
extern char *VE;               // end visual mode
extern char *VS;               // enter visual mode
extern char *XN;               // strange wrap behaviour

//
// termcap routines
//
extern "C" {
    extern short ospeed;        // terminal speed - needed by tputs()
#if !defined(__GNUG__) || __GNUG__ == 2
    int    tgetent(const char *buf, const char *name);
    int    tgetflag(const char *);
    int    tgetnum(const char *);
    char  *tgetstr(const char *, char **);
    char  *tgoto(const char *, int, int);
    int    tputs(const char *, int, int (*)(int));
#endif
}

//
// functions defined in display.C
//

extern void    clear_display();
extern void    clear_to_end_of_screen(int);
extern void    delete_listing_line(int);
extern void    init_display();
extern void    insert_listing_line(int);
extern int     outputch(int);
extern void    scroll_listing_up_one();
extern void    scroll_listing_down_one();
extern void    scroll_listing_up_N(int);
extern void    scroll_listing_down_N(int);
extern void    scroll_screen_up_one();
extern void    scroll_screen_down_one();
extern void    scroll_screen_up_N(int);
extern void    scroll_screen_down_N(int);
extern void    setraw();
extern void    termcap(const char *);
extern void    term_display();
extern void    termstop(int);
extern void    unsetraw();
extern void    update_screen_line(const char *, const char *, int);

/*
** output_string_capability - output a string capability from termcap
**                             to the terminal. The second argument,
**                             which defaults to 1, is the number
**                             of rows affected.
*/

inline void output_string_capability(const char *capability, int affected = 1)
{
    if (capability) tputs(capability, affected, outputch);
}

inline int rows() { return LI; }

inline int columns() { return CO; }

inline void initialize_terminal() { output_string_capability(IS); }

inline void synch_display() { (void)fflush(stdout); }

inline void enter_cursor_addressing_mode() { output_string_capability(TI); }

inline void enable_keypad() { output_string_capability(KS); }

inline void disable_keypad() { output_string_capability(KE); }

inline void enter_visual_mode() { output_string_capability(VS); }

inline void end_visual_mode() { output_string_capability(VE); }

inline void end_cursor_addressing_mode() { output_string_capability(TE); }

inline void enter_standout_mode() { output_string_capability(SO); }

inline void enter_reverse_mode() { output_string_capability(MR); }

inline void end_reverse_mode() { output_string_capability(ME); }

inline void end_standout_mode() { output_string_capability(SE); }

inline void enter_delete_mode() { output_string_capability(DM); }

inline void end_delete_mode() { output_string_capability(ED); }

inline void move_cursor(int row, int column)
{
    if (column >= columns()) column = columns()-1;
    output_string_capability(tgoto(CM, column, row));
}

inline void cursor_home()
{
    HO ? output_string_capability(HO) : move_cursor(0, 0);
}

inline void clear_to_end_of_line() { output_string_capability(CE); }

inline void move_to_modeline() { move_cursor(rows() - 2, 0); }

inline void move_to_message_line()
{
    if (LL)
        output_string_capability(LL);
    else
        move_cursor(rows()-1, 0); }

inline void clear_modeline() { move_to_modeline(); clear_to_end_of_line(); }

inline void clear_message_line()
{
    move_to_message_line();
    clear_to_end_of_line();
}

inline void backspace() {
    if (BS)
        putchar('\b');
    else if (LE)
        output_string_capability(LE);
    else
        output_string_capability(BC);
}

inline void cursor_up() { output_string_capability(UP); }

inline void delete_char_at_cursor()
{
    if (DM) output_string_capability(DM);
    output_string_capability(DC);
    if (ED) output_string_capability(ED);
}

inline void delete_screen_line(int y)
{
    move_cursor(y, 0);
    output_string_capability(DL, rows()-y);
}

inline void insert_blank_line(int y)
{
    move_cursor(y, 0);
    output_string_capability(AL, rows()-y);
}

inline void cursor_down() { output_string_capability(DO); }

inline void cursor_beginning_of_line() { output_string_capability(CR); } 

inline void cursor_wrap()
{
    cursor_beginning_of_line();
    cursor_down();
}

/*
** immediately ring the bell
*/

inline void ding() {
    //
    // This should be `output('\a')', but some braindead C compilers when
    // used as the backend to Cfront, don't recognize '\a' as the BELL.
    //
    outputch(7);
    synch_display();
} 

#endif
