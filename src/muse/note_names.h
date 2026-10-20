//=========================================================
//  MusE
//  Linux Music Editor
//
//  note_names.h
//  (C) Copyright 2025 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __NOTE_NAMES_H__
#define __NOTE_NAMES_H__

#include <QString>
#include <QMap>
#include <QList>

#include "xml.h"

namespace MusECore {

class NoteName
{
  // Can be -1 meaning invalid.
  int _noteNum;
  QString _firstName;
  QString _secondName;

  public:
    NoteName();
    NoteName(int noteNum, const QString &firstName, const QString &secondName = QString());

    // Can return -1 meaning invalid.
    int noteNum() const;
    QString firstName() const;
    // If useFirstIfBlank is true and the second name is blank, returns the first name instead.
    QString secondName(bool useFirstIfBlank = false) const;
    void setFirstName(const QString &name);
    void setSecondName(const QString &name);

    // Returns true on error.
    bool read(Xml&);
    void write(int, Xml&) const;
};

class NoteNameList : public QMap<int, NoteName>
{
    QString _displayName;
    //QString _direction;   // "ltr", "rtl", "auto"
    int _startMidiNote;
    int _startMidiOctave;

  public:
    // Brings in all base constructors because constructors aren't inherited in C++.
    using QMap<int, NoteName>::QMap;

    enum ReturnResult { ResultSuccess = 0, ResultCancelled, ResultError };

    NoteNameList();
    NoteNameList(std::initializer_list<std::pair<int, NoteName>> list,
                 QString displayName = QString(),
                 //QString direction = QString("auto"),
                 int startMidiNote = 0,
                 int startMidiOctave = -1);

    static NoteNameList initialDefaults;

    //bool isRTLNoteList() const;
    void setDisplayName(const QString& s);
    //void setDirection(const QString& s);
    void setStartingMidiNote(int);
    void setStartingMidiOctave(int);

    QString displayName() const;
    //QString direction()    const;
    int startingMidiNote() const;
    int startingMidiOctave() const;
    int scaleLength()      const;

    // Returns true on error.
    bool read(Xml&);
    void write(int, Xml&) const;

    ReturnResult load();
    ReturnResult save() const;

    // This is a fast lookup. Note num is -1 if not found.
    NoteName findNoteName(int) const;
    // This uses a slower linear search. Either sharp or flat name can be given, but can't be blank.
    // Returns -1 if not found.
    int findNoteNumber(QString &) const;
};


} // namespace MusECore

#endif


