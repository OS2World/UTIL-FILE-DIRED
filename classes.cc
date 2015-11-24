/*
** classes.C - contains definitions of the member functions which
**             aren\'t defined in the relevant class declarations.
**
** classes.C 1.25   Delta\'d: 09:49:36 9/28/92   Mike Lijewski, CNSF
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

#include <new.h>
#include <stdio.h>
#include <string.h>

#include "dired.h"

DirList::DirList(char *name)
{
    _head = _tail = 0;
    _next = _prev = 0;
    _nelems = 0;
    _name = name;
}

/*
** Inserts a new DirLine, constructed from the given char**, after
** the current line in the DirList.
*/

DirLine *DirList::insert(char **line)
{
    DirLine *new_line = new DirLine(line);
    DirLine *ln = currLine();

    if (atEndOfList())
    {
        ln->_next = new_line;
        new_line->_prev = ln;
        _tail = new_line;
    }
    else
    {
        new_line->_next = ln->next();
        new_line->_prev = ln;
        ln->_next->_prev = new_line;
        ln->_next = new_line;
    }
    _nelems++;
    return new_line;
}

/*
** Adds the DirLine to the listing maintained by DirList.
*/

void DirList::add(DirLine *link)
{
    if (nelems())
    {
        _tail->_next = link;
        _tail->_next->_prev = tail();
        _tail = link;
        _nelems++;
    }
    else
    {
        _head = _tail = link;
        _nelems = 1;
    }
}

/*
** Delete the current listing line in the window
** and update our view.  The width of our view
** always decreases by one.  If the calling procedure
** adds more lines to the screen, they\'ll have to reset
** lastLine\(\) and/or firstLine\(\), but currLine doesn\'t need to change.
*/

void DirList::deleteLine()
{
    DirLine *line = currLine();

    if (atBegOfList())
    {
        //
        // that is, firstLine\(\) == head\(\)
        //
        _head = _firstLine = _currLine = head()->next();
        _head->_prev = 0;
    }
    else if (atWindowTop())
    {
        //
        // but firstLine\(\) != head\(\)
        //
        _firstLine = _currLine = line->next();
        line->_next->_prev = line->prev();
        line->_prev->_next = line->next();
    }
    else if (atEndOfList())
    {
        //
        // lastLine\(\) == tail\(\)
        //
        _tail = _lastLine = _currLine = line->prev();
        _tail->_next = 0;
    }
    else
    {
        _currLine = line->next();
        line->_next->_prev = line->prev();
        line->_prev->_next = line->next();
    }

    _nelems--;
    delete line;
}

DirList::~DirList()
{
    if (nelems())
    {
        DirLine *tmp = tail(), *prev = tail()->prev();
        while(tmp)
        {
            delete tmp;
            if ((tmp = prev) != 0) prev = tmp->prev();
        }
        delete tmp;
        DELETE _name;
    }
}

void DirStack::push(DirList *list)
{
    if (nelems())
    {
        top()->setNext(list);
        list->setPrev(top());
        _top = list;
        _nelems++;
    }
    else
    {
        _top    = list;
        _nelems = 1;
    }
}

DirList *DirStack::pop()
{
    DirList *tmp = top();

    if (nelems() > 1)
    {
        _top = top()->prev();
        top()->setNext(0);
    }
    else
        _top = 0;

    _nelems--;
    return tmp;
}

/*
** The definition of the head of the freelist that DirLine::operator new\(\)
** uses to dole out dirLines efficiently.
*/

DirLine *DirLine::freeList;

typedef void (*PEHF)();

void *DirLine::operator new(size_t size)
{
    if (size != sizeof(DirLine)) return ::new char[size];

    DirLine *line = freeList;

    if (line)
        freeList = line->next();
    else
    {
        DirLine *block = (DirLine *) ::new char[chunksize * sizeof(DirLine)];
        if (block == 0)
        {
            PEHF newHandler = set_new_handler(0);
            set_new_handler(newHandler);
            if (newHandler)
                newHandler();
            else
                return 0;
        }
        for (int i = 0; i < chunksize - 1; i++)
            block[i]._next = (DirLine *)&block[i + 1];
        block[chunksize - 1]._next = 0;
        line = block;
        freeList = &block[1];
    }
    return line;
}

void DirLine::operator delete(void *object)
{
    DirLine *line = (DirLine *)object;
    line->_next = freeList;
    freeList = line;
}

StringRep::StringRep(const char *s)
{
    len = ::strlen(s);
    rep = ::new char[len + 1];
    ::strcpy(rep, s);
    count = 1;
}

String StringRep::operator+(const String& s) const
{
    size_t slen  = s.length() + length();
    char *buf    = ::new char[slen + 1];
    ::strcpy(buf, rep);
    ::strcat(buf, s.p->rep);
    return String(&buf, slen);
}

/*
** The definition of the head of the freelist that StringRep::operator new\(\)
** uses to dole out StringReps efficiently.
*/

StringRep *StringRep::freeList;

void* StringRep::operator new(size_t size)
{
    if (size != sizeof(StringRep)) return ::new char[size];

    StringRep *s = freeList;

    if (s)
        freeList = s->next;
    else
    {
        StringRep *block = (StringRep*)::new char[chunksize*sizeof(StringRep)];
        if (block == 0)
        {
            PEHF newHandler = set_new_handler(0);
            set_new_handler(newHandler);
            if (newHandler)
                newHandler();
            else
                return 0;
        }
        for (int i = 0; i < chunksize - 1; i++)
            block[i].next = (StringRep *)&block[i + 1];
        block[chunksize - 1].next = 0;
        s = block;
        freeList = &block[1];
    }
    return s;
}

void StringRep::operator delete(void *object)
{
    StringRep *s = (StringRep *)object;
    s->next = freeList;
    freeList = s;
}

String::~String() { if (--p->count <= 0) delete p; }

String& String::operator=(const String& rhs)
{
    rhs.p->count++;
    if (--p->count <= 0) delete p;
    p = rhs.p;
    return *this;
}

void String::operator+=(const String& rhs)
{
    size_t slen = p->length() + rhs.length();
    char *buf   = ::new char[slen + 1];
    (void)strcpy(buf, p->rep);
    (void)strcat(buf, rhs.p->rep);
    if (p->count == 1)
    {
        DELETE p->rep;
        p->rep = buf;
        p->len = slen;
    }
    else
        operator=(String(&buf, slen));
}

void String::operator+=(const char *rhs)
{
    size_t slen = p->length() + ::strlen(rhs);
    char *buf = ::new char[slen + 1];
    ::strcpy(buf, p->rep);
    ::strcat(buf, rhs);
    if (p->count == 1)
    {
        DELETE p->rep;
        p->rep = buf;
        p->len = slen;
    }
    else
        operator=(String(&buf, slen));
}

void String::range_error(int index)
{
    ::error("range error: %d out of bounds", index);
    exit(1);
}

SBHelper String::operator[](int index)
{
    if (index < 0 || index >= length()) range_error(index);
    return SBHelper(*this, index);
}

SBHelper::SBHelper(String& s, int i) : str(s), index(i) { };

char SBHelper::operator=(char c)
{
    if (str.p->count == 1)
        //
        // Only one reference to our String.  Just assign the character to
        // the appropriate place.  Note that String::operator\[\] does the
        // range checking.
        //
        str.p->rep[index] = c;
    else
    {
        // We have to uniquify our str.
        str = String(str.p->rep);
        str.p->rep[index] = c;
    }
    return c;
}
