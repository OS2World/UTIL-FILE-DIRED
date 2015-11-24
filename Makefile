#
# Makefile for `dired'
#
# Makefile 1.37   Delta'd: 17:32:49 10/1/92   Mike Lijewski, CNSF
#

#
# Your C++ compiler goes here.
#
CC = gcc -O -Zomf -Zmt
O = .obj

#
# flags you need to compile:
#
#   Add -DNO_STRSTR if you don't have the ANSI C function strstr().
#
#   Add -O if you trust your optimizer.
#
#   Add -DNO_STRCHR if you don't have strchr() and strrchr().
#
#   Add -DNO_SYMLINKS if your OS doesn't support symbolic links.  If
#   you don't define this, it is assumed that your OS supports symbolic
#   links.
#
#   Add -DL_AND_G if your version of `ls' needs the `-lg' flags in
#   order to display both the owner and group of files in a long
#   listing.  Typically, BSD-based OSs need this and SYSV ones don't.
#
#   Add -DCOMPLETION if you want to enable tab-completion on the `E'
#   and `c' commands.  As currently written this assumes that you have
#   the POSIX directory routines in <dirent.h>.  In particular, I use
#   opendir(), readdir() and closedir().
#
#   Add -DOLDDELETE if your compiler can't handle the 'delete []' form
#   of the delete operator for deleting arrays of objects allocated
#   via new.  If you don't know whether you compiler can handle it or
#   not, just don't define it and see what happens.  If your compiler
#   accepts it, it'll do the right thing.
#
#   Add -DSIGINTERRUPT if you need to call siginterrupt(2) in order to
#   guarantee that signals will interrupt slow system calls.  If you
#   don't have siginterrupt(2), you most certainly don't need.  Even
#   if you have it you may not need it, though defining -DSIGINTERRUPT
#   in this case should be a no-op.  The best bet is to define this if
#   you have siginterrupt(2).  Note: Suns need this.
#
CFLAGS = -DCOMPLETION -DNO_SYMLINKS -DNO_LINKS

#
# Those flags needed to compile in the type of terminal
# control you have.  Use -DTERMIOS if you have <termios.h>, the POSIX
# terminal control.  Use -DTERMIO if you have <termio.h>, the SYSV
# terminal control.  Otherwise, the default assumes you have <sgtty.h>,
# the BSD terminal control.
#
# If you choose to use -DTERMIOS and have problems, try -DTERMIO.  On
# at least two systems I've tried, the vendor hasn't had all the
# include files set up correctly to include <unistd.h> together with 
#  <osfcn.h>, among others.
#
TERMFLAGS = -DTERMIO

#
# libraries needed:
#
#                   -ltermcap on BSD-like systems
#                   -ltermlib on SYSV-like systems
#                   -lcurses on systems w/o the above libraries
#
LIBS = -ltermcap -los2

##############################################################################
# nothing should need to be changed below here.
##############################################################################

#SHELL     = /bin/sh
#MAKESHELL = /bin/sh

HDR  = classes.h dired.h display.h keys.h version.h

MISC = dired.1 dired.lpr Makefile ChangeLog INSTALL MANIFEST README

OBJ  = classes$O commands$O dired$O display$O utilities$O

SRC  = classes.cc command1.cc command2.cc dired.cc display.cc globals.cc utilities.cc

#
# Sorry for including globals.cc in the compile line instead of the
# list of object files.  The Cfront 2.0 on my main development
# machine chokes when passed a list consisting solely of $O files.
#
dired.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) globals.cc $(LIBS)

display$O: display.cc display.h
	$(CC) $(CFLAGS) $(TERMFLAGS) -c display.cc

#
# Commands.cc is built from command1.cc and command2.cc the first time
# it is needed.  This is so that the shar files don't get too big.
#
#commands.cc: command1.cc command2.cc
#	cat command1.cc command2.cc > commands.cc

clean:
	-rm -f core *$O dired

realclean:
	-rm -f core *$O *~ *..c dired

#
# This depends on Rich Salz' cshar program.
#
shar: $(SRC) $(HDR) $(MISC)
	cshar -n1 -e5 -o dired-01 $(MISC)
	cshar -n2 -e5 -o dired-02 command1.cc classes.h dired.h
	cshar -n3 -e5 -o dired-03 command2.cc display.h keys.h
	cshar -n4 -e5 -o dired-04 classes.cc dired.cc display.cc globals.cc
	cshar -n5 -e5 -o dired-05 version.h utilities.cc
        
tar:
	tar cf dired.tar $(SRC) $(HDR) $(MISC)
	compress -f dired.tar

#
# dependencies
#
classes$O   : classes.h dired.h
commands$O  : classes.h dired.h display.h keys.h commands.cc
dired$O     : classes.h dired.h display.h
globals$O   : classes.h dired.h version.h
utilities$O : classes.h dired.h display.h keys.h


.SUFFIXES: .cc $O

.cc$O:
	$(CC) $(CFLAGS) -c $*.cc
