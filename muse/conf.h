//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: conf.h,v 1.4.2.1 2006/09/28 19:22:25 spamatica Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONF_H__
#define __CONF_H__

#include <q3groupbox.h>
#include "configmidifilebase.h"

class QLineEdit;

//---------------------------------------------------------
//   MidiFileConfig
//    config properties of exported midi files
//---------------------------------------------------------

class MidiFileConfig : public ConfigMidiFileBase {
      Q_OBJECT

   private slots:
      void okClicked();
      void cancelClicked();

   public:
      MidiFileConfig();
      void updateValues();
      };

class Xml;
extern bool readConfiguration();
extern void readConfiguration(Xml&, bool readOnlySequencer);
#endif

