//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: stringparam.h,v 1.0.0.0 2010/04/24 01:01:01 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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

 
#ifndef __STRINGPARAM_H__
#define __STRINGPARAM_H__

#include <string>
#include <map>

class QString;

namespace MusECore {

class Xml;

typedef std::map<std::string, std::string >::iterator           iStringParamMap;
typedef std::map<std::string, std::string >::const_iterator     ciStringParamMap;

class StringParamMap : public std::map<std::string, std::string > 
{
  public:
    void set(const char* key, const char* value);
    void remove(const char* key);
    
    iStringParamMap findKey(const char* key);
    
    void read(Xml& xml, const QString& name);
    void write(int level, Xml& xml, const char* name) const;
};

} // namespace MusECore

#endif //__STRINGPARAM_H__
