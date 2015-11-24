/*
** command1.C - some of the commands called from the main command loop.
**
**               Command1.C and command2.C
**               are concatenated during the make into
**               commands.C, which consists of the main
**               command loop and all commands called from
**               within that loop.
**
** command1.C 1.103   Delta\'d: 09:49:38 9/28/92   Mike Lijewski, CNSF
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
#endif

#include <osfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __EMX__
#include <io.h>
#define chdir _chdir2
#define DEVNULL "nul"
#else
#define DEVNULL "/dev/null"
#endif

#include "dired.h"
#include "display.h"
#include "keys.h"

/*
** scroll_up_one_line - Scroll the listing up one line.
**                      We only call this routine when we KNOW that
**                      there is at least one line below the window
**                      which can be scrolled into it and the cursor
**                      is on the last line of the screen.
*/

static void scroll_up_one_line(DirList *dl)
{
    dl->setFirst(dl->firstLine()->next());
    dl->setLast(dl->lastLine()->next());
    dl->setCurrLine(dl->lastLine());

    if (CS)
    {
        scroll_listing_up_one();
        display_string(dl->currLine()->line(), dl->currLine()->length());
    }
    else if (DL || SF)
    {
        clear_modeline();
        scroll_screen_up_one();
        update_modeline();
        move_cursor(rows()-3, 0);
        display_string(dl->currLine()->line(), dl->currLine()->length());
    }
    else
        redisplay();

    dl->saveYXPos(rows()-3, goal_column(dl));
    move_cursor(rows()-3, dl->savedXPos());
}

/*
** scroll_down_one_line - Scroll the listing down one line.
**                        We only call this routine when we KNOW
**                        that the head\(\) of the listing is not visible
**                        and the cursor is on the first line in the window.
*/

static void scroll_down_one_line(DirList *dl)
{
    if (lines_displayed(dl) == rows() - 2)
        //
        // Must update lastLine.  We previously had a screenfull of lines.
        //
        dl->setLast(dl->lastLine()->prev());

    dl->setFirst(dl->firstLine()->prev());
    dl->setCurrLine(dl->firstLine());

    if (CS)
    {
        scroll_listing_down_one();
        display_string(dl->currLine()->line(), dl->currLine()->length());
    }
    else if (AL || SR)
    {
        clear_modeline();
        scroll_screen_down_one();
        update_modeline();
        cursor_home();
        display_string(dl->currLine()->line(), dl->currLine()->length());
    }
    else
        redisplay();

    dl->saveYXPos(0, goal_column(dl));
    move_cursor(0, dl->savedXPos());
}

/*
** scroll_up_full_window - scroll listing up one full window,
**                         leaving one line of overlap.  This routine
**                         is only called when we know that the tail\(\)
**                         of the listing is not currently displayed.
*/

static void scroll_up_full_window(DirList *dl)
{
    DirLine *ln = dl->lastLine();
    dl->setFirst(ln);
    dl->setCurrLine(ln);

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
    
    ln ? dl->setLast(ln->prev()) : dl->setLast(dl->tail());
    dl->saveYXPos(0, goal_column(dl));
    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(0, dl->savedXPos());
    
    synch_display();
}

/*
** scroll_down_full_window - try to scroll listing down one full window,
**                           with one line of overlap.  This routine is
**                           only called when we KNOW that there is at
**                           least one line "above" the current listing.
**                           Only change the current line if it flows off
**                           the "bottom" of the screen.  This routine is
**                           only called when we know that the head\(\) of the
**                           listing isn\'t currently displayed.
*/

static void scroll_down_full_window(DirList *dl)
{
    DirLine *ln = dl->firstLine();
    for (int y = 0; y < rows() - 3 && ln != dl->head(); y++, ln = ln->prev());

    //
    // y == # of lines preceding firstLine\(\) to add to screen
    //
    
    dl->setFirst(ln);
    cursor_home();
    for (int j = 0; j < rows()-2 && ln; j++, ln = ln->next())
    {
        clear_to_end_of_line();
        display_string(ln->line(), ln->length());
    }
    move_cursor(j, 0);
    for (; j < rows() - 2; j++)
    {
        clear_to_end_of_line();
        cursor_down();
    }
    
    if (ln) dl->setLast(ln->prev());
    
    if (dl->savedYPos()+y >= rows()-2)
    {
        dl->setCurrLine(dl->lastLine());
        dl->saveYXPos(rows()-3, goal_column(dl));
    }
    else
        dl->saveYXPos(dl->savedYPos()+y, dl->savedXPos());

    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(dl->savedYPos(), dl->savedXPos());
    
    synch_display();
}

/*
** scroll_up_half_window - scroll listing up half a window.  This routine
**                         is only called when the tail\(\) of the listing
**                         isn\'t being displayed.  We try to leave the
**                         cursor on the file it was on previously,
**                         otherwise it is left on the first file in
**                         the screen.
*/

static void scroll_up_half_window(DirList *dl, int y)
{
    if (dl->currLine()->length() > columns()) rightshift_current_line(dl);

    DirLine *ln = dl->firstLine();
    for (int i = 0; i < (rows() - 2)/2; i++, ln = ln->next()) ;
    dl->setFirst(ln);
    
    if (CS || DL || SF || DLN)
    {
        if (CS)
            scroll_listing_up_N((rows()-2)/2);
        else
        {
            clear_modeline();
            scroll_screen_up_N((rows()-2)/2);
            update_modeline();
        }
        move_cursor(rows() - 2 -((rows()-2)/2), 0);
        ln = dl->lastLine()->next();
        for (i = 0; i < (rows() - 2)/2 && ln; i++, ln = ln->next())
            display_string(ln->line(), ln->length());
        ln ? dl->setLast(ln->prev()) : dl->setLast(dl->tail());
    }
    else
    {
        
        cursor_home();
        for (i = 0; i < rows() - 2 && ln->next(); i++, ln = ln->next())
        {
            clear_to_end_of_line();
            display_string(ln->line(), ln->length());
        }
        
        if (i != rows()-2)
        {
            //
            // We hit last line before outputing all that we could.
            // Must output lastLine\(\) == tail\(\).
            //
            display_string(ln->line(), ln->length());
            dl->setLast(ln);
            i++;  // so we know how many lines have been written
        }
        else
            dl->setLast(ln->prev());

        // now clear any remaining rows on the screen
        move_cursor(i, 0);
        for (; i < rows() - 2; i++)
        {
            clear_to_end_of_line();
            cursor_down();
        }
    }

    int pos = y - (rows()-2)/2;
    if (pos < 0)
    {
        pos = 0;
        dl->setCurrLine(dl->firstLine());
    }
    
    dl->saveYXPos(pos, goal_column(dl));
    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(pos, dl->savedXPos());
    
    synch_display();
}

/*
** scroll_down_half_window - try to scroll listing down half a window.
**                           If `freshen\' is true, which is the default,
**                           the screen is refreshed.  It is important
**                           to note that we may not be able to scroll
**                           down a complete half window, since we
**                           always anchor the head of the listing to
**                           the first line in the screen.  This routine
**                           is only called when the head\(\) of the
**                           listing isn\'t being displayed.
*/

static void scroll_down_half_window(DirList *dl, int y, int freshen = 1)
{
    if (dl->firstLine() != dl->head())
    {
        //
        // We can scroll down.  Try to leave the cursor on the file
        // it started out on.  Otherwise, leave it on the
        // \(rows\(\)-2\)/2 line, which was the previous firstLine\(\).
        //
        DirLine *ln = dl->firstLine();
        for (int i = 0; i < (rows()-2)/2 && ln->prev(); i++, ln = ln->prev()) ;
        dl->setFirst(ln);

        if (dl->currLine()->length() > columns()) rightshift_current_line(dl);

        if (CS || AL || ALN || SR)
        {
            if (CS)
                scroll_listing_down_N(i);
            else
            {
                clear_modeline();
                scroll_screen_down_N(i);
                update_modeline();
                clear_message_line();
            }
            cursor_home();
            for (int j = 0; j < i; j++, ln = ln->next())
                display_string(ln->line(), ln->length());
            ln = dl->firstLine();
            for (int i = 0; i < rows()-2 && ln->next(); i++, ln = ln->next()) ;
            dl->setLast(ln);
        }
        else
        {
            cursor_home();
            for (int i = 0; i < rows()-2 && ln->next(); i++, ln = ln->next())
            {
                clear_to_end_of_line();
                display_string(ln->line(), ln->length());
            }
            
            if (i != rows() - 2)
            {
                //
                // We hit last line before outputing all that we could.
                // Must output lastLine\(\) == tail\(\).
                //
                display_string(ln->line(), ln->length());
                dl->setLast(ln);
                i++;  // so we know how many lines have been written
            }
            else
                dl->setLast(ln->prev());

            // now clear any remaining rows on the screen
            int tmp = i;
            move_cursor(i, 0);
            for (; i < rows()-2; i++)
            {
                clear_to_end_of_line();
                cursor_down();
            }
            i = tmp;  // because we use `i\' below
        }

        int pos = i + y;
        if (pos > rows() - 3)
        {
            pos = rows() - 3;
            dl->setCurrLine(dl->lastLine());
        }

        dl->saveYXPos(pos, goal_column(dl));
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(pos, dl->savedXPos());

        if (freshen) synch_display();
    }
}

/*
** goto_first - position cursor on first line in listing.  This routine
**              is not called if atBegOfList\(\) is true.
*/

static void goto_first(DirList *dl)
{
    if (dl->head() != dl->firstLine())
        initial_listing(dl);
    else
    {
        if (dl->currLine()->length() > columns()) rightshift_current_line(dl);
        dl->setCurrLine(dl->head());
    }

    dl->saveYXPos(0, goal_column(dl));
    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(0, dl->savedXPos());

    synch_display();
}

/*
** goto_last - position cursor on last file in listing.  This routine is
**             not called if atEndOfList\(\) is true.
*/

static void goto_last(DirList *dl)
{
    if (dl->currLine()->length() > columns()) rightshift_current_line(dl);

    dl->setCurrLine(dl->tail());

    if (dl->tail() == dl->lastLine())
    {
        // only need to reposition the cursor
        dl->saveYXPos(lines_displayed(dl) - 1, goal_column(dl));
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    else
    {
        // redisplay end of listing & update our view
        DirLine *ln = dl->tail();
        dl->setLast(ln);
        for (int i = 0; i < rows() - 2; i++, ln = ln->prev())
        {
            move_cursor(rows() - 3 - i, 0);
            clear_to_end_of_line();
            display_string(ln->line(), ln->length());
        }
        dl->setFirst(ln->next());
        dl->saveYXPos(rows() - 3,goal_column(dl));
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(rows() -3 , dl->savedXPos());
    }

    synch_display();
}

/*
** edit_prompted_for_directory - prompt for a directory to edit and edit
**                               it, provided it is a valid directory.
**                               If the first character of the directory is
**                               tilde \(`~\'\), the tilde is expanded to the
**                               user\'s home directory.
*/

static void edit_prompted_for_directory(DirList *dl)
{
#ifdef COMPLETION
    const char *dir = prompt("Directory to edit: ", 1); // prompt w/ completion
#else
    const char *dir = prompt("Directory to edit: ");
#endif

    if (dir == 0)
    {
        // command was aborted
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    if (*dir == '~') dir = expand_tilde(dir);

    if (!is_directory(dir))
    {
        message("`%' is not a valid directory name", dir);
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    if (!read_and_exec_perm(dir))
    {
        message("need read & exec permission to edit `%'", dir);
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    dired(dir);
}

/*
** reread_listing_line - rereads the listing line for the current line in the
**                       DirList, which refers to file, and then updates
**                       the listing line.
*/

static void reread_listing_line(DirList *dl, const char *file)
{
    String cmd = String(ls_cmd[the_sort_order()]) + file;

    FILE *fp = popen(cmd, "r");
    if (fp == 0)
        error("File %s, line %d: popen(%s) failed",
              __FILE__, __LINE__, (const char *)cmd);

    char *new_line = fgetline(fp);
    if (new_line == 0)
        error("File %s, line %d: fgetline() failed", __FILE__, __LINE__);
    (void)pclose(fp);

    dl->currLine()->update(&new_line);
}

/*
** edit_current_file - edit the current file in the DirList.
*/

static void edit_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    
    if (is_directory(file))
    {
        if (strcmp(file, ".") == 0 || (strcmp(file, "..") == 0 &&
                                       strcmp(dl->name(), "/") == 0))
            return;

        if (!read_and_exec_perm(file))
        {
            message("need read & exec permissions to edit `%'", file);
            move_cursor(dl->savedYPos(), dl->savedXPos());
            synch_display();
            return;
        }

        dired(file);
    }
    else if (is_regular_file(file))
    {
        char *editor = getenv("EDITOR");
        if (editor == 0) editor = "vi";
        String cmd = String(editor) + " " + file;

        //
        // Clear screen and position cursor in case the editor doesn\'t
        // do this itself.  This is primarily for those people who use
        // non-fullscreen editors, such as `ed\', which don\'t do this.
        //
        clear_display();
        move_to_message_line();
        synch_display();
        exec_with_system(cmd, 0);

        //
        // Re-read the listing line for this file
        // and insert into the directory listing.
        // This way, changes in file characteristics are
        // reflected in the soon-to-be updated listing.
        //
        reread_listing_line(dl, file);
        redisplay();
    }
    else
    {
        message("can only edit regular files and directories");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
    }
}

/*
** type_key_to_continue - ask user to type any key to continue
*/

static inline void type_any_key_to_continue()
{
    eat_a_character("Press Any Key to Continue");
}

/*
** page_current_file - attempt to page the current file in the DirList.
**                     If we have a directory, we edit it.
*/

static void page_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    if (is_directory(file))
    {
        if (strcmp(file, ".") == 0 || (strcmp(file, "..") == 0 &&
                                       strcmp(dl->name(), "/") == 0))
            return;

        if (!read_and_exec_perm(file))
        {
            message("need read & exec permissions to edit `%'", file);
            move_cursor(dl->savedYPos(), dl->savedXPos());
            synch_display();
            return;
        }

        dired(file);
    }
    else if (is_regular_file(file))
    {
        char *pager = getenv("PAGER");
        if (pager == 0) pager = "more";
        String cmd = String(pager) + " " + file;
        clear_display();
        move_to_message_line();
        synch_display();
        exec_with_system(cmd, 0);
        if (the_sort_order() == ACCESS_TIME ||
            the_sort_order() == INODE_CHANGE_TIME)
            //
            // Re-read the listing line for this file
            // and insert into the directory listing.
            // This way, changes in file characteristics are
            // reflected in the soon-to-be updated listing.
            // Since we\'ve only paged the file, the listing should only
            // change, if we\'re interested in the access or inode-
            // modification times.
            //
            reread_listing_line(dl, file); 
        redisplay();
    }
    else
    {
        message("can only page through regular files and directories");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
    }
}

/*
** insert_line - inserts the current line into the DirList after line y.
**               Only called if y != rows\(\)-3.  That is, this routine
**               is for the case when we don\'t need to scroll the screen
**               to place the line into its logical place on the screen.
*/

static void insert_line(DirList *dl, int y )
{
    if (CS)
    {
        insert_listing_line(y + 1);
        display_string(dl->currLine()->line(), dl->currLine()->length());
    }
    else if(AL)
    {
        clear_modeline();
        insert_blank_line(y + 1);
        display_string(dl->currLine()->line(), dl->currLine()->length());
        update_modeline();
    }
    else
        redisplay();
}

/*
** insert_into_dirlist - inserts the listing line for `dest\' into the
**                       DirList after line `y\' in the current window.
*/

static void insert_into_dirlist(DirList *dl, const char *dest, int y)
{
    char *pos = strrchr(dest, '/');
    //
    // pos is non-zero in those cases where `dest\' contains
    // `..\' trickery.  Say, ../this-dir/new-file.
    //
    String command = String(ls_cmd[the_sort_order()]) +
                     (pos ? pos + 1 : dest);

    FILE *fp = popen(command, "r");
    if (fp == 0)
        error("File %s, line %d: popen(%s) failed",
              __FILE__, __LINE__, (const char *)command);

    char *new_line = fgetline(fp);
    if (new_line == 0)
        error("File %s, line %d: fgetline() failed", __FILE__, __LINE__);
    (void)pclose(fp);
    
    int nlines = lines_displayed(dl);
    if (dl->currLine()->length() > columns()) rightshift_current_line(dl);
    dl->setCurrLine(dl->insert(&new_line));
    
    if (nlines == rows() - 2)
    {
        if (y == rows() - 3)
        {
            //
            // We must scroll listing up.
            //
            dl->setFirst(dl->firstLine()->next());
            dl->setLast(dl->currLine());

            if (CS)
            {
                scroll_listing_up_one();
                display_string(dl->currLine()->line(), dl->currLine()->length());
            }
            else if (DL || SF)
            {
                clear_modeline();
                scroll_screen_up_one();
                update_modeline();
                move_cursor(rows()-3, 0);
                display_string(dl->currLine()->line(), dl->currLine()->length());
            }
            else
                redisplay();

            dl->saveYXPos(y, goal_column(dl));
            move_cursor(y, dl->savedXPos());
        }
        else
        {
            //
            // Just insert line.
            //
            dl->setLast(dl->lastLine()->prev());
            insert_line(dl, y);
            dl->saveYXPos(y+1, goal_column(dl));
            move_cursor(y+1, dl->savedXPos());
        }
    }
    else
    {
        insert_line(dl, y);
        dl->saveYXPos(y + 1, goal_column(dl));
        move_cursor(y + 1, dl->savedXPos());
        if (nlines == y + 1)
            //
            // The new line becomes the new lastLine.
            //
            dl->setLast(dl->currLine());
    }

    if (dl->currLine()->length() > columns()) leftshift_current_line(dl);
}

/*
** yes_or_no - returns true if a \'y\' or \'Y\' is typed in response to
**             the msg.  Deals with SIGWINCH and SIGTSTP. Doesn\'t
**             synch the display.
*/

static int yes_or_no(const char *msg)
{
    message(msg);
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
            message(msg);
        }
        else
        {
            clear_message_line();
            return c == 'y' || c == 'Y';
        }

}

/*
** search - search through the line list, starting with the given DirLine,
**          for a line whose filename contains a match for `str\'.  The
**          matching is strictly a string match using `strstr\', not a
**          regex match.  Returns the first matching line found, or 0
**          if none found.  `pos\' is a pointer to an int containing
**          the position in the window of `ln\'.  On exit, it contains the
**          position of the matching line, relative to the starting position.
**          If `forward\' is nonzero \(the default\), we search forward,
**          otherwise we search backwards.  If str is NULL, we redo the
**          previous search.
*/

static DirLine *search(DirLine *ln, const char *str, int *pos, char forward=1)
{
    static String saved_str;
    const char *name;
    
    if (*str)
        //
        // Save copy of `str\' so we can redo searches.
        //
        saved_str = str;
    else if (saved_str != "")
        //
        // We have a previous search string.
        //
        str = saved_str;
    else
        return 0;

    while (ln)
    {
#ifndef NO_SYMLINKS
        if ((name = strstr(ln->line(), " -> ")) != 0)
            // we have a symbolic link
            --name;
        else
#endif
            name = ln->line() + ln->length();

        while (!isspace(*name)) --name;
        if (strstr(name, str)) return ln;

        if (forward)
        {
            ln = ln->next();
            (*pos)++;
        }
        else
        {
            ln = ln->prev();
            (*pos)--;
        }
    }

    return 0;
}

/*
** in_same_directory - returns true if the file `file\', which is
**                     relative to the DirList `dl\', is actually
**                     in the DirList\'s directory.
**                     Returns false on any error.
*/

static int in_same_directory(DirList *dl, const char *file)
{
    const char *last = strrchr(file, '/');
    if (last == 0) return 1;
    String dir(file);
    dir[last - file] = 0;
    struct stat stbuf1, stbuf2;
    if (stat(dir, &stbuf1) < 0 || stat(dl->name(), &stbuf2) < 0) return 0;
    return stbuf1.st_ino == stbuf2.st_ino && stbuf1.st_dev == stbuf2.st_dev;
}

/*
** copy_file - copy current file to destination. Update window appropriately.
*/

static void copy_file(DirList *dl, const char *src, const char *dest)
{
    const char *cp = "cp";
    const char *args[4];
    args[0] = cp; args[1] = src; args[2] = dest; args[3] = 0;

    if (is_regular_file(dest))
    {
        String msg = String("overwrite '") + dest + "'? (y|n) ";
        if (yes_or_no(msg))
            if (execute(cp, args))
            {
                message("`copy' was successful");
                if (in_same_directory(dl, dest))
                {
                    //
                    // `dest\' is in our current directory.
                    // It may contain /\'s due to .. or ~ funny business.
                    //
                    int pos = 0;
                    const char *slash = strrchr(dest, '/');
                    if (slash) dest   = slash + 1;
                    DirLine *found    = search(dl->firstLine(), dest, &pos);
                    if (found && pos < rows() - 2)
                    {
                        //
                        // And it\'s also in our current window.
                        //
                        String cmd = String(ls_cmd[the_sort_order()]) + dest;

                        FILE *fp = popen(cmd, "r");
                        if (fp == 0)
                            error("File %s, line %d: popen(%s) failed",
                                  __FILE__, __LINE__, (const char *)cmd);

                        char *new_line = fgetline(fp);
                        if (new_line == 0)
                            error("File %s, line %d: fgetline() failed",
                                  __FILE__, __LINE__);
                        (void)pclose(fp);
            
                        update_screen_line(found->line(), new_line, pos);
                        found->update(&new_line);
                    }
                }
            }
            else
                message("`copy' failed");

        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    else if (is_directory(dest))
    {
        String file(dest);
        if (dest[strlen(dest) - 1] != '/') file += "/";
        file += src;

        if (is_regular_file(file))
        {
            String msg = String("overwrite `") + file + "'? (y|n) ";
            if (yes_or_no(msg))
                if (execute(cp, args))
                    message("`copy' was successful");
                else
                    message("`copy' failed");
        }
        else
        {
            //
            // Just do the `cp\'.
            //
            if (execute(cp, args))
                message("`copy' was successful");
            else
                message("`copy' failed");
        }

        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    else
    {
        // `dest\' doesn\'t already exist
        if (execute(cp, args))
        {
            message("`copy' was successful");
            //
            // Is `dest\' in our current directory?
            //
            if (in_same_directory(dl, dest))
                insert_into_dirlist(dl, dest, dl->savedYPos());
            else
                move_cursor(dl->savedYPos(), dl->savedXPos());
        }
        else
        {
            message("`copy' failed");
            move_cursor(dl->savedYPos(), dl->savedXPos());
        }
    }
}

/*
** copy_current_file - attempt to copy current file to another
*/

static void copy_current_file(DirList *dl)
{
    const char *src = get_file_name(dl);
    if (strcmp(src, ".")  == 0 || strcmp(src, "..") == 0)
    {
        message("`copying' of `.' and `..' is not allowed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
#ifdef COMPLETION
    const char *dest = prompt("Copy to: ", 1);  // prompt with completion
#else
    const char *dest = prompt("Copy to: ");
#endif
    
    if (dest == 0)
    {
        // command was aborted
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*dest == 0)
    {
        message("not a valid file name");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    if (*dest == '~') dest = expand_tilde(dest);
    copy_file(dl, src, dest);
    synch_display();
}

/*
** link_file - attempt to link `file\' to `dest\'.
*/

static void link_file(DirList *dl, const char *file, const char *dest)
{
#ifndef NO_LINKS
    if (!link(file, dest))
    {
        message("`link' was successful");
        if (in_same_directory(dl, dest))
            insert_into_dirlist(dl, dest, dl->savedYPos());
        else
            move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    else
    {
        message("`link' failed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
#endif
}

/*
** link_current_file - attempt to make a link to the current file
*/

static void link_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    const char *link = prompt("Link to: ");

    if (link == 0)
    {
        // command was aborted
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*link == 0)
    {
        message("not a valid file name");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    if (*link == '~') link = expand_tilde(link);
    link_file(dl, file, link);
    synch_display();
}

#ifndef NO_SYMLINKS
/*
** symlink_file - symbolically link `file\' to `dest\'
*/

static void symlink_file(DirList *dl, const char *file, const char *dest)
{
    if (!symlink(file, dest))
    {
        message("`symlink' was successful");
        if (in_same_directory(dl, dest))
            insert_into_dirlist(dl, dest, dl->savedYPos());
        else
            move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    else
    {
        message("`symlink' failed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
}

/*
** symlink_current_file - attempt create a symbolic link to the current file
*/

static void symlink_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    const char *link = prompt("Name of symbolic link: ");
    
    if (link == 0)
    {
        // command was aborted
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*link == 0)
    {
        message("not a valid file name");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    if (*link == '~') link = expand_tilde(link);
    symlink_file(dl, file, link);
    synch_display();
}
#endif


/*
** remove_listing_line - delete the current line in the DirList
**                       and update both the screen and data
**                       structures appropriately.  `y\' is the position
**                       in the window of the current line.
*/

static void remove_listing_line(DirList *dl, int y)
{
    if (dl->lastLine() != dl->tail())
    {
        //
        // Last line in listing isn\'t in window, so we\'ll have to set
        // a new one.  We can just scroll up one line.
        //
        dl->setLast(dl->lastLine()->next());
        dl->deleteLine();

        if (CS || DL)
        {
            if (CS)
                delete_listing_line(y);
            else
            {
                clear_modeline();
                delete_screen_line(y);
                update_modeline();
            }
            move_cursor(rows()-3, 0);
            display_string(dl->lastLine()->line(), dl->lastLine()->length());
        }
        else
        {
            move_cursor(y, 0);
            DirLine *ln = dl->currLine();
            for (int i = y; i < rows()-2; i++, ln = ln->next())
	    {
	        clear_to_end_of_line();
                display_string(ln->line(), ln->length());
	    }
            update_modeline();
        }

        dl->saveYXPos(y, goal_column(dl));
    }
    else
    {
        //
        // Last line of listing is visible in window,
        // so firstLIne\(\) and lastLine\(\) won\'t change.
        //
        if (dl->atWindowTop() && dl->atWindowBot())
        {
            //
            // The last line in the window is also the first line.
            // Since we don\'t allow deletion of `.\' or `..\', there
            // must be more viewable lines.  Scroll down half
            // a window to put more into view.
            //
            scroll_down_half_window(dl, y, 0);
            dl->deleteLine();
            DirLine *ln = dl->firstLine();
            for (int pos = 0; ln != dl->tail(); pos++, ln = ln->next()) ;
            dl->saveYXPos(pos, goal_column(dl));
            move_cursor(pos+1, 0);
            clear_to_end_of_line();
            move_cursor(pos, dl->savedXPos());
        }
        else if (dl->atWindowBot())
        {
            //
            // We want to delete the last line in the window.
            //
            dl->deleteLine();
            move_cursor(y, 0);
            clear_to_end_of_line();
            dl->saveYXPos(y-1, goal_column(dl));
            move_cursor(y-1, dl->savedXPos());
        }
        else
        {
            //
            // We are in the middle of the listing.
            //
            dl->deleteLine();
            if (CS || DL)
            {
                if (CS)
                    delete_listing_line(y);
                else
                {
                    clear_modeline();
                    delete_screen_line(y);
                    update_modeline();
                }
            }
            else
            {
                clear_to_end_of_screen(y);
                move_cursor(y, 0);
                for (DirLine *ln = dl->currLine(); ln; ln = ln->next())
                    display_string(ln->line(), ln->length());
                update_modeline();
            }
            dl->saveYXPos(y, goal_column(dl));
         }
    }
    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(dl->savedYPos(), dl->savedXPos());
}
/*
** command2.C - the main command loop and some of the commands themselves.
**
**               Command1.C and command2.C
**               are concatenated during the make into
**               commands.C, which consists of the main
**               command loop and all commands called from
**               within that loop.
**
** command2.C 1.42   Delta\'d: 09:49:45 9/28/92   Mike Lijewski, CNSF
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

/*
** delete_file - delete the current file which is a regular file.
**               Update the window appropriately.
*/

static void delete_file(DirList *dl, const char *file)
{
    if (unlink(file) != -1)
        remove_listing_line(dl, dl->savedYPos());
    else
    {
        message("`deletion' failed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
}

/*
** delete_directory - delete the current file which is a directory.
**                    Update the window appropriately.
*/

static void delete_directory(DirList *dl, const char *dirname)
{
    if (rmdir(dirname) != -1)
        remove_listing_line(dl, dl->savedYPos());
    else
    {
        message("`deletion' failed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
}

/*
** delete_current_file - attempt to delete current file.
*/

static void delete_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    
    if (strcmp(file, ".")  == 0 || strcmp(file, "..") == 0)
    {
        message("`deletion' of `.' and `..' is not allowed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    String msg = String("delete `") + file + "'? (y|n) ";

    if (is_directory(file) && yes_or_no(msg))
        delete_directory(dl, file);
    else if (yes_or_no(msg))
        delete_file(dl, file);
    else
        //
        // Don\'t do the delete.
        //
        move_cursor(dl->savedYPos(), dl->savedXPos());
    
    synch_display();
}


/*
** update_current_line - updates the current line in the listing
**                       and it\'s representation on the screen.
*/

static void update_current_line(DirList *dl, char *new_line)
{
    if (strlen(new_line) <= columns() && dl->currLine()->length() <= columns())
    {
        //
        // The most common case.
        //
        update_screen_line(dl->currLine()->line(), new_line, dl->savedYPos());
        dl->currLine()->update(&new_line);
        dl->saveYXPos(dl->savedYPos(), goal_column(dl));
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    else
    {
        //
        // Either the old or the new line must be shifted.
        // Probably not worth trying to use update_screen_line\(\).
        //
        dl->currLine()->update(&new_line);
        dl->saveYXPos(dl->savedYPos(), goal_column(dl));
        
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
        {
            move_cursor(dl->savedYPos(), 0);
            clear_to_end_of_line();
            display_string(new_line);
            move_cursor(dl->savedYPos(), dl->savedXPos());
        }
    }
}

/*
** rename_file - rename src to dest and update window appropriately.
*/

static void rename_file(DirList *dl, const char *src, const char *dest)
{
    if (rename(src, dest) != -1)
    {
        message("`rename' was successful");
        if (in_same_directory(dl, dest))
        {
            //
            // The important point to note about the following code
            // is that if `dest\' is a directory, we need to add a `d\'
            // to the ls command as in
            //
            //   ls -ld source
            //
            int is_dir = is_directory(dest);
            size_t size = strlen(ls_cmd[the_sort_order()]) + strlen(dest) + 3;

            if (is_dir) size++;

            String command = ls_cmd[the_sort_order()];
            if (is_dir) command += "-d";
            command += String(" ") + dest;

            FILE *fp = popen(command, "r");
            if (fp == 0)
                error("File %s, line %d: popen(%s) failed",
                      __FILE__, __LINE__, (const char *)command);

            char *new_line = fgetline(fp);
            if (new_line == 0)
                error("File %s, line %d: fgetline() failed",
                      __FILE__, __LINE__);
            (void)pclose(fp);

            update_current_line(dl, new_line);
        }
        else
            remove_listing_line(dl, dl->savedYPos());
    }
    else
    {
        message("`rename' failed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
}

/*
** rename_current_file - attempt to rename the current file
*/

static void rename_current_file(DirList *dl)
{
    const char *from_file = get_file_name(dl);
    
    if (strcmp(from_file, ".")  == 0 || strcmp(from_file, "..") == 0)
    {
        message("`renaming' of `.' and `..' is not allowed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    const char *to_file = prompt("Rename to: ");
    
    if (to_file == 0)
    {
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*to_file == 0)
    {
        message("not a valid file name");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }

    if (*to_file == '~') to_file = expand_tilde(to_file);
    rename_file(dl, from_file, to_file);
    synch_display();
}

/*
** compress_file - compress the current file and update the window.
*/

static void compress_file(DirList *dl, const char *file)
{
    String cmd = String("compress") + " -f " + file + " 1>" + DEVNULL + " 2>&1";

    if (!system(cmd))
    {
        message("Compressing ... ");

        String command = String(ls_cmd[the_sort_order()]) + file +
                         ".Z 2>" + DEVNULL;

        FILE *fp = popen(command, "r");
        if (fp == 0)
            error("File %s, line %d: popen(%s) failed",
                  __FILE__, __LINE__, (const char *)command);

        char *new_line = fgetline(fp);
        if (new_line)
            //
            // Only update the line if we were able to read the line.
            // Some versions of compress don\'t properly return a failure
            // code if they don\'t compress the file.  Hence we probably
            // still have the uncompressed file lying around.
            //
            update_current_line(dl, new_line);
        (void)pclose(fp);

        message("Compressing ... done");
    }
    else
        message("`compress' failed");
}

/*
** compress_current_file - attempt to compress current file
*/

static void compress_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);

    //
    // Disallow compressing of symbollically linked files.
    //
    if (strstr(&(dl->currLine()->line())[dl->savedXPos()], " -> "))
    {
        message("compress'ing symbollically linked files not allowed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    if (file[strlen(file)-1] == 'Z' && file[strlen(file) - 2] == '.')
    {
        message("file appears to already be compressed");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    if (is_regular_file(file))
        compress_file(dl, file);
    else
        message("can only `compress' regular files");
    move_cursor(dl->savedYPos(), dl->savedXPos());
    synch_display();
}

/*
** chgrp_file - change the group of the current file.
**              Update the window appropriately.
*/

static void chgrp_file(DirList *dl, const char *file, const char *group)
{
    const char *chgrp  = "chgrp";
    const char *args[4];
    args[0] = chgrp; args[1] = group; args[2] = file; args[3] = 0;

    if (execute(chgrp, args))
    {
        String command = ls_cmd[the_sort_order()];
        if (is_directory(file)) command += "-d";
        command += String(" ") + file;

        FILE *fp = popen(command, "r");
        if (fp == 0)
            error("File %s, line %d: popen(%s) failed",
                  __FILE__, __LINE__, (const char *)command);

        char *new_line = fgetline(fp);
        if (new_line == 0)
            error("File %s, line %d: fgetline() failed", __FILE__, __LINE__);
        (void)pclose(fp);

        update_current_line(dl, new_line);
    }
    else
        message("`chgrp' failed");
}

/*
** chgrp_current_file - attempt to chgrp current file
*/

static void chgrp_current_file(DirList *dl)
{
    const char *file  = get_file_name(dl);
    const char *group = prompt("Change to group: ");
    
    if (group == 0)
    {
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*group == 0)
    {
        message("not a valid group");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    chgrp_file(dl, file, group);
    move_cursor(dl->savedYPos(), dl->savedXPos());
    synch_display();
}

/*
** chmod_file - change the mode of the current file.
**              Update the window appropriately.
*/

static void chmod_file(DirList *dl, const char *file, const char *mode)
{
    const char *chmod = "chmod";
    const char *args[4];
    args[0] = chmod; args[1] = mode; args[2] = file; args[3] = 0;

    if (execute(chmod, args))
    {
        String command = ls_cmd[the_sort_order()];
        if (is_directory(file)) command += "-d";
        command += String(" ") + file;

        FILE *fp = popen(command, "r");
        if (fp == 0)
            error("File %s, line %d: popen(%s) failed",
                  __FILE__, __LINE__, (const char *)command);

        char *new_line = fgetline(fp);
        if (new_line == 0)
            error("File %s, line %d: fgetline() failed", __FILE__, __LINE__);
        (void)pclose(fp);

        update_current_line(dl, new_line);
    }
    else
        message("`chmod' failed");
}

/*
** chmod_current_file - attempt to chmod the current file
*/

static void chmod_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    const char *mode = prompt("Change to mode: ");
    
    if (mode == 0)
    {
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*mode == 0)
    {
        message("not a valid file mode");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    chmod_file(dl, file, mode);
    
    move_cursor(dl->savedYPos(), dl->savedXPos());
    synch_display();
}

/*
** print_current_file - attempt to print current file
*/

static void print_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    char *printer    = getenv("DIREDPRT");
#ifdef __EMX__
    if (printer == 0) printer = "print";
#else
    if (printer == 0) printer = "lpr";
#endif
    String cmd = String(printer) + " " + file;
    
    if (is_regular_file(file))
        if (!system(cmd))
            message("`%s' printed successfully", file);
        else
            message("print of `%s' failed", file);
    else
        message("`%s' isn't a regular file", file);
    
    move_cursor(dl->savedYPos(), dl->savedXPos());
    synch_display();
}

/*
** uncompress_file - uncompress the file at line y in the DirList.
**                   Update the window appropriately.
*/

static void uncompress_file(DirList *dl, const char *file)
{
    String cmd = String("compress -d") + " " + file 
               + " 1>" + DEVNULL + " 2>&1";

    if (!system(cmd))
    {
        message("Uncompressing ... ");

        char *dot = strrchr(file, '.');
        *dot = 0;  // remove .Z suffix

        String command = String(ls_cmd[the_sort_order()]) + file +
                         " 2>" + DEVNULL;

        FILE *fp = popen(command, "r");
        if (fp == 0)
           error("File %s, line %d: popen(%s) failed",
                  __FILE__, __LINE__, (const char *)command);            

        char *new_line = fgetline(fp);
        if (new_line)
            //
            // Only update the line if we were able to read the line.
            // Some versions of compress don\'t properly return a failure
            // code if they don\'t uncompress the file.  Hence we probably
            // still have the .Z file lying around.
            //
            update_current_line(dl, new_line);
        (void)pclose(fp);

        message("Uncompressing ... done");
    }
    else
        message("`uncompress' failed");
}

/*
** uncompress_current_file - attempt to uncompress current file
*/

static void uncompress_current_file(DirList *dl)
{
    const char *file = get_file_name(dl);
    
    if (file[strlen(file)-1] != 'Z' && file[strlen(file)-1] != '.')
    {
        message("can only `uncompress' compressed files");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    uncompress_file(dl, file);

    move_cursor(dl->savedYPos(), dl->savedXPos());
    synch_display();
}

/*
** search_forward - search forward for file or directory matching string.
**                  The search always starts with the file following
**                  the current file.  If a match occurs, the cursor
**                  is placed on the new current file, with that line
**                  placed at the top of the window.
*/

static void search_forward(DirList *dl)
{
    const char *str = prompt("Search forward for: ");
    if (str == 0)
    {
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }                  
    
    DirLine *ln = dl->currLine();
    if (ln == dl->tail())
    {
        message("no match");
        move_cursor(dl->savedYPos(), dl->savedXPos());
        return;
    }
    
    int pos = dl->savedYPos()+1;
    DirLine *found = search(ln->next(), str, &pos);
    if (found)
    {
        if (dl->currLine()->length() > columns()) rightshift_current_line(dl);
        dl->setCurrLine(found);
    }
    
    if (found && pos < rows()-2)
    {
        //
        // We found a match in our current window.
        // Only need to update the cursor position.
        //
        dl->saveYXPos(pos, goal_column(dl));
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(pos, dl->savedXPos());
    }
    else if (found)
    {
        //
        // We found a match, but it is out of our current view.
        // Place the matched line at the top of the window.
        //
        ln = found;
        dl->setFirst(ln);
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

        ln ? dl->setLast(ln->prev()) : dl->setLast(dl->tail());
        dl->saveYXPos(0, goal_column(dl));
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(0, dl->savedXPos());
    }
    else
    {
        message("no match");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    
    synch_display();
}

/*
** search_backward - search backward for file or directory matching string.
**                   The search always starts with the file following
**                   the current file.  If a match occurs, the cursor
**                   is placed on the new current file, with that line
**                   placed at the top of the window.
*/

static void search_backward(DirList *dl)
{
    const char *str = prompt("Search backward for: ");
    if (str == 0)
    {
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    
    DirLine *ln = dl->currLine();
    if (ln == dl->head()) return;
    
    int pos = dl->savedYPos()-1;
    DirLine *found = search(ln->prev(), str, &pos, 0);
    if (found)
    {
        if (dl->currLine()->length() > columns()) rightshift_current_line(dl);
        dl->setCurrLine(found);
    }
    
    if (found && pos >= 0)
    {
        //
        // We found a match in our current window.
        // Only need to update the cursor position.
        //
        dl->saveYXPos(pos, goal_column(dl));
        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(pos, dl->savedXPos());
    }
    else if (found)
    {
        //
        // We found a match, but it is out of our
        // current view.  Place the matched line at the top
        // of the window.  Since we\'re searching backwards
        // and the match is out of our present window, we
        // must have a whole new window to display.
        //
        dl->setFirst(ln = found);
        cursor_home();
        for (int i = 0; i < rows() - 2 && ln->next(); i++, ln = ln->next())
        {
            clear_to_end_of_line();
            display_string(ln->line(), ln->length());
        }
        
        dl->setLast(ln->prev());
        dl->saveYXPos(0, goal_column(dl));

        if (dl->currLine()->length() > columns())
            leftshift_current_line(dl);
        else
            move_cursor(0, dl->savedXPos());
    }
    else
    {
        message("no match");
        move_cursor(dl->savedYPos(), dl->savedXPos());
    }
    synch_display();
}

//
// Help message for the message window when displaying help.
//
const char *const HELP_MSG[] = {
    "Space scrolls forward.  Other keys quit.",
    "Space forward, Backspace backward.  Other keys quit.",
    "Backspace scrolls backward.  Other keys quit."
};

/*
** help - give some help.  Deal with SIGWINCH and SIGTSTP.
*/

static void help(DirList *dl)
{
    update_modeline("----- HELP");
    
    int position = 0;
    char key;
    do
    {
        cursor_home();
        for (int i = 0; i < rows() - 2 && i + position < HELP_FILE_DIM; i++)
        {
            clear_to_end_of_line();
            display_string(help_file[position + i]);
        }
        move_cursor(i, 0);
        for (; i < rows() - 2; i++)
        {
            clear_to_end_of_line();
            cursor_down();
        }
        clear_message_line();

        if (position + rows() -2 >= HELP_FILE_DIM)
            //
            // The tail of the help message.
            //
            (void)fputs(HELP_MSG[2], stdout);
        else if (position == 0)
            //
            // The head of the help message.
            //
            (void)fputs(HELP_MSG[0], stdout);
        else
            //
            // Somewhere in between.
            //
            (void)fputs(HELP_MSG[1], stdout);

        synch_display();

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
        }
        else if (key == KEY_SPC)
        {
            if (position >= HELP_FILE_DIM - 1) goto finished;
            position += rows() - 2;
        }
        else if (key == *BC)
        {
            if (position == 0) goto finished;
            position -= rows() - 2;
        }
        else 
            goto finished;  // return to the listing
    }
    while (position < HELP_FILE_DIM - 1);

  finished:
    update_modeline(modeline_prefix, dl->name());
    redisplay();
}

/*
** reread_current_directory - reread the current directory and
**                            put it up fresh on the screen.  We
**                            attempt to put the cursor back onto
**                            the previous current file, if that file
**                            still exists.
*/

static void reread_current_directory(DirList * &dlr)
{
    char *dirname = new char[strlen(dlr->name()) + 1];
    (void)strcpy(dirname, dlr->name());
    const char *old_current_file = get_file_name(dlr);

    int old_pos;  // position in old DirList of old_current_file
    DirLine *ln = dlr->head();
    for (old_pos = 0; ln != dlr->currLine(); ln = ln->next(), old_pos++) ;

    delete dir_stack->pop();
    
    DirList *dl = get_directory_listing(dirname);
    if (dl == 0)
        error("File %s, line %d: couldn't read directory `%s'",
              __FILE__, __LINE__, dirname);

    dlr = dl;  // update dir_list in read_commands\(\)

    dir_stack->push(dl);
    
    ln = dl->head();
    int pos = 0;
    const char *name;
    int namelen = 0;
    while (ln)
    {

#ifndef NO_SYMLINKS
        if ((name = strstr(ln->line(), " -> ")) != 0)
            // we have a symbolic link
            --name;
        else
#endif
            name = ln->line() + ln->length();

        for (namelen = 0; isspace(*name) == 0; name--, namelen++) ;

        if (strncmp(name + 1, old_current_file, namelen) == 0) break;

        ln = ln->next();
        pos++;
        namelen = 0;
    }

    if (ln == 0)
    {
        //
        // Update ln and pos to refer to a suitable file on
        // which to place cursor.
        //
        ln = dl->head();
        for (pos = 0; pos < old_pos && ln; ln = ln->next(), pos++) ;
    }

    dl->setCurrLine(ln);

    if (pos < rows() - 2)
    {
        //
        // Display starting at the head.
        //
        dl->setFirst(ln = dl->head());
        dl->saveYXPos(pos, goal_column(dl));
    }
    else
    {
        //
        // Center dl->currLine.
        //
        ln = dl->currLine();
        for (int i = 0; i < (rows() - 1)/2; i++, ln = ln->prev()) ;
        dl->setFirst(ln);
        dl->saveYXPos((rows() - 1)/2, goal_column(dl));
    }

    cursor_home();
    for (int i = 0; i < rows() - 2 && ln; ln = ln->next(), i++)
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

    ln ? dl->setLast(ln->prev()) : dl->setLast(dl->tail());
    
    if (dl->currLine()->length() > columns())
        leftshift_current_line(dl);
    else
        move_cursor(dl->savedYPos(), dl->savedXPos());
    
    synch_display();
}

/*
** expand_percent - expand any percent signs in cmd to file.  The result is
**                  stored in volatile storage.
*/

static const char *expand_percent(const char *cmd, const char *file)
{
    static char *expanded_cmd;
    DELETE expanded_cmd;

    if (!strchr(cmd, '%')) return cmd;

    expanded_cmd  = new char[strlen(cmd) + 1];
    (void)strcpy(expanded_cmd, cmd);
    char *pos = strchr(expanded_cmd, '%');

    while (pos)
    {
        char *full_cmd = new char[strlen(expanded_cmd) + strlen(file)];
        (void)strncpy(full_cmd, expanded_cmd, pos - expanded_cmd);
        full_cmd[pos - expanded_cmd] = '\0';
        (void)strcat(full_cmd, file);
        (void)strcat(full_cmd, pos + 1);
        DELETE expanded_cmd;
        expanded_cmd = full_cmd;
        pos = strchr(expanded_cmd, '%');
    }

    return expanded_cmd;
}

/*
** shell_command - execute a shell command.  % is expanded to
**                 the current filename.
**                 If *cmd == 0, start up a shell.
**                 If *cmd == !, reexecute most recent shell command.
*/

static void shell_command(DirList *dl)
{
    static String original_cmd;
    static String saved_cmd;
    static String saved_shell;
    const char *file = get_file_name(dl);
    const char *cmd  = prompt("!");
    char *shell;

    if (cmd == 0)
    {
        //
        // command aborted
        //
        move_cursor(dl->savedYPos(), dl->savedXPos());
        synch_display();
        return;
    }
    else if (*cmd == '\0')
    {
        //
        // start up a shell
        //
        if (saved_shell == "") 
	{
#ifdef __EMX__
	    shell = getenv("COMSPEC");
	    if (shell == NULL) shell = "cmd.exe";
#else
	    shell = getenv("SHELL");
	    if (shell == NULL) shell = "sh";
#endif
	    saved_shell = shell;
	}
        saved_cmd = original_cmd = saved_shell;

        const char *slash = strrchr(saved_shell, '/');
        const char *args[2];
        args[0] = slash ? slash + 1 : (const char *)saved_shell;
        args[1] = 0;

        message("Starting interactive shell ...");

        cursor_wrap();
        synch_display();
        unsetraw();
        unset_signals();

        execute(saved_shell, args, 0);

        set_signals();
        setraw();
    }
    else if (*cmd == '[')
    {
        //
        // Re-expand previous original command, if it contains a %.
        // Otherwise, re-execute previous saved command since the original
        // and saved will be the same.
        //
        if (original_cmd != "")
        {
            //
            // expand the `%\'
            //
            saved_cmd = expand_percent(original_cmd, file);
            message(saved_cmd);
            cursor_wrap();
            synch_display();
            exec_with_system(saved_cmd);
        }
        else
        {
            message("No Previous Shell Command");
            move_cursor(dl->savedYPos(), dl->savedXPos());
            synch_display();
            return;
        }
    }
    else if (*cmd == '!')
    {
        //
        //  re-execute previously saved command
        //
        if (saved_cmd != "")
        {
            message(saved_cmd);
            cursor_wrap();
            synch_display();
            exec_with_system(saved_cmd);
        }
        else
        {
            message("No Previous Shell Command");
            move_cursor(dl->savedYPos(), dl->savedXPos());
            synch_display();
            return;
        }
    }
    else
    {
        //
        // expand and then execute command
        //
        original_cmd = cmd;
        saved_cmd = expand_percent(original_cmd, file);
        message(saved_cmd);
        cursor_wrap();
        synch_display();
        exec_with_system(saved_cmd);
    }
    redisplay();
}

/*
** change_sort_order - query the user for a new sorting order - expects to
**                     read a single character - and change the sort order
**                     to that value, if it is a valid value.  Deals with
**                     SIGWINCH and SIGTSTP.
*/

static void change_sort_order(DirList *dl)
{
    const char *msg = "Set sort order to? [a, c, t, u]: ";
    message(msg);

    char key;
    while (1)
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
            // Redisplay the message - termstop\(\) takes care of the rest
            // in the case of SIGTSTP.
            //
            message(msg);
        }
    else
        switch(key)
        {
          case 'a':
            set_sort_order(ALPHABETICALLY);
            message("Sorting `alphabetically' now.");
            goto end;
          case 'c':
            set_sort_order(INODE_CHANGE_TIME);
            message("Sorting by `inode-change time' now.");
            goto end;
          case 't':
            set_sort_order(MODIFICATION_TIME);
            message("Sorting by `modification time' now.");
            goto end;
          case 'u':
            set_sort_order(ACCESS_TIME);
            message("Sorting by `access time' now.");
            goto end;
          default:
            message("Sort order not changed.");
            goto end;
        }
  end:
    move_cursor(dl->savedYPos(), dl->savedXPos());
    synch_display();
}

/*
** read_commands - the command loop
*/

void read_commands(DirList *dir_list)
{
    int key;
    for (;;)
    {
        switch (key = get_key(dir_list))
        {
          case KEY_j         :
          case KEY_n         :
          case KEY_CTL_N     :
          case KEY_SPC       :
          case KEY_CR        :
          case KEY_ARROW_DOWN:

              if (dir_list->atEndOfList()) break;

              if (dir_list->currLine()->length() > columns()) 
                  rightshift_current_line(dir_list);
    
              if (dir_list->savedYPos() < rows() - 3)
              {
                  //
                  // There are still more lines below us in the window
                  // so we just move the cursor down one line.
                  //
                  dir_list->setCurrLine(dir_list->currLine()->next());
                  int x = goal_column(dir_list);
                  if (x == dir_list->savedXPos())
                      cursor_down();
                  else
                      move_cursor(dir_list->savedYPos() + 1, x);
                  dir_list->saveYXPos(dir_list->savedYPos() + 1, x);
              }
              else
                  //
                  // We are on the last line on the screen and there
                  // are more lines to display.  Scroll up one line
                  // and leave the cursor on the next logical line.
                  //
                  scroll_up_one_line(dir_list);

              if (dir_list->currLine()->length() > columns())
                  leftshift_current_line(dir_list);

              synch_display();
              break;

          case KEY_k       :
          case KEY_p       :
          case KEY_CTL_P   :
          case KEY_CTL_Y   :
          case KEY_ARROW_UP:

              if (dir_list->atBegOfList()) break;

              if (dir_list->currLine()->length() > columns())
                  rightshift_current_line(dir_list);

              if (dir_list->savedYPos() != 0)
              {
                  // We aren\'t at the top of the window so can mOve up.
                  dir_list->setCurrLine(dir_list->currLine()->prev());
                  int x = goal_column(dir_list);
                  if (x == dir_list->savedXPos() && UP)
                      cursor_up();
                  else
                      move_cursor(dir_list->savedYPos() - 1, x);
                  dir_list->saveYXPos(dir_list->savedYPos() - 1, x);
              }
              else
                  //
                  // We are on the first line of the window and there are
                  // lines preceding us in the directory listing.
                  //
                  scroll_down_one_line(dir_list);

              if (dir_list->currLine()->length() > columns())
                  leftshift_current_line(dir_list);

              synch_display();
              break;

          case KEY_CTL_F:
          case KEY_CTL_V:

              if (dir_list->lastLine() == dir_list->tail()) break;
              scroll_up_full_window(dir_list);

              break;
          case KEY_b    :
          case KEY_CTL_B:
          case KEY_CTL_Z:

              if (dir_list->firstLine() == dir_list->head()) break;
              scroll_down_full_window(dir_list);

              break;
          case KEY_CTL_D:

              if (dir_list->lastLine() == dir_list->tail()) break;
              scroll_up_half_window(dir_list, dir_list->savedYPos());

              break;
          case KEY_CTL_U:

              if (dir_list->firstLine() == dir_list->head()) break;
              scroll_down_half_window(dir_list, dir_list->savedYPos());

              break;
          case KEY_TOP:

              if (dir_list->atBegOfList()) break;
              goto_first(dir_list);

              break;
          case KEY_BOT:

              if (dir_list->atEndOfList()) break;
              goto_last(dir_list);

              break;
          case KEY_e:
          case KEY_f:

              edit_current_file(dir_list); break;

          case KEY_m:
          case KEY_v:

              page_current_file(dir_list); break;

          case KEY_c:

              copy_current_file(dir_list); break;

          case KEY_d:

              delete_current_file(dir_list); break;

          case KEY_r:

              rename_current_file(dir_list); break;

          case KEY_C:

              compress_current_file(dir_list); break;

          case KEY_E:

              edit_prompted_for_directory(dir_list); break;

          case KEY_G:

              chgrp_current_file(dir_list); break;

          case KEY_QM:
          case KEY_H :

              help(dir_list); break;

          case KEY_L:

              link_current_file(dir_list); break;

          case KEY_M:

              chmod_current_file(dir_list); break;

          case KEY_O:

              change_sort_order(dir_list); break;

          case KEY_P:

              print_current_file(dir_list); break;

          case KEY_g:
          case KEY_R:

              reread_current_directory(dir_list); break;

#ifndef NO_SYMLINKS
          case KEY_S:

              symlink_current_file(dir_list); break;
#endif

          case KEY_U:

              uncompress_current_file(dir_list); break;

          case KEY_SLASH:

              search_forward(dir_list); break;

          case KEY_BKSLASH:

              search_backward(dir_list); break;

          case KEY_BANG:

              shell_command(dir_list); break;

          case KEY_V:

              message(version);
              move_cursor(dir_list->savedYPos(), dir_list->savedXPos());
              synch_display();
              break;

          case KEY_CTL_L:

              redisplay(); break;

          case KEY_q:

              if (dir_stack->nelems() > 1)
              {
                  delete dir_stack->pop();
                  dir_list = dir_stack->top();

                  // update our current directory
                  if (chdir(dir_list->name()) < 0)
                      error("File %s, line %d: couldn't chdir() to `%s'",
                            __FILE__, __LINE__, dir_list->name());
                  //
                  // We track the CWD and PWD variables if they\'re defined,
                  // so that applications, such as emacs, which use them
                  // will work properly.
                  //
                  if (getenv("CWD"))
                  {
                      static String str;
                      static String ostr;
                      str = String("CWD=") + dir_list->name();
                      if (putenv(str) < 0)
                          error("File %s, line %d: putenv(%s) failed.",
                                __FILE__, __LINE__, dir_list->name());
                      ostr = str;
                  }
                  if (getenv("PWD"))
                  {
                      static String str;
                      static String ostr;
                      str = String("PWD=") + dir_list->name();
                      if (putenv(str) < 0)
                          error("File %s, line %d: putenv(%s) failed.",
                                __FILE__, __LINE__, dir_list->name());
                      ostr = str;
                  }
                  
                  update_modeline(modeline_prefix, dir_list->name());
                  redisplay();
              }
              else
              {
                  term_display();
                  exit(0);
              }
              break;

          case KEY_ESC:

              // some Emacs ESC key bindings

              switch(get_key(dir_list))
              {
                case KEY_v:
                  if (dir_list->firstLine() == dir_list->head()) break;
                  scroll_down_full_window(dir_list);
                  break;
                case KEY_TOP:
                  if (dir_list->atBegOfList()) break;
                  goto_first(dir_list);
                  break;
                case KEY_BOT:
                  if (dir_list->atEndOfList()) break;
                  goto_last(dir_list);
                  break;
                default:
                  ding();
                  break;
              }
          break;

          case KEY_Q:

              term_display(); exit(0); break;

          default:

              ding(); break;
        }
    }
}
