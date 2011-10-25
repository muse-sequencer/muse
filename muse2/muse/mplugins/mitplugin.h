//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mitplugin.h,v 1.1.1.1.2.1 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __MITPLUGIN_H__
#define __MITPLUGIN_H__

#include <list>

namespace MusECore {

class MEvent;
class Xml;

//---------------------------------------------------------
//   MITPlugin
//    midi input transform plugin
//---------------------------------------------------------

class MITPlugin {
   public:
      virtual ~MITPlugin(){}
      virtual void process(MEvent& event) = 0;
      virtual void readStatus(Xml&) {}
      virtual void writeStatus(int, Xml&) const {}
      };

typedef std::list<MITPlugin*> MITPluginList;
typedef MITPluginList::iterator iMITPlugin;

extern MITPluginList mitPlugins;
extern void processMidiInputTransformPlugins(MEvent&);
extern void writeStatusMidiInputTransformPlugins(int, Xml&);
extern void readStatusMidiInputTransformPlugin(Xml&);

} // namespace MusECore

#endif

