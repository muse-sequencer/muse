//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mitplugin.h,v 1.1.1.1.2.1 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MITPLUGIN_H__
#define __MITPLUGIN_H__

#include <list>

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

#endif

