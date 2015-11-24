/*
** Routines controlling the physical display
**
** display.C 1.30   Delta\'d: 14:27:58 10/5/92   Mike Lijewski, CNSF
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
#endif /*_IBMR2*/

#include <osfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#ifdef TERMIOS
#include <termios.h>
#include <unistd.h>
#elif  TERMIO
#include <termio.h>
#else
#include <sgtty.h>
#endif

#include "dired.h"
#include "display.h"

/*
** The definition of `ospeed\' -- needed by the termcap routines.
*/
short ospeed;

//
// termcap capabilities we use
//
char *AL;               // insert blank line before cursor
char *ALN;              // insert N blank lines at cursor
int   AM;               // automatic margins?
char *BC;               // backspace, if not BS
int   BS;               // ASCII backspace works
char *CD;               // clear to end of display
char *CE;               // clear to end of line
char *CL;               // clear screen
int   CO;               // number of columns
char *CM;               // cursor motion
char *CR;               // cursor beginning of line
char *CS;               // set scroll region
int   DA;               // backing store off top?
int   DB;               // backing store off bottom?
char *DC;               // delete character at cursor
char *DL;               // delete line cursor is on
char *DLN;              // delete N lines from cursor
char *DM;               // string to enter delete mode
char *DO;               // cursor down
char *ED;               // string to end delete mode
int   HC;               // hardcopy terminal?
char *IS;               // initialize terminal
char *HO;               // cursor home
char *KD;               // down arrow key
char *KE;               // de-initialize keypad
char *KS;               // initialize keypad \(for arrow keys\)
char *KU;               // up arrrow key
char *LE;               // cursor back one column
int   LI;               // number of rows
char *LL;               // cursor to lower left
char *MR;               // reverse mode
char *ME;               // end reverse mode
int   OS;               // terminal overstrikes?
char  PC;               // pad character
char *PCstr;            // pad string
char *SE;               // end standout mode
char *SF;               // scroll screen up one line
char *SO;               // enter standout mode
char *SR;               // scroll screen down one line
char *TE;               // end cursor addressing mode
char *TI;               // enter cursor addressing mode
char *UP;               // cursor up
char *VE;               // end visual mode
char *VS;               // enter visual mode
char *XN;               // strange wrap behaviour

/*
** termcap - reads termcap file setting all the terminal capabilities
**           which we\'ll use.
*/

void termcap(const char *term_type)
{
    static char capability_buffer[512], *bp = capability_buffer;
    char termcap_buffer[2048];

    switch (tgetent(termcap_buffer, term_type))
    {
      case -1:
        (void)fputs("couldn't open termcap database\n", stderr);
        exit(1);
      case 0:
        (void)fprintf(stderr, "invalid terminal type: `%s'\n", term_type);
        exit(1);
      default: break;
    }

    AL = tgetstr("al", &bp);
    ALN = tgetstr("AL", &bp);
    AM = tgetflag("am");
    BC = tgetstr("bc", &bp);
    BS = tgetflag("bs");
    CD = tgetstr("cd", &bp);
    CE = tgetstr("ce", &bp);
    CL = tgetstr("cl", &bp);
    CM = tgetstr("cm", &bp);
    CR = tgetstr("cr", &bp);
    CS = tgetstr("cs", &bp);
    DA = tgetflag("da");
    DB = tgetflag("db");
    DC = tgetstr("dc", &bp);
    DL = tgetstr("dl", &bp);
    DLN = tgetstr("DL", &bp);
    DM = tgetstr("dm", &bp);
    DO = tgetstr("do", &bp);
    ED = tgetstr("ed", &bp);
    HC = tgetflag("hc");
    HO = tgetstr("ho", &bp);
    IS = tgetstr("is", &bp);
    KD = tgetstr("kd", &bp);
    KE = tgetstr("ke", &bp);
    KS = tgetstr("ks", &bp);
    KU = tgetstr("ku", &bp);
    LE = tgetstr("le", &bp);
    LL = tgetstr("ll", &bp);
    MR = tgetstr("mr", &bp);
    ME = tgetstr("me", &bp);
    OS = tgetflag("os");
    PCstr = tgetstr("pc", &bp);
    SE = tgetstr("se", &bp);
    SF = tgetstr("sf", &bp);
    SO = tgetstr("so", &bp);
    SR = tgetstr("sr", &bp);
    TE = tgetstr("te", &bp);
    TI = tgetstr("ti", &bp);
    UP = tgetstr("up", &bp);
    VE = tgetstr("ve", &bp);
    VS = tgetstr("vs", &bp);
    XN = tgetstr("xn", &bp);

    PC = PCstr ? PCstr[0] :  0;

    if (!BC && !LE && !BS)
    {
        (void)fputs("terminal can't backspace - unusable\n", stderr);
        exit(1);
    }

    if (!BC) BC = LE ? LE : "\b";
    if (!CR) CR = "\r";
    /* if (!DO) DO = SF ? SF : "\n"; */

#ifdef __EMX__
    int sz[2];
    _scrsize(sz);
    CO = sz[0];
    LI = sz[1];
#endif
    const char *tmp = getenv("LINES");
    if (tmp) LI = atoi(tmp);
    tmp = getenv("COLUMNS");
    if (tmp) CO = atoi(tmp);

#ifdef TIOCGWINSZ
    struct winsize win;
    if (ioctl(2, TIOCGWINSZ, (char *)&win) == 0)
    {
        if (LI == 0 && win.ws_row > 0) LI = win.ws_row;
        if (CO == 0 && win.ws_col > 0) CO = win.ws_col;
    }
#endif

    if (CO == 0) CO = tgetnum("co");
    if (LI == 0) LI = tgetnum("li");

    if (LI == -1 || CO == -1 || HC || !CM || !CE)
    {
        (void)fputs("terminal too dumb to be useful\n", stderr);
        exit(1);
    }
    if (LI < 5)
    {
        (void)fputs("too few rows to be useful\n", stderr);
        exit(1);
    }
}

/*
** setraw - puts terminal into raw mode.  Cbreak mode actually, but
**          why be pedantic.  Flow control is disabled as well as BREAK keys.
**          Echoing is turned off as well as signal generation.  Hence
**          keyboard generated signals must be simulated.  Also sets
**          `ospeed\'.
*/

#ifdef TERMIOS
static struct termios tty_mode;	/* save tty mode here */
#elif  TERMIO
static struct termio tty_mode;	/* save tty mode here */
#else
static struct sgttyb  oarg;      /* save tty stuff here */
static struct tchars  otarg;
static struct ltchars oltarg;
#endif

void setraw()
{
#ifdef TERMIOS
    struct termios temp_mode;

    if (tcgetattr(STDIN_FILENO, &temp_mode) < 0)
    {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    tty_mode = temp_mode;  /* save for latter restoration */

    temp_mode.c_iflag &= ~(IGNBRK|ICRNL|INLCR);
    temp_mode.c_lflag &= ~(ICANON|ECHO|IEXTEN);
    temp_mode.c_oflag &= ~OPOST;
    temp_mode.c_cc[VQUIT] = 28; // C-\ is QUIT
    temp_mode.c_cc[VMIN]  = 1;	// min #chars to satisfy read
    temp_mode.c_cc[VTIME] = 0;	// read returns immediately

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &temp_mode) < 0)
    {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    ospeed = cfgetospeed(&temp_mode);
#elif TERMIO
    struct termio temp_mode;
    
    if (ioctl(0, TCGETA, (char *)&temp_mode) < 0)
    {
        perror("ioctl - TCGETA");
        exit(EXIT_FAILURE);
    }

    tty_mode = temp_mode;  /* save for latter restoration */

    temp_mode.c_iflag &= ~(IGNBRK|ICRNL|INLCR);
    temp_mode.c_lflag &= ~(ICANON|ECHO);
    temp_mode.c_oflag &= ~OPOST;
    temp_mode.c_cc[VQUIT] = 28; // C-\ is QUIT
    temp_mode.c_cc[VMIN]  = 1;	// min #chars to satisfy read
    temp_mode.c_cc[VTIME] = 0;	// read returns immediately

    if (ioctl(0, TCSETA, (char *)&temp_mode) < 0)
    {
        perror("ioctl - TCSETA");
        exit(EXIT_FAILURE);
    }

    ospeed = temp_mode.c_cflag & CBAUD;
#else
    struct sgttyb arg;
    struct tchars targ;
    struct ltchars ltarg;

    if (ioctl(fileno(stdin), TIOCGETP, (char *)&arg) < 0)
    {
        perror("ioctl - TIOCGETP");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fileno(stdin), TIOCGETC, (char *)&targ) < 0)
    {
        perror("ioctl - TIOCGETC");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fileno(stdin), TIOCGLTC, (char *)&ltarg) < 0)
    {
        perror("ioctl - TIOCGLTC");
        exit(EXIT_FAILURE);
    }

    oarg   = arg;
    otarg  = targ;
    oltarg = ltarg;

    arg.sg_flags=((arg.sg_flags&~(ECHO|CRMOD))|CBREAK) ;
    targ.t_eofc    = -1;  // turn off end-of-file character
    targ.t_brkc    = -1;  // turn off break delimiter
    ltarg.t_dsuspc = -1;  // turn off delayed suspend character
    ltarg.t_rprntc = -1;  // turn off reprint line character
    ltarg.t_flushc = -1;  // turn off flush character
    ltarg.t_werasc = -1;  // turn off erase work character
    ltarg.t_lnextc = -1;  // turn off literal next char

    if (ioctl(fileno(stdin), TIOCSETN, (char *)&arg) < 0)
    {
        perror("ioctl - TIOCSETN");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fileno(stdin), TIOCSETC, (char *)&targ) < 0)
    {
        perror("ioctl - TIOCSETC");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fileno(stdin), TIOCSLTC, (char *)&ltarg) < 0)
    {
        perror("ioctl - TIOCSLTC");
        exit(EXIT_FAILURE);
    }

    ospeed = arg.sg_ospeed;
#endif
}

/*
 * unsetraw - Restore a terminal\'s mode to whatever it was on the most
 *            recent call to the setraw\(\) function above.
 *            Exits with EXIT_FAILURE on failure.
 */

void unsetraw()
{
#ifdef TERMIOS
    if (tcsetattr(0, TCSAFLUSH, &tty_mode) < 0)
    {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
#elif TERMIO
    if (ioctl(0, TCSETA, (char *)&tty_mode) < 0)
    {
        perror("ioctl - TCSETA");
        exit(EXIT_FAILURE);
    }
#else
    if (ioctl(fileno(stdin), TIOCSETN, (char *)&oarg) < 0)
    {
        perror("ioctl - TIOSETN");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fileno(stdin), TIOCSETC, (char *)&otarg) < 0)
    {
        perror("ioctl - TIOSETC");
        exit(EXIT_FAILURE);
    }
    if (ioctl(fileno(stdin), TIOCSLTC, (char *)&oltarg) < 0)
    {
        perror("ioctl - TIOSLTC");
        exit(EXIT_FAILURE);
    }
#endif
}

/*
** outputch - a function to output a single character.
**            Termcap routines NEED a function.
*/

int outputch(int ch) { return putchar(ch); }

/*
** initialize display
*/

void init_display()
{
    setvbuf(stdout, 0, _IOFBF, BUFSIZ);  // fully buffer stdout
    setvbuf(stdin , 0, _IONBF, 0);  // no buffering on stdin

    const char *term = getenv("TERM");
    if (term == 0 || *term == 0)
#ifdef __EMX__
        term = "ansi";
#else
    {
        (void)fputs("please set your TERM variable appropriately\n", stderr);
        exit(1);
    }
#endif

    termcap(term);

    setraw();
    initialize_terminal();
    enter_cursor_addressing_mode();
    enter_visual_mode();
    enable_keypad();
    clear_display();
    synch_display();
}

/*
** terminate display
*/

void term_display()
{
    output_string_capability(tgoto(CM, 0, rows()-1));
    end_visual_mode();
    end_cursor_addressing_mode();
    disable_keypad();
    synch_display();
    unsetraw();
}

/*
** scroll_listing_up_N - scrolls the listing window up n lines.
**                       The cursor is left in column 0 of the first
**                       line to scroll into the window.
**                       Must have CS capability.
*/

void scroll_listing_up_N(int n)
{
    output_string_capability(tgoto(CS, rows()-3, 0));
    move_cursor(rows()-3, 0);
    for (int i = 0; i < n; i++) cursor_down();
    output_string_capability(tgoto(CS, rows()-1, 0));
    move_cursor(rows()-2-n, 0);
}

/*
** scroll_listing_down_N - half_down - scrolls the listing window
**                         \(line 0 - rows\(\)-3\) down \(rows\(\)-2\)/2 lines.
**                         The cursor is left in HOME position.
**                         Must have CS capability.
*/

void scroll_listing_down_N(int n)
{
    output_string_capability(tgoto(CS, rows()-3, 0));
    move_cursor(0, 0);
    for (int i = 0; i < n; i++) output_string_capability(SR, rows()-2);
    output_string_capability(tgoto(CS, rows()-1, 0));
    cursor_home();
}


/*
** scroll_listing_up_one - scrolls the listing window \(line 0 - rows\(\)-3\)
**                         up one row. The cursor is left in column
**                         0 of rows\(\)-3 row.  Assumes CS capability.
*/

void scroll_listing_up_one()
{
    output_string_capability(tgoto(CS, rows()-3, 0));
    move_cursor(rows()-3, 0);
    cursor_down();
    output_string_capability(tgoto(CS, rows()-1, 0));
    move_cursor(rows()-3, 0);
}

/*
** scroll_listing_down_one - scrolls the listing window \(line 0 - rows\(\)-3\)
**                           down one row. The cursor is left at HOME.
**                           Assumes CS capability.
*/

void scroll_listing_down_one()
{
    output_string_capability(tgoto(CS, rows()-3, 0));
    cursor_home();
    output_string_capability(SR, rows()-2);
    output_string_capability(tgoto(CS, rows()-1, 0));
    cursor_home();
}

/*
** insert_listing_line - inserts a blank line at line y, scrolling everything
**                       from y on down one line.  We only call this routine
**                       when we KNOW that y != rows\(\)-3 - the last listing
**                       line. Leaves the cursor in column 0 of the opened up
**                       line. Must have CS capability.
*/

void insert_listing_line(int y)
{
    output_string_capability(tgoto(CS, rows()-3, y));
    move_cursor(y, 0);
    output_string_capability(SR, rows()-3-y);
    output_string_capability(tgoto(CS, rows()-1, 0));
    move_cursor(y, 0);
}

/*
** delete_listing_line - deletes line at line y, scrolling the lines below
**                       y up.  We only call this routine when we KNOW that
**                       there is at least one line in need of being scrolled
**                       up. Must have CS capability.
*/

void delete_listing_line(int y)
{
    move_cursor(y, 0);
    clear_to_end_of_line();
    output_string_capability(tgoto(CS, rows()-3, y));
    move_cursor(rows()-3, 0);
    cursor_down();
    output_string_capability(tgoto(CS, rows()-1, 0));
}

/*
** termstop - service a SIGTSTP
*/

#ifdef SIGTSTP
void termstop(int)
{
    (void)signal(SIGTSTP,  SIG_IGN);
#ifdef SIGWINCH
    (void)signal(SIGWINCH, SIG_IGN);
#endif
    clear_display();
    synch_display();
    unsetraw();
    (void)kill(getpid(), SIGSTOP);
    setraw();
    (void)signal(SIGTSTP,  termstop);
#ifdef SIGWINCH
    (void)signal(SIGWINCH, winch);
#endif

    //
    // window size may have changed
    //
#ifdef TIOCGWINSZ
    int oCO = columns(), oLI = rows();
    struct winsize w;
    if (ioctl(2, TIOCGWINSZ, (char *)&w) == 0 && w.ws_row > 0) LI = w.ws_row;
    if (ioctl(2, TIOCGWINSZ, (char *)&w) == 0 && w.ws_col > 0) CO = w.ws_col;
    if (oCO != columns() || oLI != rows())
        win_size_changed = 1;
    else
        redisplay();
#else
    redisplay();
#endif
}
#endif /*SIGTSTP*/

/*
** clear_display
*/

void clear_display()
{
    if (CL)
        output_string_capability(CL);
    else if (CD)
    {
        cursor_home();
        output_string_capability(CD);
    }
    else
    {
        cursor_home();
        for (int i = 0; i < rows(); i++)
        {
            clear_to_end_of_line();
            cursor_down();
        }
        cursor_home();
    }
}

/*
** scroll_screen_up_one - must have DL or SF
*/

void scroll_screen_up_one()
{
    if (DL)
    {
        cursor_home();
        output_string_capability(DL, rows());
    }
    else
    {
        move_cursor(rows()-1, 0);
        output_string_capability(SF, rows());
    }
    if (DB) clear_message_line();
}

/*
** scroll_screen_down_one - must have AL or SR
*/

void scroll_screen_down_one()
{
    cursor_home();

    if (AL)
        output_string_capability(AL, rows());
    else
        output_string_capability(SR, rows());
    if (DA) clear_to_end_of_line();
}

/*
** scroll_screen_up_N - must have DLN, DL or SF.
**         
*/

void scroll_screen_up_N(int n)
{
    if (DLN)
    {
        cursor_home();
        output_string_capability(tgoto(DLN, 0, n), rows());
    }
    else if (DL)
    {
        cursor_home();
        for (int i = 0; i < n; i++)
            output_string_capability(DL, rows());
    }
    else
    {
        move_cursor(rows()-1, 0);
        for (int i = 0; i < n; i++)
            output_string_capability(SF, rows());
    }
    if (DB) clear_to_end_of_screen(rows()-n);
}

/*
** scroll_screen_down_N - must have ALN, AL or SR.
*/

void scroll_screen_down_N(int n)
{
    cursor_home();
	int i;
    if (ALN)
        output_string_capability(tgoto(ALN, 0, n), rows());
    else if (AL)
        for (i = 0; i < n; i++)
            output_string_capability(AL, rows());
    else
        for (i = 0; i < n; i++)
            output_string_capability(SR, rows());
    if (DA)
    {
        for (i = 0; i < n; i++)
        {
            clear_to_end_of_line();
            cursor_down();
        }
        cursor_home();
    }
}

/*
** clear_to_end_of_screen - clears screen from line y to the bottom
*/

void clear_to_end_of_screen(int y)
{
    move_cursor(y, 0);
    if (CD)
        output_string_capability(DL, rows()-y);
    else
        for (int i = 0; i < rows()-y; i++)
        {
	    move_cursor(y + i, 0);
            clear_to_end_of_line();
        }
}

/*
** update_screen_line
**
**     `oldline\' is what is currently on the screen in row `y\'
**     `newline\' is what we want on the screen in row `y\'
**
**     We make a good attempt to optimize the output of characters to
**     the screen.  We want to display `newline\' on the screen,
**     assuming `oldline\' is what is currently displayed.  This
**     will be "good" if `oldline\' and `newline\' are quite similar.
**     That is to say, this should only be called when there is an
**     expectation that `oldline\' and `newline\' are "almost" the same.
*/

void update_screen_line(const char *oldline, const char *newline, int y)
{
    if (strcmp(oldline, newline) == 0) return;

    size_t olen = strlen(oldline);
    size_t nlen = strlen(newline);
    size_t  len = olen < nlen ? olen : nlen;

    //
    // Never display more than columns\(\) characters.
    //
    int chop = 0;  // do we need to chop off the tail?
    if (len > columns())
    {
        chop = 1;
        len = columns();
    }

    char *equal = new char[len];

    //
    // How similar are the two strings?
    //
    int differences = 0;
    for (int i = 0; i < len; i++) equal[i] = 1;
    for (i = 0; i < len; i++)
        if (oldline[i] != newline[i])
        {
            differences++;
            equal[i] = 0;
        }

    if (differences > columns()/2)
    {
        //
        // We just display the new line.
        //
        clear_to_end_of_line();
        (void)fputs(newline, stdout);
        DELETE equal;

        return;
    }

    if (!OS)
    {
        //
        // We can just overwrite the old with the new.
        //
        int last = -2;  // position of last character written
        for (i = 0; i < len; i++)
        {
            if (equal[i]) continue;
            if (i - 1 != last) move_cursor(y, i);
            (i == len - 1 && chop) ? putchar('!') : putchar(newline[i]);
            last = i;
        }
        if (nlen > olen)
        {
            //
            // Have more characters to output.
            //
            chop = len > columns();
            move_cursor(y, i);
            for (i = (int)len; i < nlen && i < columns(); i++)
                (i == columns()-1 && chop) ? putchar('!') : putchar(newline[i]);
        }
        else if (nlen < olen)
        {
            move_cursor(y, i);
            clear_to_end_of_line();
        }
    }
    else
    {
        //
        // We can\'t overwrite.  Truncate at first difference.
        //
        int first = 0;
        for (i = 0; i < len; i++)
            if (!equal[i])
            {
                first = i;
                break;
            }
        move_cursor(y, i);
        clear_to_end_of_line();
        for (; i < nlen && i < columns(); i++)
            (i == columns() - 1) ? putchar('!') : putchar(newline[i]);
    }

    DELETE equal;
}
