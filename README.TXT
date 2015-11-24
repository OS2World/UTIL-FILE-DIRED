
Dired - A Directory Editor

WHAT IS IT?

Dired is a directory editor modelled after Dired Mode of GNU Emacs,
but targetted for non-emacs users, designed for the UNIX environment.
It implements a window into the current directory consisting of lines
of the form:

    -rw------- 1 mjlx     staff        2002 Apr 11 15:17 c++-motif
    -rw------- 1 mjlx     staff       51399 Aug 11 1990  c++browser.tar.Z
    -rw------- 1 mjlx     staff         970 Mar 14 18:44 callback
    drwx--x--x 2 mjlx     staff         384 Jun 21 16:32 cfront
    -rw------- 1 mjlx     staff        2152 Dec 17 1990  cfront-sun4.1.bug.Z
    drwx------ 3 mjlx     staff        2048 Aug  1 13:04 dired
    -rw------- 1 mjlx     staff        2033 Jan 25 1991  link-bug.C.Z

The filename in the line containing the cursor is known as the current
file.  There are numerous of commands available for operating on the
current file.  These include copy, delete, edit, view, chmod, chgrp,
compress, rename and uncompress.  Hence by simply positioning the
cursor on the appropriate filename, one can do many of the "usual"
UNIX operations on the files in a directory without bothering to
spelling out the command and filename each time.  If you edit a
directory, you get a directory listing of that directory.  In this
manner it is possible to walk up and down a directory tree from within
dired.


AUTHOR:

Mike Lijewski (W) 301-794-5132 (H) 301-982-5461
Goddard Space Flight Center
INTERNET: lijewski@rosserv.gsfc.nasa.gov
SMAIL:  446 Ridge Rd. Apt. 3, Greenbelt, MD  20770


COPYRIGHT:


Copyright (c) 1991, 1992 Cornell University
All rights reserved.

Redistribution and use in source and binary forms are permitted
provided that: (1) source distributions retain this entire copyright
notice and comment, and (2) distributions including binaries display
the following acknowledgement:  ``This product includes software
developed by Cornell University'' in the documentation or other
materials provided with the distribution and in all advertising
materials mentioning features or use of this software. Neither the
name of the University nor the names of its contributors may be used
to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.



---------
Ported to OS/2 by Kai Uwe Rommel, 11/19/1992

--
/* Kai Uwe Rommel                                      Muenchen, Germany *
 * rommel@jonas.ucrc.sub.org                       Phone +49 89 723 4101 *
 * rommel@informatik.tu-muenchen.de                  Fax +49 89 723 7889 */

DOS ... is still a real mode only non-reentrant interrupt
handler, and always will be.                -Russell Williams

