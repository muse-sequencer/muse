//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: stringparam.cpp,v 1.0.0.0 2010/04/24 01:01:01 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  String parameter module added by Tim.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#include "stringparam.h" 
#include "xml.h" 

namespace MusECore {
 
//---------------------------------------------------------
//   findKey
//---------------------------------------------------------

iStringParamMap StringParamMap::findKey(const char* key)
{
  iStringParamMap icm = find(std::string(key));
  return icm;
}

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void StringParamMap::set(const char* key, const char* value)
{
  iStringParamMap icm = find(std::string(key));
  if(icm == end())
    insert(std::pair<std::string, std::string>(key, value));
  else
    icm->second = std::string(value);  
}

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void StringParamMap::remove(const char* key)
{
  erase(std::string(key));
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void StringParamMap::read(Xml& xml, const QString& name)
{
  QString n;
  QString value;
  
  for (;;) 
  {
    Xml::Token token = xml.parse();
    const QString tag = xml.s1();
    switch (token) 
    {
      case Xml::Error:
      case Xml::End:
            return;
      case Xml::TagStart:
            xml.unknown(name.toAscii().constData());
            break;
      case Xml::Attribut:
            if(tag == "name") 
              n = xml.s2();
            else
            if(tag == "val") 
              value = xml.s2();      
            else
              xml.unknown(name.toAscii().constData());
            break;
      case Xml::TagEnd:
            if(tag == name) 
            {
              // Add or modify the item.
              set(n.toLatin1(), value.toLatin1());
              return;
            }
      default:
            break;
    }
  }
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void StringParamMap::write(int level, Xml& xml, const char* name) const
{
  if(empty())
    return;
    
  for(ciStringParamMap r = begin(); r != end(); ++r) 
    xml.tag(level, "%s name=\"%s\" val=\"%s\"/", name, r->first.c_str(), r->second.c_str());
}

} // namespace MusECore
