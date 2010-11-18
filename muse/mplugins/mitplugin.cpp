//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mitplugin.cpp,v 1.1.1.1 2003/10/27 18:52:40 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "mitplugin.h"
#include "app.h"
#include "event.h"
#include "xml.h"

#include "midiitransform.h"
#include "mittranspose.h"
#include "midifilterimpl.h"
#include "mrconfig.h"
#include "rhythm.h"

MITPluginList mitPlugins;

//---------------------------------------------------------
//   startMidiInputPlugin
//---------------------------------------------------------

void MusE::startMidiInputPlugin(int id)
      {
      bool flag = false;
      QWidget* w = 0;
      if (id == 0) {
            if (!mitPluginTranspose) {
                  mitPluginTranspose = new MITPluginTranspose();
                  mitPlugins.push_back(mitPluginTranspose);
                  connect(mitPluginTranspose, SIGNAL(hideWindow()),
                     SLOT(hideMitPluginTranspose()));
                  }
            w = mitPluginTranspose;
            }
      else if (id == 1) {
            if (!midiInputTransform) {
                  midiInputTransform = new MidiInputTransformDialog();
                  connect(midiInputTransform, SIGNAL(hideWindow()),
                     SLOT(hideMidiInputTransform()));
                  }
            w = midiInputTransform;
            }
      else if (id == 2) {
            if (!midiFilterConfig) {
                  midiFilterConfig = new MidiFilterConfig();
                  connect(midiFilterConfig, SIGNAL(hideWindow()),
                     SLOT(hideMidiFilterConfig()));
                  }
            w = midiFilterConfig;
            }
      else if (id == 3) {
            if (!midiRemoteConfig) {
                  midiRemoteConfig = new MRConfig();
                  connect(midiRemoteConfig, SIGNAL(hideWindow()),
                     SLOT(hideMidiRemoteConfig()));
                  }
            w = midiRemoteConfig;
            }
      else if (id == 4) {
            if (!midiRhythmGenerator) {
                  midiRhythmGenerator = new RhythmGen();
                  connect(midiRhythmGenerator, SIGNAL(hideWindow()),
                     SLOT(hideMidiRhythmGenerator()));
                  }
            w = midiRhythmGenerator;
            }
      if (w) {
            flag = !w->isVisible();
            if (flag)
                  w->show();
            else
                  w->hide();
            }
      midiInputPlugins->setItemChecked(id, flag);
      }

void MusE::hideMitPluginTranspose()
      {
      midiInputPlugins->setItemChecked(0, false);
      }
void MusE::hideMidiInputTransform()
      {
      midiInputPlugins->setItemChecked(1, false);
      }
void MusE::hideMidiFilterConfig()
      {
      midiInputPlugins->setItemChecked(2, false);
      }
void MusE::hideMidiRemoteConfig()
      {
      midiInputPlugins->setItemChecked(3, false);
      }
void MusE::hideMidiRhythmGenerator()
      {
      midiInputPlugins->setItemChecked(4, false);
      }

//---------------------------------------------------------
//   processMidiInputTransformPlugins
//---------------------------------------------------------

void processMidiInputTransformPlugins(MEvent& event)
      {
      for (iMITPlugin i = mitPlugins.begin(); i != mitPlugins.end(); ++i)
            (*i)->process(event);
      }

//---------------------------------------------------------
//   startMidiTransformer
//---------------------------------------------------------

void MusE::startMidiTransformer()
      {
      if (midiTransformerDialog == 0)
            midiTransformerDialog = new MidiTransformerDialog;
      midiTransformerDialog->show();
      }

//---------------------------------------------------------
//   writeStatusMidiInputTransformPlugins
//---------------------------------------------------------

void writeStatusMidiInputTransformPlugins(int level, Xml& xml)
      {
      for (iMITPlugin i = mitPlugins.begin(); i != mitPlugins.end(); ++i) {
            xml.tag(level++, "mplugin name=\"%d\"");
            (*i)->writeStatus(level, xml);
            xml.etag(level, "mplugin");
            }
      }

//---------------------------------------------------------
//   readStatusMidiInputTransformPlugin
//---------------------------------------------------------

void readStatusMidiInputTransformPlugin(Xml&)
      {
      }

