//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/aboutbox_impl.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
#include "aboutbox_impl.h"
#include "config.h"
#include "icons.h"
#include "globals.h"
#include "audio.h"
#include "midiseq.h"
#include "audiodev.h"
#ifdef HAVE_RTAUDIO
#include "driver/rtaudio.h"
#endif

namespace MusEGui {

AboutBoxImpl::AboutBoxImpl()
{
  setupUi(this);
  imageLabel->setPixmap(*aboutMuseImage);
  QString version(VERSION);
  QString gitstring(GITSTRING);
  versionLabel->setText("Version: " + version + (gitstring == QString() ? "" : "\n("+ gitstring + ")"));
  QString systemInfo="";

#ifdef LV2_SUPPORT
  systemInfo.append("\t\tLV2 support enabled.\n");
#endif
#ifdef DSSI_SUPPORT
  systemInfo.append("\t\tDSSI support enabled.\n");
#endif
#ifdef VST_NATIVE_SUPPORT
  #ifdef VST_VESTIGE_SUPPORT
    systemInfo.append("\t\tNative VST support enabled using VESTIGE compatibility header.\n");
  #else
    systemInfo.append("\t\tNative VST support enabled using Steinberg VSTSDK.\n");
  #endif
#endif

    internalDebugInformation->append(versionLabel->text());
    internalDebugInformation->append("Build info:");
    internalDebugInformation->append(systemInfo);
    internalDebugInformation->append("Runtime information:\n");
    internalDebugInformation->append(QString("Running audio driver:\t%1").arg(MusEGlobal::audioDevice->driverName()));

#ifdef HAVE_RTAUDIO
    if (MusEGlobal::audioDevice->deviceType() == MusECore::AudioDevice::RTAUDIO_AUDIO) {
      internalDebugInformation->append(QString("RT audio driver:\t%1").arg(((MusECore::RtAudioDevice*)MusEGlobal::audioDevice)->driverBackendName()));
    }
#endif
    internalDebugInformation->append(QString("Sample rate\t\t%1").arg(MusEGlobal::sampleRate));
    internalDebugInformation->append(QString("Segment size\t\t%1").arg(MusEGlobal::segmentSize));
    internalDebugInformation->append(QString("Segment count\t%1").arg(MusEGlobal::segmentCount));

    internalDebugInformation->append("\nTimer:");
    if ((MusEGlobal::midiSeq)) {
        internalDebugInformation->append(QString("Name\t\t%1").arg(MusEGlobal::midiSeq->getTimer()->getTimerName()));
        internalDebugInformation->append(QString("Freq\t\t%1").arg(MusEGlobal::midiSeq->getTimer()->getTimerFreq()));
    } else {
        internalDebugInformation->append("no timer information available as midiSeq is not instantiated");
    }

    internalDebugInformation->append("\nMiscellaneous:");
    internalDebugInformation->append(QString("debugMode:\t\t%1").arg(MusEGlobal::debugMode?"true":"false"));
    internalDebugInformation->append(QString("midInputTrace:\t%1").arg(MusEGlobal::midiInputTrace?"true":"false"));
    internalDebugInformation->append(QString("midiOutputTrace:\t%1").arg(MusEGlobal::midiOutputTrace?"true":"false"));
    internalDebugInformation->append(QString("unityWorkaround:\t%1").arg(MusEGlobal::unityWorkaround?"true":"false"));
    internalDebugInformation->append(QString("debugMsg:\t\t%1").arg(MusEGlobal::debugMsg?"true":"false"));
    internalDebugInformation->append(QString("heavyDebugMsg:\t%1").arg(MusEGlobal::heavyDebugMsg?"true":"false"));
    internalDebugInformation->append(QString("debugSync:\t\t%1").arg(MusEGlobal::debugSync?"true":"false"));
    internalDebugInformation->append(QString("loadPlugins:\t\t%1").arg(MusEGlobal::loadPlugins?"true":"false"));
    internalDebugInformation->append(QString("loadMESS:\t\t%1").arg(MusEGlobal::loadMESS?"true":"false"));
    internalDebugInformation->append(QString("loadVST:\t\t%1").arg(MusEGlobal::loadVST?"true":"false"));
    internalDebugInformation->append(QString("loadNativeVST:\t%1").arg(MusEGlobal::loadNativeVST?"true":"false"));
    internalDebugInformation->append(QString("loadDSSI:\t\t%1").arg(MusEGlobal::loadDSSI?"true":"false"));
    internalDebugInformation->append(QString("usePythonBridge:\t%1").arg(MusEGlobal::usePythonBridge?"true":"false"));

    internalDebugInformation->append(QString("useLASH:\t\t%1").arg(MusEGlobal::useLASH?"true":"false"));
    internalDebugInformation->append(QString("loadLV2:\t\t%1").arg(MusEGlobal::loadLV2?"true":"false"));
    internalDebugInformation->append(QString("useAlsaWithJack:\t%1").arg(MusEGlobal::useAlsaWithJack?"true":"false"));
    internalDebugInformation->append(QString("noAutoStartJack:\t%1").arg(MusEGlobal::noAutoStartJack?"true":"false"));
    internalDebugInformation->append(QString("populateMidiPortsOnStart:\t%1").arg(MusEGlobal::populateMidiPortsOnStart?"true":"false"));
    internalDebugInformation->append(QString("realtimeScheduling:\t%1").arg(MusEGlobal::realTimeScheduling?"true":"false"));
    internalDebugInformation->append(QString("midiRTPrioOverride:\t%1").arg(MusEGlobal::midiRTPrioOverride?"true":"false"));
    internalDebugInformation->append(QString("realtimePriority:\t%1").arg(MusEGlobal::realTimePriority));

    internalDebugInformation->append(QString("midiSeqRunning:\t%1").arg(MusEGlobal::midiSeqRunning?"true":"false"));
    internalDebugInformation->append(QString("automation:\t\t%1").arg(MusEGlobal::automation?"true":"false"));

}

}
