//=========================================================
//  MusE
//  Linux Music Editor
//    strntcpy.h 
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

#ifndef __STRNTCPY_H__
#define __STRNTCPY_H__

namespace MusELib {

// Copies at most size bytes, always null-terminating the destination. It does not null-fill remaining destination bytes.
// Note that there is a similar strlcpy() in BSD (libbsd in linux), as well as QT's qstrncpy().
extern char* strntcpy(char *dest, const char *src, int size);

} // namespace MusELib

#endif