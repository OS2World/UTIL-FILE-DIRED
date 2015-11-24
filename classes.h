/*
** classes.h - the declarations of the classes used in dired
**
** classes.h 1.22   Delta'd: 15:10:40 9/22/92   Mike Lijewski, CNSF
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

#ifndef __CLASSES_H
#define __CLASSES_H

//
// This should be included by including "dired.".  This is so we get
// a definition for DELETE.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////////
// A Simple reference counted string class.  It is implemented as an
// Envelope-Letter abstaction with String being the envelope and StringRep
// being the letter.
/////////////////////////////////////////////////////////////////////////////

class String;
class SBHelper;

class StringRep {
  public:
    friend class String;
    friend class SBHelper;

    StringRep() { rep = ::new char[1]; len = 0; *rep = 0; count = 1;    }
    StringRep(const char *s);
    StringRep(char** r, size_t slen) { rep = *r; len = slen; count = 1; }
    ~StringRep()                     { DELETE rep;                      }

    static StringRep *freeList;  // we manage our own storage
    enum { chunksize = 50 };     // # of StringReps to allocate at a time
    void *operator new(size_t size);
    void operator delete(void *object);

    int operator!=(const char *rhs) const;
    int operator==(const char *rhs) const;
    int operator!=(const StringRep& rhs) const;
    int operator==(const StringRep& rhs) const;

    String operator+(const String& s) const;

    size_t length() const { return len; }
  private:
    //
    // Disable these two methods
    //
    StringRep(const StringRep&);
    StringRep& operator=(const StringRep &);
    union {
        char *rep;
        StringRep *next;
    };
    size_t len;
    int count;
};

class String {
  public:
    friend class StringRep;
    friend class SBHelper;

    String()                       { p = new StringRep();                }
    String(const String& s)        { p = s.p; p->count++;                }
    String(const char *s)          { p = new StringRep(s);               }
    String(char **s)               { p = new StringRep(s, ::strlen(*s)); }
    String(char** s, size_t slen)  { p = new StringRep(s, slen);         }
    ~String();

    String& operator=(const String& rhs);

    int operator==(const char *rhs)   const;
    int operator==(const String& rhs) const;
    int operator!=(const char *rhs)   const;
    int operator!=(const String& rhs) const;

    String operator+(const String &rhs) const   { return *p + rhs;      }
    friend String operator+(const char *lhs, const String& rhs)
                                            { return rhs + String(lhs); }

    void operator+=(const String &rhs);
    void operator+=(const char *rhs);

    operator const char *() const { return p->rep; }
    SBHelper operator[](int index);
    size_t length() const { return p->len; }
    void range_error(int index);
  private:
    StringRep *p;
};

/////////////////////////////////////////////////////////////////////////////
// This class is a helper class used by String::operator[] to distinguish
// between applications of operator[] on the lhs and rhs of "=" signs.
/////////////////////////////////////////////////////////////////////////////

class SBHelper {
  public:
    SBHelper(String &s, int i);
    char operator=(char c);
    operator char() { return str.p->rep[index]; }
  private:
    SBHelper(const SBHelper&);        // disallow this method
    void operator=(const SBHelper&);  // disallow this method
    String &str;
    int index;
};

///////////////////////////////////////////////////////////////////////////////
// DirLine - class which contains one line of the long
//           listing of a directory.
///////////////////////////////////////////////////////////////////////////////

class DirLine {
    friend class DirList;
  private:
    String              _line;
    DirLine            *_next;
    DirLine            *_prev;

    //
    // Disallow these operations by not providing definitions.
    // Also keep compiler from generating default versions of these.
    //
    DirLine();
    DirLine(const DirLine &);
    DirLine &operator=(const DirLine &); 
  public:
    DirLine(char **line) : _line(line) { _next = _prev = 0; }
    ~DirLine() { }

    static DirLine *freeList;  // we manage our own storage for DirLines
    enum { chunksize = 50 };   // size blocks of DirLines we allocate
    void *operator new(size_t size);
    void operator delete(void *object);

    const char *line()  const { return _line;          }
    int length()        const { return _line.length(); }
    DirLine *next()     const { return _next;          }
    DirLine *prev()     const { return _prev;          }
    void update(char **newline) { _line = String(newline); }
};

///////////////////////////////////////////////////////////////////////////////
// DirList - class which manages a doubly-linked list of DirLines.
//           It also maintains our current notion of what is and isn't
//           visible in the window.
///////////////////////////////////////////////////////////////////////////////

class DirList {
  private:
    DirLine    *_head;
    DirLine    *_tail;
    char       *_name;        // full pathname of the directory
    int         _nelems;
    short       _saved_x;     // saved x cursor position
    short       _saved_y;     // saved y cursor position
    DirLine    *_firstLine;   // first viewable dirLine in curses window
    DirLine    *_lastLine;    // last  viewable dirLine in curses window
    DirLine    *_currLine;    // line cursor is on in curses window
    DirList    *_next;
    DirList    *_prev;

    //
    // Disallow these operations by not providing definitions.
    // Also keep compiler from generating default versions of these.
    //
    DirList();
    DirList(const DirList &);
    DirList &operator=(const DirList &);
  public:
    DirList(char *);
    ~DirList();

    DirLine *head()              const { return _head;                   }
    DirLine *tail()              const { return _tail;                   }
    DirLine *firstLine()         const { return _firstLine;              }
    DirLine *lastLine()          const { return _lastLine;               }
    DirLine *currLine()          const { return _currLine;               }
    DirList *next()              const { return _next;                   }
    DirList *prev()              const { return _prev;                   }

    int savedXPos()              const { return _saved_x;                }
    int savedYPos()              const { return _saved_y;                }

    void setFirst(DirLine *e)          { _firstLine = e;                 }
    void setLast (DirLine *e)          { _lastLine  = e;                 }
    void setCurrLine (DirLine *ln)     { _currLine = ln;                 }

    void setNext (DirList *l)          { _next = l;                      }
    void setPrev (DirList *l)          { _prev = l;                      }

    const char *name()           const { return _name;                   }
    int nelems()                 const { return _nelems;                 }
    void saveYXPos(int y, int x)       { _saved_x = (short)x;
                                         _saved_y = (short)y;            }

    int atBegOfList()            const { return _currLine == _head;      }
    int atEndOfList()            const { return _currLine == _tail;      }

    int atWindowTop()            const { return _currLine == _firstLine; }
    int atWindowBot()            const { return _currLine == _lastLine;  }

    DirLine *insert(char **);
    void add(DirLine *);
    void deleteLine();
};

///////////////////////////////////////////////////////////////////////////////
// DirStack - class which manages a stack of DirLists.
///////////////////////////////////////////////////////////////////////////////

class DirStack {
  private:
    DirList             *_top;
    int                  _nelems;
    //
    // Disallow these operations by not providing definitions.
    // Also keep compiler from generating default versions of these.
    //
    DirStack(const DirStack &);
    DirStack &operator=(const DirStack &);
  public:
    DirStack()                 { _top = 0; _nelems = 0; }
    DirList *top()       const { return _top;           }
    int nelems()         const { return _nelems;        }
    void push(DirList *);
    DirList *pop();
};

inline int StringRep::operator!=(const char *rhs) const {
    return strcmp(rep, rhs);
}

inline int StringRep::operator==(const char *rhs) const {
    return strcmp(rep, rhs) == 0;
}

inline int StringRep::operator!=(const StringRep& rhs) const {
    return strcmp(rep, rhs.rep);
}

inline int StringRep::operator==(const StringRep& rhs) const {
    return strcmp(rep, rhs.rep) == 0;
}

inline int String::operator==(const char *rhs) const {
    return *p == rhs;
}

inline int String::operator==(const String& rhs) const {
    return *p == *(rhs.p);
}

inline int String::operator!=(const char *rhs) const {
    return *p != rhs;
}

inline int String::operator!=(const String& rhs) const {
    return *p != *(rhs.p);
}

#endif /* __CLASSES_H */
