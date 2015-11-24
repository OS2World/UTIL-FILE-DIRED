/*
** utilities.C - utility functions
**
** utilities.C 1.98   Delta\'d: 14:27:42 10/5/92   Mike Lijewski, CNSF
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


#include <ctype.h>

#ifndef _IBMR2
#include <libc.h>
#endif /*_IBMR2*/

#include <osfcn.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>

#ifdef _IBMR2
#include <sys/access.h>
#endif /*_IBMR2*/

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __EMX__
#include <io.h>
#include <process.h>
#define getcwd _getcwd2
#define DEVNULL "nul"
#else
#include <sys/wait.h>
#define DEVNULL "/dev/null"
#endif

#ifdef COMPLETION
#include <dirent.h>
#endif

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "dired.h"
#include "display.h"
#include "keys.h"

/*
** expand_tilde - expects a string of the form "~ ...".
**                Returns a new string in volatile storage
**                with the user\'s home directory in place of the `~\'.
**                The user\'s home directory is always appended
**                in the form: "/usr/staff/mjlx"; a slash is not added to
**                the end of the home directory string.  Returns the original
**                string if we cannot get the user\'s home directory.  The
**                user should not attempt to delete the return value.
*/

const char *expand_tilde(const char *str)
{
    static char *home = getenv("HOME");
    if (home == NULL)
    {
        struct passwd *user = getpwuid(getuid());
        if (user == NULL) return str;
        home = user->pw_dir;
    }
    if (*str != '~') return str;
    static String expansion;
    expansion = String(home) + (str + 1);
    return expansion;
} 

/*
** fgetline\(FILE *f\) - returns a pointer to the start of a line read
**	     	       from fp, or the null pointer if we hit eof or get
**                     an error from fgets\(\). Exits if new\(\) fails.
**                     Strips the newline from the line.
**		       Caller should free memory if desired.
*/

static const int FGETLINE_BUFSIZE = 80; // chunksize for calls to new\(\)

char *fgetline(FILE *fp)
{
    char *buffer = new char[FGETLINE_BUFSIZE];

    char *result = fgets(buffer, FGETLINE_BUFSIZE, fp);
    if (result == 0)
    {
        //
        // either error or at eof
        //
        DELETE buffer;
        return 0;
    }

    if (buffer[strlen(buffer)-1] != '\n' && !feof(fp))
    {
        //
        // longer line than buffer can hold
        //
        char *restofline = fgetline(fp);

        if (restofline == 0) return 0; // eof or error

        char *longline = new char[strlen(buffer) + strlen(restofline) + 1];
        (void)strcat(strcpy(longline, buffer), restofline);

        DELETE restofline;
        DELETE buffer;

        if (longline[strlen(longline) - 1] == '\n')
            longline[strlen(longline) - 1] = '\0';

        return longline;
    }
    else
    {
        if (buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = '\0';

        return buffer;
    }
}

/*
** current_directory - This routine tries to determine the full pathname
**                     of the current directory. The pointer returned is
**                     new\(\)\'d storage.
*/

char *current_directory()
{
    const int chunksize = 50;
    int size            = chunksize;
    char *dir           = new char[size];
    while (getcwd(dir, size) == 0)
        if (errno == ERANGE)
        {
            DELETE dir;
            dir = new char[size += chunksize];
            continue;
        }
        else
            //
            // Must have got an EACCES.
            //
            return 0;
    return dir;
}

/*
** display_string - prints a string to the given the display, guaranteeing not
**                  to print more than columns\(\) characters.  If the string
**                  exceeds the width of the window, a `!\' is placed in
**                  the final column.  Can be called with or without the
**                  length of the string to be printed.  In most places in
**                  the code we know the exact length of the strings we
**                  wish to print.  Note that `len\' has a default value
**                  of zero defined by the declaration in "dired.h".
**                  We never call this when trying to write to the last
**                  row on the screen.  That is the dominion of message\(\).
*/

void display_string(const char *str, size_t len)
{
    size_t string_length = len == 0 ? strlen(str) : len;

    if (string_length < columns())
    {
        (void)fputs(str, stdout);
        cursor_wrap();
    }
    else if (string_length > columns())
    {
        (void)printf("%*.*s%c", columns() - 1, columns() - 1, str, '!');
        if (!AM || XN) cursor_wrap();
    }
    else
    {
        (void)fputs(str, stdout);
        if (!AM || XN) cursor_wrap();
    }
}

/*
** get_directory_listing - Get a long listing of the given directory.
**                         Returns 0 if we got other than a
**                         "memory exhausted" error.
*/

DirList *get_directory_listing(char *dirname)
{
    message("Reading directory ... ");

    String cmd = String(ls_cmd[the_sort_order()]) + dirname + " 2>" + DEVNULL;
    FILE *fp = popen(cmd, "r");
    if (fp == 0) return 0;
    
    DirList *directory = new DirList(dirname);

    //
    // discard lines of the form:
    //
    //      total 1116
    //
    char *line = fgetline(fp);
    DELETE line;
    if (fp == 0) return 0;
    
    while((line = fgetline(fp)) != 0) directory->add(new DirLine(&line));

    message("Reading directory ... done");

    if (feof(fp) && !ferror(fp))
    {
        (void)pclose(fp);
        return directory;
    }
    else
        return 0;
}

/*
** error - Prints error message so it can be read.  This is the error
**         function we call once we\'ve initialized the display.
*/

void error(const char *str)
{
    clear_display();
    move_cursor(rows()-1, 0);
    (void) printf(str);
    cursor_wrap();
    synch_display();
    term_display();
    exit(EXIT_FAILURE);
}

void error(const char *fmt, int index)
{
    clear_display();
    move_cursor(rows()-1, 0);
    (void) printf(fmt, index);
    cursor_wrap();
    synch_display();
    term_display();
    exit(EXIT_FAILURE);
}

void error(const char *fmt, const char *file, int line)
{
    clear_display();
    move_cursor(rows()-1, 0);
    (void) printf(fmt, file, line);
    cursor_wrap();
    synch_display();
    term_display();
    exit(EXIT_FAILURE);
}

void error(const char *fmt, const char *file, int line, const char *str)
{
    clear_display();
    move_cursor(rows()-1, 0);
    (void) printf(fmt, file, line, str);
    cursor_wrap();
    synch_display();
    term_display();
    exit(EXIT_FAILURE);
}

/*
** update_modeline - this routine concatenates the two strings
**                   into the modeline.  The modeline
**                   is displayed in standout mode if possible.
**                   We never put more than columns\(\) characters into
**                   the modeline.  The modeline is the penultimate
**                   line on the terminal screen.  It does not
**                   synch the display.  If head == tail == 0, we
**                   just display the old modeline.  This happens
**                   if for some reason we had to clear the screen.
*/

void update_modeline(const char *head, const char *tail)
{
    static char *oldline;
    move_to_modeline();
    enter_reverse_mode();

    if (head == 0 && tail == 0)
    {
        //
        // Redisplay old modeline.
        //
        (void)fputs(oldline, stdout);
        end_reverse_mode();
        return;
    }

    int len = (int)strlen(head);
    char *modeline = new char[columns() + 1];
    (void)strncpy(modeline, head, columns());
    modeline[columns()] = 0;  // ensure it\'s null-terminated

    if (len < columns())
    {
        //
        // Write exactly columns\(\) characters to modeline.
        //
        for (int i = len; i < columns() - 1 && tail && *tail; i++, tail++)
            modeline[i] = *tail;
        if (i < columns() - 1)
        {
            modeline[i++] = ' ';
            for (; i < columns(); i++) modeline[i] = '-';
        }
        else if (tail && *tail)
            //
            // The string was overly long.  Put a \'!\' in the last space
            // on the modeline to signify truncation.
            //
            modeline[columns() - 1] = '!';
        else
            //
            // Here len == columns\(\)-1 && there is nothing else in tail.
            //
            modeline[columns() - 1] = ' ';
    }
    else if (len > columns())
        modeline[columns() - 1] = '!';

    if (oldline)
    {
        update_screen_line(oldline, modeline, rows() - 2);
        DELETE oldline;
    }
    else
        (void)fputs(modeline, stdout);

    oldline = modeline;
    end_reverse_mode();
}

/*
** is_directory - returns non-zero if a directory, otherwise 0.
**                Also returns zero on error.
*/

int is_directory(const char *dir)
{
    struct stat stbuf;
    if (stat(dir, &stbuf) < 0) return 0;
    return S_ISDIR(stbuf.st_mode);
}

/*
** is_regular_file - returns non-zero if a regular file, otherwise 0.
**                   Also returns zero on error.
*/

int is_regular_file(const char *file)
{
    struct stat stbuf;
    if (stat(file, &stbuf) < 0) return 0;
    return S_ISREG(stbuf.st_mode);
}

/*
** read_and_exec_perm - returns non-zero if we have read and execute
**                      permission on the directory, otherwise 0.
**                      Returns 0 on error.
*/

int read_and_exec_perm(const char *dir)
{
    return access(dir, R_OK | X_OK) == -1 ? 0 : 1;
}

/*
** Find the column position of the first character of the filename
** in the current line of the given DirList.
**
** The straight-forward way to do this is to walk the string from it\'s
** tail to it\'s head until we hit some whitespace.  This presumes
** that filenames don\'t contain whitespace.  The one special case
** to worry about is if the file is a symbolic link.  In that case we\'ll
** have a filename field entry of the form
**
**       Xm -> /usr/lpp/include/Xm
*/

int goal_column(DirList *l)
{
    DirLine *line = l->currLine();
    const char *tmp;

#ifndef NO_SYMLINKS
    if ((tmp = strstr(line->line(), " -> ")) != 0)
        //
        // We have a symbolic link.
        //
        --tmp;
    else
#endif
        tmp = line->line() + line->length() - 1;

    while(!isspace(*tmp)) --tmp;

    return tmp - line->line() + 1;
}

/*
** get_file_name - returns the filename of the current line of the DirList
**                 in volatile storage.
**
**                 If we have a symbolic link, we return the link not
**                 the file pointed to.
*/

const char *get_file_name(DirList *dl)
{
    static String file;
    file = &(dl->currLine()->line())[goal_column(dl)];

    //
    // Do we have a symbolic link?
    //
    char *result = strstr(file, " -> ");
    if (result) *result = '\0';

    return file;
}

/*
** redisplay - this routine redisplays the DirList at the top
**             of our stack.  It assumes that the physical screen 
**             has become corrupted, clearing each line before writing
**             to it.
*/

void redisplay()
{
    DirList *dl = dir_stack->top();

    DirLine *ln = dl->firstLine();
    cursor_home();
    for (int i = 0; i < rows() - 2 && ln; i++, ln = ln->next())
    {
        clear_to_end_of_line();
        display_string(ln->line(), ln->length());
    }
    move_cursor(i, 0);
    for (; i < rows() - 2; i++)
    {
        clear_to_end_of_line();
        cursor_down();
    }

    move_to_modeline();
    clear_to_end_of_line();
    update_modeline();
    clear_message_line();

    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(dl->savedYPos(), dl->savedXPos());

    synch_display();
}

//
// forward declaration
//
static void adjust_window();

/*
** eat_a_character - displays the message in standout mode and waits
**                   until a character is typed.  Deals with SIGWINCH
**                   and SIGTSTP
*/

void eat_a_character(const char *msg)
{
    enter_standout_mode();
    message(msg);
    end_standout_mode();
    char c;
    while (1)
        if (get_key(&c) < 0 // assume fails only when errno == EINTR
#ifdef SIGWINCH
            || win_size_changed
#endif
            )
        {
#ifdef SIGWINCH
            if (win_size_changed)
            {
                win_size_changed = 0;
                adjust_window();
                redisplay();
            }
#endif
            //
            // Must redisplay the message.
            //
            enter_standout_mode();
            message(msg);
            end_standout_mode();
        }
        else
            return;
}

/*
** read_from_keybd - reads a character from the keyboard.  It only returns
**                   when it\'s successfully read a character.  So if we
**                   get suspended while waiting to read a character, the
**                   signal handler will redisplay the screen and we\'ll
**                   still be here waiting to read a character.  This is
**                   only used by the main command loop so that we know
**                   that redisplay\(\) will do all the necessary redisplay
**                   operations.
*/

int read_from_keybd()
{
    char c;
    while(1)
        if (get_key(&c) < 0 // assume fails only when errno == EINTR
#ifdef SIGWINCH
            || win_size_changed
#endif
            )
        {
#ifdef SIGWINCH
            if (win_size_changed)
            {
                win_size_changed = 0;
                adjust_window();
                redisplay();
            }
#endif
        }
        else
            return c;
}

/*
** execute - executes command using exec\(2\).  Returns 1 if the exec
**           went OK, otherwise it returns 0.  If `closem\' is true, which
**           is the default, we close file descriptors 0, 1 and 2.
*/

int execute(const char *file, const char *argv[], int closem)
{
    int status;
#ifdef __EMX__
    status = spawnvp(P_WAIT, file, (char *const *)argv);
    set_signals();
    setraw();
    return status == 0 ? 1 : 0;
#else
    int pid = fork();
    switch(pid)
    {
      case -1: // error
        return 0;
      case 0: // in the child
        if (closem)
        {
            (void)close(0);
            (void)close(1);
            (void)close(2);
        }
        execvp(file, (char *const *)argv);
        //
        // Exec failed.
        //
        exit(1);
      default: // in the parent
        waitpid(pid, &status, 0);
        return status == 0 ? 1 : 0;
    }
#endif
}

/*
** exec_with_system - execute the passed command using system\(3\).
**                    If prompt == 1, which is the default, we prompt for
**                    a key before returning.  We use system\(3\) so that
**                    shell and environment variables will be expanded.
*/

void exec_with_system(const char *cmd, int prompt)
{
    unsetraw();
    unset_signals();
    system(cmd);
    set_signals();
    setraw();
    if (prompt) eat_a_character("Press Any Key to Continue");
}

#ifdef SIGWINCH

#include <sys/ioctl.h>

/*
** adjust_window - called to adjust our window after getting a SIGWINCH
*/

static void adjust_window()
{
#ifdef TIOCGWINSZ
    struct winsize w;
    if (ioctl(2, TIOCGWINSZ, (char *)&w) == 0 && w.ws_row > 0) LI = w.ws_row;
    if (ioctl(2, TIOCGWINSZ, (char *)&w) == 0 && w.ws_col > 0) CO = w.ws_col;

    //
    // Is current line still on the screen?
    //
    if (dir_stack->top()->savedYPos() >= rows()-2)
    {
        dir_stack->top()->setCurrLine(dir_stack->top()->firstLine());
        dir_stack->top()->saveYXPos(0, goal_column(dir_stack->top()));
    }

    // need to adjust lastLine\(\)
    DirLine *ln = dir_stack->top()->firstLine();
    for (int i = 0; i < rows()-2 && ln; i++, ln = ln->next()) ;
    ln ? dir_stack->top()->setLast(ln->prev()) :
         dir_stack->top()->setLast(dir_stack->top()->tail());
#endif
}
#endif

#ifdef COMPLETION

/*
** string_sort - this is the sort routine we pass to qsort\(\) for
**               sorting the table of possible completions.
*/

static int string_sort(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

/*
** complete - read directory `dir\' for any completions of `prefix\'.
**            Returns zero on error; if prefix is null; if there are no
**            completions; or if the completion is the prefix itself.
**            Else it returns a string in volatile space containing the
**            longest possible completion of `prefix\', minus the
**            characters in the prefix. Used only by prompt\(\).
*/

char *complete(const char *directory, const char *prefix)
{
    static char *completion = 0;  // our volatile storage
    size_t prefix_length = strlen(prefix);
    const int chunksize = 10;
    int tablesize = chunksize;
    int matches = 0;  // number of matches of `prefix\' in `directory\'
    
    DIR	*dirp = opendir(directory);
    if (dirp == NULL) return 0;

    char **completions = new char*[tablesize * sizeof(char *)];

    struct dirent *entry;
    for (entry = readdir(dirp); entry != NULL; entry = readdir(dirp))
    {
        if (strncmp(entry->d_name, prefix, prefix_length) == 0)
        {
            //
            // We\'ve got a match.
            //
            completions[matches] = new char[entry->d_namlen + 1];
            (void)strcpy(completions[matches++], entry->d_name);
            if (matches == tablesize)
            {
                //
                // Grow table.
                //
                tablesize += chunksize;
                char **newtable = new char*[tablesize * sizeof(char *)];
                for (int i = 0; i < tablesize - chunksize; i++)
                    newtable[i] = completions[i];
                DELETE completions;
                completions = newtable;
            }
        }
    }

    //
    // I should really be testing the rc here, but so many of the
    // machines I\'ve tested have this improperly prototyped to void,
    // that it\'s best not to for the time being.
    //
    (void)closedir(dirp);

    if (matches == 0)
    {
        DELETE completions;
        return 0;
    }

    if (matches == 1)
    {
        if (strcmp(prefix, completions[0]) == 0)
        {
            //
            // If the completion matches our input prefix, we return 0.
            //
            DELETE completions[0];
            DELETE completions;
            return 0;
        }
        DELETE completion;

        completion = new char[strlen(completions[0]) - prefix_length +1];
        (void)strcpy(completion, completions[0] + prefix_length);

        DELETE completions[0];
        DELETE completions;

        return completion;
    }

    qsort(completions, matches, sizeof(char **), string_sort);

    //
    // the completion cannot be longer than the first sorted item
    //
    size_t maxlen = strlen(completions[0]);
    char *tmp = new char[maxlen + 1];
    (void)strcpy(tmp, prefix);
    size_t i = prefix_length;
    char c = completions[0][i];
    for (; i < maxlen; i++, c = completions[0][i])
    {
        int stop = 0;
        for (int j = 1; j < matches; j++)
            if (c != completions[j][i])
            {
                stop = 1;
                break;
            }
        if (stop) break;
        tmp[i] = c;
    }
    tmp[i] = 0;  // nullify the string
    if (strcmp(prefix, tmp) == 0)
    {
        //
        // If the completion matches our input prefix, we return 0.
        //
        DELETE tmp;
        for (i = 0; i < matches; i++) DELETE completions[i];
        DELETE completions;

        return 0;
    }
    DELETE completion;
    completion = new char[maxlen - prefix_length + 1];
    (void)strcpy(completion, tmp + prefix_length);

    DELETE tmp;

    for (i = 0; i < matches; i++) DELETE completions[i];
    DELETE completions;

    return completion;
}

#endif /*COMPLETION*/

/*
** prompt - displays `msg\' prompt and then collects the response.
**          The keys of the response are echoed as they\'re collected.  
**          A response of 0 indicates that the command was aborted.
**          A response can contain any graphical character.
**          C-G will abort out of a prompt; Backspace works as expected.
**          Carriage return indicates the end of response.
**          Non-graphical characters are ignored.  If do_completion != 0,
**          with do_completon == 0 by default, we do filename completion
**          on the TAB character.  The response is kept in volatile storage,
**          so if the client needs to save it, they must make a copy of it.
*/

#ifdef COMPLETION
const char *prompt(const char *msg, int do_completion)
#else
const char *prompt(const char *msg)
#endif
{
    static char *response;  // our volatile storage

    size_t written = 0;     // number of characters written to message line
    const char *abort_msg = "(C-g to Abort) ";
    String nmsg = String(abort_msg) + msg;
    size_t len = nmsg.length();

    move_to_message_line();  // it will have been already cleared by get_key\(\)

    if (len < columns())
    {
        (void)fputs(nmsg, stdout);
        written = len;
    }
    else
    {
        // Leave space for columns\(\)/2 + 1 characters.
        (void)fputs((const char *)nmsg + (len-columns()/2+1), stdout);
        written = columns()/2 - 1;
    }
    synch_display();

    //
    // We never echo into the last position in the message window.
    //
    size_t space_available = columns() - written; // available spaces in line

    DELETE response;  // release any volatile storage
    response = new char[space_available + 1];
    size_t pos = 0;  // index of next character in `response\'

    char key;
    for (;;)
    {
        if (get_key(&key) < 0 // assume fails only when errno == EINTR
#ifdef SIGWINCH
            || win_size_changed
#endif
            )
        {
#ifdef SIGWINCH
            if (win_size_changed)
            {
                win_size_changed = 0;
                adjust_window();
                redisplay();
            }
#endif
            //
            // On a SIGTSTP the signal handler does the redisplay\(\),
            // so all we have to worry about is getting the prompt
            // redisplayed correctly.
            // Must make sure total output is less than screen width.
            //
            clear_message_line();
            response[pos] = 0;
            if (pos + len < columns())
            {
                //
                // Output message and response-to-date.
                //
                (void)fputs(nmsg, stdout);
                (void)fputs(response, stdout);
                space_available = columns() - pos - len;
            }
            else if (pos < columns())
            {
                //
                // Display the response.
                //
                (void)fputs(response, stdout);
                space_available = columns() - strlen(response);
            }
            else
            {
                //
                // Display the backend of the response
                //
                (void)fputs(&response[pos - columns()/2 + 1], stdout);
                space_available = columns()/2 + 1;
            }
            synch_display();
        }
        else if (isprint(key))
        {
            //
            // Echo character to message window and wait for another.
            //
            response[pos++] = key;
            space_available--;
            if (!space_available)
            {
                //
                // Need to allocate more room for the response.
                // Note that strlen\(response\) == pos
                //
                space_available = columns()/2 + 1;
                char *nresponse = new char[pos + space_available + 1];
                response[pos] = 0;  // stringify response
                (void)strcpy(nresponse, response);
                DELETE response;
                response = nresponse;
                //
                // Shift prompt in message window so we
                // always have the end in view to which we\'re
                // adding characters as they\'re typed.
                //
                clear_message_line();
                (void)fputs(&response[pos - columns()/2 + 1], stdout);
            }
            else
                putchar(key);
            synch_display();
        }
        else
            switch (key)
            {
#ifdef COMPLETION
              case KEY_TAB: // perform filename completion
                if (do_completion == 0 || pos == 0)
                {
                    ding();
                    break;
                }

                response[pos] = 0; // stringify response
                char *last = strrchr(response, '/');
                const char *current_directory = dir_stack->top()->name();
                char *completion = 0;

                if (response[0] == '~')
                {
                    //
                    // Expand the tilde before trying the completion,
                    // but there is no use trying the completion if we don\'t
                    // have a string of the form: "~/.*".
                    //
                    if (response[1] != '/')
                    {
                        ding();
                        break;
                    }
                    String head(response);
                    char *expansion = (char *) expand_tilde(head);
                    char *last = strrchr(expansion, '/');
                    expansion[last - expansion] = 0;
                    completion = complete(expansion, last + 1);
                }
                else if (last == NULL)
                    //
                    // Filename is relative to our current directory.
                    //
                    completion = complete(current_directory, response);
                else
                {
                    //
                    // The response contains a slash.
                    //
                    if (response[0] == '/')
                    {
                        //
                        // It\'s an absolute path.
                        //
                        if (pos == 1)
                        {
                            ding();
                            break;
                        }
                        if (response == last)
                            //
                            // The first slash is also the last.
                            //
                            completion = complete("/", last + 1);
                        else
                        {
                            String head(response);
                            head[last - response] = 0;
                            completion = complete(head, last + 1);
                        }
                    }
                    else
                    {
                        //
                        // It\'s relative to our current directory.
                        //
                        String head = String(current_directory)+ "/" +response;
                        head[strlen(current_directory)+last-response+1] = 0;
                        completion = complete(head, last + 1);
                    }
                }
                if (completion == 0)
                {
                    ding();
                    break;
                }

                size_t clen = strlen(completion);
                pos += clen;
                space_available -= clen;
                if (space_available > 0)
                {
                    //
                    // We\'ve got the space to hold the completed filename.
                    //
                    (void)strcat(response, completion);
                    (void)fputs(completion, stdout);
                }
                else
                {
                    //
                    // Allocate more space for response and adjust
                    // message line.
                    //
                    char *nresponse = new char[pos + columns()/2 + 1];
                    (void)strcpy(nresponse, response);
                    (void)strcat(nresponse, completion);
                    DELETE response;
                    response = nresponse;
                    clear_message_line();
                    (void)fputs(&response[pos - columns()/2 + 1], stdout);
                    space_available = columns()/2 + 1;
                }
                synch_display();
                break;

#endif /*COMPLETION*/

              case KEY_CR: // we have the complete response
                response[pos] = 0;
                clear_message_line();
                synch_display();
                return response;

              case KEY_ABORT: // abort --  reset cursor to previous position
                move_cursor(dir_stack->top()->savedYPos(),
                            dir_stack->top()->savedXPos());
                message("Aborted");
                return 0;

              case KEY_BKSP: // back up one character
                if (pos == 0)
                {
                    ding();
                    break;
                }
                backspace();
                DC ? delete_char_at_cursor() : clear_to_end_of_line();
                --pos;
                ++space_available;
                if (space_available == columns())
                {
                    //
                    // The only way this can happen is if we
                    // had previously shifted the response to the left.
                    // Now we must shift the response to the right.
                    //
                    clear_message_line();
                    response[pos] = 0;
                    if (pos + len < columns())
                    {
                        //
                        // Output message and response-to-date.
                        //
                        (void)fputs(nmsg, stdout);
                        (void)fputs(response, stdout);
                        space_available = columns() - pos - len;
                    }
                    else if (pos < columns())
                    {
                        //
                        // Display the response.
                        //
                        (void)fputs(response, stdout);
                        space_available = columns() - strlen(response);
                    } else
                    {
                        //
                        // Display the backend of the response
                        //
                        (void)fputs(&response[pos - columns()/2 + 1], stdout);
                        space_available = columns()/2 + 1;
                    }
                }
                synch_display();
                break;

              default: ding(); break; // ignore other characters
            }
    }
}

/*
** lines_displayed - returns the number of lines in the DirList
**                   currently displayed on the screen.
*/

int lines_displayed(DirList *dl)
{
    DirLine *ln = dl->firstLine();
    for (int i = 1; ln != dl->lastLine(); i++, ln = ln->next()) ;
    return i;
}

/*
** message - prints a message on the last line of the screen.
**           It is up to the calling process to put the cursor
**           back where it belongs.  Synchs the display.  It can
**           be called as either:
**
**                message\(msg\);
**           or
**                message\(fmt, str\);
**
**           In the later case it must be the case that the format `fmt\'
**           has exactly one `%\' into which the `str\' will be substituted
**           as in the ?printf\(\) functions. 
*/

static char message_window_dirty = 0;

void message(const char *fmt, const char *str)
{
    char *msg;          // the complete message to be output
    int allocated = 0;  // was `msg\' allocated in new\(\) space?

    clear_message_line();

    if (str)
    {
        msg = new char[strlen(fmt) + strlen(str) + 1];
        const char *token = strchr(fmt, '%');
        if (token == 0)
            //
            // This shouldn\'t happen.  But if it does, let\'s
            // just print the format `fmt\'.
            //
            msg = (char *)fmt;
            else
            {
            (void)strncpy(msg, fmt, token - fmt);
            msg[token - fmt] = 0;  // strncpy doesn\'t nullify the string
            (void)strcat(msg, str);
            (void)strcat(msg, token + 1);
            allocated = 1;
        }
    }
    else
        msg = (char *)fmt;

    if (strlen(msg) < columns())
        (void)fputs(msg, stdout);
    else
        (void)printf("%*.*s", columns() - 1, columns() - 1, msg);

    synch_display();
    message_window_dirty = 1;
    if (allocated) DELETE msg;
}

/*
** get_key - reads a key using getch\(\) and then clears the message window,
**           if it needs to be cleared. Used only by read_commands in the
**           main switch statement so that message\(\) doesn\'t need to sleep\(\)
**           and clear\(\) on the messages that get written.  This way, the
**           message window is cleared after each keypress within the main
**           loop, when necessary.  We also check for and deal with window
**           size changes and the UP and DOWN arrow keys here.
*/

struct arrow_key {
    int len;
    int *seq;
    arrow_key(const char *);
};

arrow_key::arrow_key(const char *str)
{
    if (str == 0)
    {
        //
        // The capability isn\'t defined.
        //
        len = 0;
        seq = 0;
        return;
    }

    seq = new int[12]; // should be ample

    int i = 0;
    do
    {
        switch (*str)
        {
          case '\\':
          {
              int c = *++str;
              switch (c)
              {
                case 'E':  seq[i++] = 0x1b; break;
                case 'b':  seq[i++] = '\b'; break;
                case 'f':  seq[i++] = '\f'; break;
                case 'n':  seq[i++] = '\n'; break;
                case 'r':  seq[i++] = '\r'; break;
                case 't':  seq[i++] = '\t'; break;
                case 'v':  seq[i++] = '\v'; break;
                case '\\': seq[i++] = '\\'; break;
                case '\'': seq[i++] = '\''; break;
                case '\"': seq[i++] = '\"'; break;
                case '^':  seq[i++] = '^';  break;
                default:
                  error("invalid escape in /etc/termcap for arrow key: \\%c",
                        c);
                  break;
              }
              break;
          }
          case '^':
          {
              int c = *++str;
              if (isalpha(c))
              {
                  seq[i] = (c > 'a' ? c - 'a' : c - 'A') + 1;
                  i++;  // g++ 2.2.2 chokes if I write seq\[i++\] = ...
              }
              else
                  switch(c)
                  {
                    case '[':  seq[i++] = 0x1b; break;
                    case '\\': seq[i++] = 0x1c; break;
                    case ']':  seq[i++] = 0x1d; break;
                    case '^':  seq[i++] = 0x1e; break;
                    case '_':  seq[i++] = 0x1f; break;
                    default:
                      error("invalid control sequence for arrow key: ^%c",
                            c);
                      break;
                  }
              }
              break;
          default:
              seq[i++] = *str;
              break;
        }
    } while (*++str);

    len = i;
}

int get_key(DirList *dl)
{
    int key, index = 0;
    static arrow_key up(KU), down(KD);
    static int *keys = 0;
    static int remaining = 0;

    if (keys == 0) keys = new int[max(up.len, down.len)];

#ifdef SIGWINCH
    if (win_size_changed)
    {
        win_size_changed = 0;
        adjust_window();
        redisplay();
    }
#endif

    if (remaining)
    {
        //
        // We have some characters left over from a partial match
        // of an arrow key; use them up.
        //
        key = keys[0];
        remaining--;
        for (int i = 0; i < remaining; i++) keys[i] = keys[i+1];
        return key;
    }
    else
        key = read_from_keybd();

    if (message_window_dirty)
    {
        clear_message_line();
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        message_window_dirty = 0;
    }

    //
    // Now deal with potential arrow keys.
    //
    if (KU || KD)
    {
        for (index = 0; (index < up.len && up.seq[index] == key) ||
             (index < down.len && down.seq[index] == key); index++)
        {
            if ((up.len - 1) == index && up.seq[index] == key)
                return KEY_ARROW_UP;
            if ((down.len - 1) == index && down.seq[index] == key)
                return KEY_ARROW_DOWN;
            if (index == (max(up.len, down.len) - 1)) break;
            keys[index] = key;
            key = read_from_keybd();
        }
        if (index == 0)
            return key; // no initial match -- the most usual case
        else
        {
            //
            // We had a partial match, but not a complete one.
            // We must return the characters which we\'ve read in
            // the proper order so that the main command loop can
            // check for matches.  The problem here is the potential
            // ambiguity between what the terminal claims to be arrow
            // keys and what has been hardcoded as the commands for
            // dired.
            //
            keys[index] = key;
            key = keys[0];  // what we\'ll return to the command loop
            for (int i = 0; i < index; i++) keys[i] = keys[i+1];
            remaining = index;
            return key;
        }
    }
    else
        return key;
}
    
/*
** cleanup - cleanup and exit after a SIGHUP, SIGTERM, SIGQUIT or SIGINT
*/

void cleanup(int) { term_display(); exit(0); }

/*
** initial_listing - prints the initial listing screen.  Called  by dired\(\)
**                   and read_commands\(\) when rereading the current directory.
**                   Adjusts firstLine\(\), lastLine\(\) and currLine\(\).
*/

void initial_listing(DirList *dl)
{
    static int first = 1;

    DirLine *ln = dl->head();
    dl->setFirst(ln);
    dl->setCurrLine(ln);
    cursor_home();
    for (int i = 0; i < rows()-2 && ln; ln = ln->next(), i++)
    {
        if (!first) clear_to_end_of_line();
        display_string(ln->line(), ln->length());
    }

    ln ? dl->setLast(ln->prev()) : dl->setLast(dl->tail());

    //
    // Don\'t forget to clear any remaining lines in those
    // cases when the screen hasn\'t already been cleared and
    // there are potentially dirty lines from a previous dired\(\).
    //
    if (!first)
    {
        move_cursor(i, 0);
        for (; i < rows() - 2; i++)
        {
            clear_to_end_of_line();
            cursor_down();
        }
    }
    if (first) first = 0;
}

#ifdef SIGWINCH

/*
** winch - set flag indicating window size changed.
*/

void winch(int)
{
    (void)signal(SIGWINCH, SIG_IGN);
    win_size_changed = 1;
    (void)signal(SIGWINCH, winch);
}

#endif    

/*
** set_signals - set up our signal handlers
*/

void set_signals()
{
    (void)signal(SIGHUP,  cleanup);
    (void)signal(SIGINT,  cleanup);
    (void)signal(SIGQUIT, cleanup);
    (void)signal(SIGTERM, cleanup);
#ifdef SIGTSTP
    (void)signal(SIGTSTP, termstop);
#endif
#ifdef SIGWINCH
    (void)signal(SIGWINCH, winch);
#endif
}

/*
** unset_signals - set signals back to defaults
*/

void unset_signals()
{
    (void)signal(SIGHUP,  SIG_DFL);
    (void)signal(SIGINT,  SIG_DFL);
    (void)signal(SIGQUIT, SIG_DFL);
    (void)signal(SIGTERM, SIG_DFL);
#ifdef SIGTSTP
    (void)signal(SIGTSTP, SIG_DFL);
#endif
#ifdef SIGWINCH
    (void)signal(SIGWINCH, SIG_DFL);
#endif
}

/*
** leftshift_current_line - shifts the current line in DirList left until
**                          its tail is visible.
*/
void leftshift_current_line(DirList *dl)
{
    int inc = dl->currLine()->length()-columns()+1;
    move_cursor(dl->savedYPos(), 0);
    clear_to_end_of_line();
    display_string(&(dl->currLine()->line())[inc],columns()-1);
    dl->saveYXPos(dl->savedYPos(), max(goal_column(dl)-inc, 0));
    move_cursor(dl->savedYPos(), dl->savedXPos());
}

/*
** rightshift_current_line - rightshifts current line to "natural" position.
*/
void rightshift_current_line(DirList *dl)
{
    move_cursor(dl->savedYPos(), 0);
    clear_to_end_of_line();
    display_string(dl->currLine()->line(), dl->currLine()->length());
    dl->saveYXPos(dl->savedYPos(), goal_column(dl));
    move_cursor(dl->savedYPos(), dl->savedXPos());
}

#ifdef NO_STRSTR
/*
** strstr - from Henry Spencers ANSI C library suite
*/

char *strstr(const char *s, const char *wanted)
{
    register const char *scan;
    register size_t len;
    register char firstc;
    
    //
    // The odd placement of the two tests is so "" is findable.
    // Also, we inline the first char for speed.
    // The ++ on scan has been moved down for optimization.
    //
    firstc = *wanted;
    len = strlen(wanted);
    for (scan = s; *scan != firstc || strncmp(scan, wanted, len) != 0; )
        if (*scan++ == '\0') return NULL;
    return (char *)scan;
}

#endif /* NO_STRSTR */

#ifdef NO_STRCHR
/*
** strchr - find first occurrence of a character in a string.  From Henry
**          Spencer\'s string\(3\) implementation.
*/

char *strchr(const char *s, char charwanted)
{
    register const char *scan;
    
    //
    // The odd placement of the two tests is so NULL is findable.
    //
    for (scan = s; *scan != charwanted;)	/* ++ moved down for opt. */
        if (*scan++ == '\0') return 0;
    return (char *)scan;
}

/*
** strrchr - find last occurrence of a character in a string. From Henry
**           Spencer\'s string\(3\) implementation. 
*/

char *strrchr(const char *s, char charwanted)
{
    register const char *scan;
    register const char *place;
    
    place = NULL;
    for (scan = s; *scan != '\0'; scan++)
        if (*scan == charwanted) place = scan;
    if (charwanted == '\0') return (char *)scan;
    return (char *)place;
}

#endif /* NO_STRCHR */


#ifdef __EMX__
extern "C" int _read_kbd(int, int, int);
#endif

int get_key(char *key)
{
#ifdef __EMX__
again:
  int c = _read_kbd(0, 1, 0);
  if (c == 0)
    switch (_read_kbd(0, 1, 0))
    {
    case 'G':
      *key = '<';
      break;
    case 'O':
      *key = '>';
      break;
    case 'H':
      *key = 'P' - 64;
      break;
    case 'P':
      *key = 'N' - 64;
      break;
    case 'I':
      *key = 'Z' - 64;
      break;
    case 'Q':
      *key = 'V' - 64;
      break;
    default:
      goto again;
    }
  else
    *key = c;
  return *key == -1 ? -1 : 1;
#else
  return read(0, key, 1);
#endif  
}
