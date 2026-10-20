//=========================================================
//  MusE
//  Linux Music Editor
//
//  note_names.cpp
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


#include "note_names.h"
#include "globals.h"
#include "libs/file/file.h"
#include "filedialog.h"

#include <QLocale>
#include <QCollator>

namespace MusECore {

NoteNameList NoteNameList::initialDefaults =
{
  // Index : note number : first name : second name
  {
    {0,  {0,  "C",  ""}},
    {1,  {1,  "C#", "Db"}},
    {2,  {2,  "D",  ""}},
    {3,  {3,  "D#", "Eb"}},
    {4,  {4,  "E",  ""}},
    {5,  {5,  "F",  ""}},
    {6,  {6,  "F#", "Gb"}},
    {7,  {7,  "G",  ""}},
    {8,  {8,  "G#", "Ab"}},
    {9,  {9,  "A",  ""}},
    {10, {10, "A#", "Bb"}},
    {11, {11, "B",  ""}},
  },
  // displayName
  "Western 12-TET",
  // direction
  //"auto",
  // startMidiNote
  0,
  // startMidiOctave
  -1
};

//---------------------------------------
//  NoteName
//---------------------------------------

NoteName::NoteName() : _noteNum(-1)
{

}

NoteName::NoteName(int noteNum, const QString &firstName, const QString &secondName) :
  _noteNum(noteNum), _firstName(firstName), _secondName(secondName)
{

}

int NoteName::noteNum() const { return _noteNum; }
QString NoteName::firstName() const { return _firstName; }
QString NoteName::secondName(bool useFirstIfBlank) const
{
  if(useFirstIfBlank && _secondName.isEmpty())
    return _firstName;
  return _secondName;
}
void NoteName::setFirstName(const QString &name) { _firstName = name; }
void NoteName::setSecondName(const QString &name)  { _secondName = name; }

bool NoteName::read(Xml& xml)
{
  for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
              case Xml::Error:
              case Xml::End:
                    return true;
              case Xml::TagStart:
                          xml.unknown("NoteName");
                    break;
              case Xml::Attribut:
                        if (tag == "noteNum")
                          _noteNum = xml.s2().toInt();
                        else if (tag == "firstName")
                          _firstName = xml.s2();
                        else if (tag == "secondName")
                          _secondName = xml.s2();
                        else
                          fprintf(stderr, "NoteName::read unknown tag %s\n", tag.toLocal8Bit().constData());
                    break;
              case Xml::TagEnd:
                    if (tag == "noteName") {
                          // Empty first name not allowed.
                          return _firstName.isEmpty();
                          }
                    break;
              default:
                    break;
              }
        }
  return true;
}

void NoteName::write(int level, Xml& xml) const
{
  QString s = QString("noteName noteNum=\"%1\" firstName=\"%2\"").arg(_noteNum).arg(Xml::xmlString(_firstName));
  if(!_secondName.isEmpty())
    s += QString(" secondName=\"%1\"").arg(Xml::xmlString(_secondName));
  xml.emptyTag(level, s);
}


//---------------------------------------
//  NoteNameList
//---------------------------------------

NoteNameList::NoteNameList() :
  QMap<int, NoteName>::QMap(), _startMidiNote(0), _startMidiOctave(-1)
{
}

NoteNameList::NoteNameList(std::initializer_list<std::pair<int, NoteName>> list,
                           QString displayName,
                           //QString direction,
                           int startMidiNote,
                           int startMidiOctave) :
 QMap<int, NoteName>::QMap(list),
 _displayName(displayName),
 //_direction(direction),
 _startMidiNote(startMidiNote),
 _startMidiOctave(startMidiOctave)
{

}

void NoteNameList::setDisplayName(const QString& s) { _displayName = s; }
//void NoteNameList::setDirection(const QString& s)   { _direction = s; }
void NoteNameList::setStartingMidiNote(int v) { _startMidiNote = v; }
void NoteNameList::setStartingMidiOctave(int v) { _startMidiOctave = v; }

QString NoteNameList::displayName() const { return _displayName; }
//QString NoteNameList::direction()    const { return _direction; }
int NoteNameList::startingMidiNote() const { return _startMidiNote; }
int NoteNameList::startingMidiOctave() const { return _startMidiOctave; }
int NoteNameList::scaleLength()      const { return size(); }

bool NoteNameList::read(Xml& xml)
{
    clear();
    _displayName.clear();
    //_direction = "auto";

    // Get default or system locale.
    QLocale loc;
    // There is a default constructor but it's rather new at 5.13 so we'll do this.
    QCollator col(loc);
    col.setCaseSensitivity(Qt::CaseInsensitive);
    bool err = false;

    for (;;) {
        Xml::Token token = xml.parse();
        const QString& tag = xml.s1();

        switch (token) {

        case Xml::Error:
        case Xml::End:
            return true;

        case Xml::Attribut:
            if (tag == "name")
                _displayName = xml.s2();
            //else if (tag == "direction")
            //    _direction = xml.s2();     // "rtl", "ltr", "auto"
            else if (tag == "startMidiNote")
                _startMidiNote = xml.s2().toInt();
            else if (tag == "startMidiOctave")
                _startMidiOctave = xml.s2().toInt();
            else
              fprintf(stderr, "NoteNameList::read unknown tag %s\n", tag.toLocal8Bit().constData());
            break;

        case Xml::TagStart:
            if (tag == "noteName") {
                NoteName n;
                if (n.read(xml))
                {
                  err = true;
                }
                else
                {
                    // Check for existing note number. Check for existing note names.
                    // Duplicate first or second names are not allowed at different indexes.
                    // Second names can be empty.
                    const QString &fnn = n.firstName();
                    const QString &snn = n.secondName();
                    const int notenum = n.noteNum();
                    for(const_iterator i = cbegin(); i != cend(); ++i)
                    {
                      const NoteName &nn2 = *i;
                      const QString &fnn2 = nn2.firstName();
                      const QString &snn2 = nn2.secondName();
                      if(notenum == nn2.noteNum() ||
                         col.compare(fnn, fnn2) == 0 ||
                         (!snn2.isEmpty() && col.compare(fnn, snn2) == 0) ||
                         (!snn.isEmpty() && col.compare(snn, fnn2) == 0) ||
                         (!snn.isEmpty() && !snn2.isEmpty() && col.compare(snn, snn2) == 0))
                      {
                        err = true;
                      }
                    }
                    if(!err)
                      insert(notenum, n);
                }
            }
            else {
                xml.unknown("NoteNameList");
            }
            break;

        case Xml::TagEnd:
            if (tag == "noteNameList") {
                // Check number of items. More than 128 not allowed.
                // Check for gaps. Not allowed.
                const int sz = size();
                if(sz >= 128)
                  return true;
                for(int i = 0; i < sz; ++i)
                {
                  if(find(i) == cend())
                    return true;
                }
                return err;
            }
            break;

        default:
            break;
        }
    }

    return true;
}

void NoteNameList::write(int level, Xml& xml) const
{
    if(empty())
      return;
    QString s = QString("noteNameList name=\"%1\" startMidiNote=\"%2\" startMidiOctave=\"%3\"")
      .arg(Xml::xmlString(_displayName)).arg(_startMidiNote).arg(_startMidiOctave);
    xml.tag(level++, s);
    for(const_iterator inn = cbegin(); inn != cend(); ++inn)
      inn->write(level, xml);
    xml.etag(--level, "noteNameList");
}

NoteNameList::ReturnResult NoteNameList::load()
      {
      QString s("note_names/");

      QString fn = MusEGui::getOpenFileName(s, MusEGlobal::note_names_file_pattern,
         nullptr, QObject::tr("MusE: Load note name list"), nullptr);
      if (fn.isEmpty())
            return ResultCancelled;
      MusEFile::File f(fn, QString(".mnn"), nullptr);
      MusEFile::File::ErrorCode res = MusEGui::fileOpen(f, QIODevice::ReadOnly, nullptr, true);
      if (res != MusEFile::File::NoError)
            return ResultError;

      MusECore::Xml xml(f.iodevice());
      int mode = 0;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        f.close();
                        return ResultError;
                  case MusECore::Xml::TagStart:
                        if (mode == 0 && tag == "muse")
                              mode = 1;
                        else if (mode == 1 && tag == "noteNameList") {

                              if(read(xml))
                              {
                                fprintf(stderr, "Error loading NoteNameList\n");
                                f.close();
                                return ResultError;
                              }

                              mode = 0;
                              }
                        else
                              xml.unknown("NoteNameList");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if ((!mode && tag == "muse") || (mode && tag == "noteNameList"))
                        {
                              f.close();
                              return ResultSuccess;
                        }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

NoteNameList::ReturnResult NoteNameList::save() const
      {
      QString s("note_names/");

      QString fn = MusEGui::getSaveFileName(s, MusEGlobal::note_names_file_pattern, nullptr,
        QObject::tr("MusE: Save note name list"));
      if (fn.isEmpty())
            return ResultCancelled;
      MusEFile::File f(fn, QString(".mnn"), nullptr);
      MusEFile::File::ErrorCode res = MusEGui::fileOpen(f, QIODevice::WriteOnly, nullptr, false, true);
      if (res != MusEFile::File::NoError)
            return ResultError;

      MusECore::Xml xml(f.iodevice());
      xml.header();
      // Current note name list version. Don't use the running current song file version.
      xml.tag(0, "muse version=\"4.2\"");
      //xml.putFileVersion(0);
      write(1, xml);
      xml.etag(0, "muse");

      f.close();
      return ResultSuccess;
      }

NoteName NoteNameList::findNoteName(int noteNum) const
{
  noteNum = noteNum % size();
  const_iterator inn = find(noteNum);
  if(inn == constEnd())
    // Note num will be -1 meaning invalid.
    return NoteName();
  return inn.value();
}

int NoteNameList::findNoteNumber(QString &name) const
{
  if(name.isEmpty())
    return -1;
  for(const_iterator inn = constBegin(); inn != constEnd(); ++inn)
  {
    const NoteName &nn = inn.value();
    // Second names can be empty.
    if(nn.firstName() == name || nn.secondName() == name)
      return nn.noteNum();
  }
  return -1;
}

// bool NoteNameList::isRTLNoteList() const
// {
//     if (direction() == "rtl")
//         return true;
//     if (direction() == "ltr")
//         return false;
//
//     // auto-detect from first strong character
//     for (const NoteName& nn : *this) {
//         QString n = nn.firstName() + nn.secondName();
//         for (const QChar& c : n) {
//             if (c.direction() == QChar::DirR ||
//                 c.direction() == QChar::DirAL)
//                 return true;
//         }
//     }
//     return false;
// }

} // namespace MusECore
