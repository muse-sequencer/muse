//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_list.cpp
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

#include "plugin_list.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_LIST(dev, format, args...) // std::fprintf(dev, format, ##args);

namespace MusEPlugin {

//---------------------------------------------------------
//   find
//---------------------------------------------------------

PluginScanInfoRef PluginScanList::find(const PluginInfoString_t& file,
                                       const PluginInfoString_t& uri,
                                       const PluginInfoString_t& label,
                                       PluginScanInfoStruct::PluginType_t types) const
{
  const bool f_empty = file.isEmpty();
  const bool u_empty = uri.isEmpty();
  const bool l_empty = label.isEmpty();
  for(ciPluginScanList i = begin(); i != end(); ++i)
  {
    const PluginScanInfoRef& ref = *i;
    if((ref->info()._type & types) &&
       //(!u_empty || f_empty || file == ref->info()._completeBaseName) &&
       (!u_empty || f_empty || file == ref->info().filePath()) &&
       (u_empty  || uri   == ref->info()._uri) &&
       (!u_empty || l_empty || label == ref->info()._label))
      return ref;
  }
  //fprintf(stderr, "Plugin <%s> not found\n", name.toLatin1().constData());
  return PluginScanInfoRef();
}

PluginScanInfoRef PluginScanList::find(const PluginScanInfoStruct& info) const
{
  //const bool f_empty = info._completeBaseName.isEmpty();
  const bool f_empty = info.filePath().isEmpty();
  const bool u_empty = info._uri.isEmpty();
  const bool l_empty = info._label.isEmpty();
  for(ciPluginScanList i = begin(); i != end(); ++i)
  {
    const PluginScanInfoRef& ref = *i;
    if((info._type == ref->info()._type) && 
       //(!u_empty || f_empty || info._completeBaseName == ref->info()._completeBaseName) &&
       (!u_empty || f_empty || info.filePath() == ref->info().filePath()) &&
       (u_empty  || info._uri == ref->info()._uri) &&
       (!u_empty || l_empty || info._label == ref->info()._label))
          return ref;
  }
  //fprintf(stderr, "Plugin <%s> not found\n", name.toLatin1().constData());
  return PluginScanInfoRef();
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

bool PluginScanList::add(PluginScanInfo* info)
{
// Ignore duplicates.
// NOTE: For now we don't ignore anything down at this low-level. Ignoring is done more at higher levels.
//   PluginScanInfoRef psi = find(info->info()._completeBaseName, info->info()._label);
//   if(!psi)
//   {
//     fprintf(stderr, "PluginScanList::add: Ignoring %s: path:%s duplicate path of %s\n",
//             info->info()._label.c_str(),
//             info->info().fileName().c_str(),
//             psi->info().fileName().c_str());
//     return false;
//   }
  
  // Takes ownership of the object.
  push_back(PluginScanInfoRef(info));
  return true;
}

} // namespace MusEPlugin
