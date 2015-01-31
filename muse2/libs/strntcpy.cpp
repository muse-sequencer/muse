//=========================================================
//  MusE
//  Linux Music Editor
//    strntcpy.cpp 
//  (C) Copyright 2014 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include "strntcpy.h"

namespace MusELib {

// Copies at most size bytes, always null-terminating the destination. It does not null-fill remaining destination bytes.
// NULL source is accepted. Returns the destination, or zero if size is zero.  
// Note that there is a similar strlcpy() in BSD (libbsd in linux), as well as QT's qstrncpy().
char* strntcpy(char *dest, const char *src, int size)
{
  if(size == 0)
    return 0;
  if(!src)
  {
    dest[0] = '\0';
    return dest;
  }
  int i = 0;
  while(i < size)
  {
    dest[i] = src[i];
    if(src[i++] == '\0')
      break;
  }
  dest[--i] = '\0';
  return dest;
}


} // namespace MusELib
