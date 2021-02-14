//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_list.h
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __PLUGIN_LIST_H__
#define __PLUGIN_LIST_H__

#include "plugin_scan.h"
#include <list>
#include <memory>

namespace MusEPlugin {
  
//-------------------------------------------------
// PluginScanListRef
// A reference counted pointer to PluginScanInfo
//-------------------------------------------------

typedef std::shared_ptr<PluginScanInfo> PluginScanInfoRef;

//-----------------------------------------
// PluginScanList
//-----------------------------------------

class PluginScanList : public std::list<PluginScanInfoRef>
{
  public:
    PluginScanList() {}

    bool add(PluginScanInfo* info);
    // Each argument optional, can be empty.
    // If uri is not empty, the search is based solely on it, the other arguments are ignored.
    PluginScanInfoRef find(const PluginInfoString_t& file,
                           const PluginInfoString_t& uri,
                           const PluginInfoString_t& label,
                           PluginScanInfoStruct::PluginType_t types = PluginScanInfoStruct::PluginTypeAll) const;
    // If uri is not empty, the search is based solely on it, the other info members are ignored.
    PluginScanInfoRef find(const PluginScanInfoStruct& info) const;
};
typedef PluginScanList::iterator iPluginScanList;
typedef PluginScanList::const_iterator ciPluginScanList;


} // namespace MusEPlugin

#endif


