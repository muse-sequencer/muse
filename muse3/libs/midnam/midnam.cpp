//=========================================================
//  MusE
//  Linux Music Editor
//
//  midnam.cpp
//  (C) Copyright 2019 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "midnam.h"

namespace MusECore {

void MidiNamNote::write(int level, MusECore::Xml& xml) const
{
  xml.put(level, "<Note Number=\"%d\" Name=\"%s\" />", _number, Xml::xmlString(_name).toLocal8Bit().constData());
}

void MidiNamNote::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidiNamNote");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Number") {
                          _number = xml.s2().toInt();
                          }
                    else if (tag == "Name") {
                          _name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Note")
                        return;
              default:
                    break;
              }
        }
}

//----------------------------------------------------------------


void MidiNamNotes::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = begin(); i != end(); ++i)
    (*i).write(level, xml);
}

void MidiNamNotes::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Note")
                        {
                          MidiNamNote n;
                          n.read(xml);
                          insert(n);
                        }
                        else
                          xml.unknown("MidiNamNotes");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "NoteNameList" || tag == "NoteGroup")
                            return;
                  default:
                        break;
                  }
            }
      
      }

//----------------------------------------------------------------


void MidiNamNoteGroup::write(int level, MusECore::Xml& xml) const
{
  xml.tag(level++, "NoteGroup Name=\"%s\"", Xml::xmlString(_name).toLocal8Bit().constData());
  MidiNamNotes::write(level, xml);
  xml.etag(--level, "NoteGroup");
}

void MidiNamNoteGroup::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Note")
                        {
                          MidiNamNote n;
                          n.read(xml);
                          insert(n);
                        }
                        else
                          xml.unknown("MidiNamNoteGroup");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                          _name = xml.s2();
                          }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "NoteGroup")
                            return;
                  default:
                        break;
                  }
            }
      
      }

//----------------------------------------------------------------


void MidiNamNoteGroups::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = begin(); i != end(); ++i)
    (*i).write(level, xml);
}

void MidiNamNoteGroups::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "NoteGroup")
                        {
                          MidiNamNoteGroup n;
                          n.read(xml);
                          insert(n);
                        }
                        else
                          xml.unknown("MidiNamNoteGroups");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "NoteNameList")
                            return;
                  default:
                        break;
                  }
            }
      
      }


//----------------------------------------------------------------


void MidiNamNoteNames::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = begin(); i != end(); ++i)
  {
    const QString& name = i->first;
    const MidiNamNoteGroups& ngs = i->second;
    xml.tag(level++, "NoteNameList Name=\"%s\"", Xml::xmlString(name).toLocal8Bit().constData());
    for(iMidiNamNoteGroups ing = ngs.begin(); ing != ngs.end(); ++ing)
      (*ing).write(level, xml);
    xml.etag(--level, "NoteNameList");
  }
}

void MidiNamNoteNames::read(MusECore::Xml& xml)
{
  MidiNamNoteGroups groups;

  // Create a default group into which ungrouped notes go.
  // FIXME: Non const ref NOT allowed! Chosen way not the most efficient.
//   MidiNamNoteGroupsPair mng_pr = groups.insert(MidiNamNoteGroup(QObject::tr("<default>"), true));
//   iMidiNamNoteGroups imng = mng_pr.first;
//   const MidiNamNoteGroup& def_group = *imng;
  MidiNamNoteGroup def_group(QObject::tr("<default>"), true);
  QString name;
  
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return;
              case MusECore::Xml::TagStart:
                    if (tag == "NoteGroup")
                    {
                      MidiNamNoteGroup n;
                      n.read(xml);
                      groups.insert(n);
                    }
                    else if (tag == "Note")
                    {
                      MidiNamNote n;
                      n.read(xml);
                      def_group.insert(n);
                    }
                    else
                      xml.unknown("MidiNamNoteGroups");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Name") {
                      name = xml.s2();
                      }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "NoteNameList")
                    {
                      groups.insert(def_group);
                      insert(MidiNamNoteNamesPair(name, groups));
                      return;
                    }
                    break;
              default:
                    break;
              }
        }
}


//----------------------------------------------------------------
//----------------------------------------------------------------


void MidiNamVal::write(int level, MusECore::Xml& xml) const
{
  xml.put(level, "<Value Number=\"%d\" Name=\"%s\" />", _number, Xml::xmlString(_name).toLocal8Bit().constData());
}

void MidiNamVal::read(MusECore::Xml& xml)
{
  for (;;) {
        MusECore::Xml::Token token(xml.parse());
        const QString& tag(xml.s1());
        switch (token) {
              case MusECore::Xml::Error:
              case MusECore::Xml::End:
                    return;
              case MusECore::Xml::TagStart:
                      xml.unknown("MidiNamVal");
                    break;
              case MusECore::Xml::Attribut:
                    if (tag == "Number") {
                          _number = xml.s2().toInt();
                          }
                    else if (tag == "Name") {
                          _name = xml.s2();
                          }
                    break;
              case MusECore::Xml::TagEnd:
                    if (tag == "Value")
                        return;
              default:
                    break;
              }
        }
}

//----------------------------------------------------------------


void MidiNamValNames::write(int level, MusECore::Xml& xml) const
{
  for(const_iterator i = begin(); i != end(); ++i)
    (*i).write(level, xml);
}

void MidiNamValNames::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Value")
                        {
                          MidiNamVal n;
                          n.read(xml);
                          insert(n);
                        }
                        else
                          xml.unknown("MidiNamValNames");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                              _name = xml.s2();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "ValueNameList")
                            return;
                  default:
                        break;
                  }
            }
      
      }

//----------------------------------------------------------------

      
// void MidiNamVals::write(int level, MusECore::Xml& xml) const
// {
//   for(const_iterator i = begin(); i != end(); ++i)
//     i->second.write(level, xml);
// }
// 
// void MidiNamVals::read(MusECore::Xml& xml)
//       {
//       for (;;) {
//             MusECore::Xml::Token token(xml.parse());
//             const QString& tag(xml.s1());
//             switch (token) {
//                   case MusECore::Xml::Error:
//                   case MusECore::Xml::End:
//                         return;
//                   case MusECore::Xml::TagStart:
//                         if (tag == "Note")
//                         {
//                           MidiNamNote n;
//                           n.read(xml);
//                           insert(n);
//                         }
//                         else
//                           xml.unknown("MidiNamNotes");
//                         break;
//                   case MusECore::Xml::Attribut:
//                         break;
//                   case MusECore::Xml::TagEnd:
//                         if (tag == "NoteNameList" || tag == "NoteGroup")
//                             return;
//                   default:
//                         break;
//                   }
//             }
//       
//       }


//----------------------------------------------------------------

      
void MidiNamValues::write(int /*level*/, MusECore::Xml& /*xml*/) const
{
//   for(const_iterator i = begin(); i != end(); ++i)
//     i->second.write(level, xml);
}

void MidiNamValues::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "ValueNameList")
                          _valueNames.read(xml);
                        if (tag == "UsesValueNameList")
                        {
                          _usesValueNameList = true;
                          _valueNameList = xml.parse1();
                        }
                        else
                          xml.unknown("MidiNamValues");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Min") {
                              _min = xml.s2().toInt();
                              }
                        else if (tag == "Max") {
                              _max = xml.s2().toInt();
                              }
                        else if (tag == "Default") {
                              _default = xml.s2().toInt();
                              }
                        else if (tag == "Units") {
                              _units = xml.s2().toInt();
                              }
                        else if (tag == "Mapping") {
                              _mapping = xml.s2().toInt();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "Values")
                            return;
                  default:
                        break;
                  }
            }
      
      }

      
//----------------------------------------------------------------

      
void MidiNamCtrl::write(int /*level*/, MusECore::Xml& /*xml*/) const
{
//   for(const_iterator i = begin(); i != end(); ++i)
//     i->second.write(level, xml);
}

void MidiNamCtrl::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Values")
                          _values.read(xml);
                        else
                          xml.unknown("MidiNamCtrl");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Type") {
                              _type = Type(xml.s2().toInt());
                              }
                        else if (tag == "Number") {
                              _number = xml.s2().toInt();
                              }
                        else if (tag == "Name") {
                              _name = xml.s2();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "Control")
                            return;
                  default:
                        break;
                  }
            }
      
      }


//----------------------------------------------------------------

      
void MidiNamCtrls::write(int /*level*/, MusECore::Xml& /*xml*/) const
{
//   for(const_iterator i = begin(); i != end(); ++i)
//     i->second.write(level, xml);
}

void MidiNamCtrls::read(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "Control")
                        {
                          MidiNamCtrl ctrl;
                          ctrl.read(xml);
                          insert(ctrl);
                        }
                        else
                          xml.unknown("MidiNamCtrls");
                        break;
                  case MusECore::Xml::Attribut:
                        if (tag == "Name") {
                              _name = xml.s2();
                              }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "ControlNameList")
                            return;
                  default:
                        break;
                  }
            }
      
      }

} // namespace MusECore


