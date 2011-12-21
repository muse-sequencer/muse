//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mitplugin.cpp,v 1.1.1.1 2003/10/27 18:52:40 wschweer Exp $
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

#include "mitplugin.h"
#include "app.h"
#include "event.h"
#include "xml.h"

#include "midiitransform.h"
#include "mittranspose.h"
#include "midifilterimpl.h"
#include "mrconfig.h"

#ifdef BUILD_EXPERIMENTAL
#include "rhythm.h"
#endif

namespace MusEGui {

//---------------------------------------------------------
//   startMidiInputPlugin
//---------------------------------------------------------

void MusE::startMidiInputPlugin(int id)
      {
      bool flag = false;
      QWidget* w = 0;
      QAction* act;
      if (id == 0) {
            if (!MusEGlobal::mitPluginTranspose) {
                  MusEGlobal::mitPluginTranspose = new MITPluginTranspose();
                  MusECore::mitPlugins.push_back(MusEGlobal::mitPluginTranspose);
                  connect(MusEGlobal::mitPluginTranspose, SIGNAL(hideWindow()),
                     SLOT(hideMitPluginTranspose()));
                  }
            w = MusEGlobal::mitPluginTranspose;
            act = midiTrpAction;
            }
      else if (id == 1) {
            if (!midiInputTransform) {
                  midiInputTransform = new MidiInputTransformDialog();
                  connect(midiInputTransform, SIGNAL(hideWindow()),
                     SLOT(hideMidiInputTransform()));
                  }
            w = midiInputTransform;
            act = midiInputTrfAction;
            }
      else if (id == 2) {
            if (!midiFilterConfig) {
                  midiFilterConfig = new MidiFilterConfig();
                  connect(midiFilterConfig, SIGNAL(hideWindow()),
                     SLOT(hideMidiFilterConfig()));
                  }
            w = midiFilterConfig;
            act = midiInputFilterAction;
            }
      else if (id == 3) {
            if (!midiRemoteConfig) {
                  midiRemoteConfig = new MRConfig();
                  connect(midiRemoteConfig, SIGNAL(hideWindow()),
                     SLOT(hideMidiRemoteConfig()));
                  }
            w = midiRemoteConfig;
            act = midiRemoteAction;
            }
#ifdef BUILD_EXPERIMENTAL
      else if (id == 4) {
            if (!midiRhythmGenerator) {
                  midiRhythmGenerator = new RhythmGen();
                  connect(midiRhythmGenerator, SIGNAL(hideWindow()),
                     SLOT(hideMidiRhythmGenerator()));
                  }
            w = midiRhythmGenerator;
            act = midiRhythmAction;
            }
#endif
      if (w) {
            flag = !w->isVisible();
            if (flag)
                  w->show();
            else
                  w->hide();
            }
      act->setChecked(flag);
      }

void MusE::hideMitPluginTranspose()
      {
      midiTrpAction->setChecked(false);
      }
void MusE::hideMidiInputTransform()
      {
      midiInputTrfAction->setChecked(false);
      }
void MusE::hideMidiFilterConfig()
      {
      midiInputFilterAction->setChecked(false);
      }
void MusE::hideMidiRemoteConfig()
      {
      midiRemoteAction->setChecked(false);
      }
#ifdef BUILD_EXPERIMENTAL
void MusE::hideMidiRhythmGenerator()
      {
      midiRhythmAction->setChecked(false);
      }
#endif

//---------------------------------------------------------
//   startMidiTransformer
//---------------------------------------------------------

void MusE::startMidiTransformer()
      {
      if (midiTransformerDialog == 0)
            midiTransformerDialog = new MidiTransformerDialog;
      midiTransformerDialog->show();
      }

} // namespace MusEGui

namespace MusECore {

MITPluginList mitPlugins;

//---------------------------------------------------------
//   processMidiInputTransformPlugins
//---------------------------------------------------------

void processMidiInputTransformPlugins(MEvent& event)
      {
      for (iMITPlugin i = mitPlugins.begin(); i != mitPlugins.end(); ++i)
            (*i)->process(event);
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

} // namespace MusECore
