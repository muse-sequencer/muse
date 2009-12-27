//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.cpp,v 1.6.2.5 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <qtoolbutton.h>
#include <qlabel.h>
#include <qlayout.h>

#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "song.h"
#include "track.h"
#include "strip.h"
#include "meter.h"
#include "utils.h"

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Strip::setRecordFlag(bool flag)
      {
      if (record) {
            record->blockSignals(true);
            record->setOn(flag);
            record->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Strip::resetPeaks()
      {
      track->resetPeaks();
      }

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void Strip::recordToggled(bool val)
      {
      if (track->type() == Track::AUDIO_OUTPUT) {
            if (val && track->recordFlag() == false) {
                  muse->bounceToFile((AudioOutput*)track);
                  }
            audio->msgSetRecord((AudioOutput*)track, val);
            if (!((AudioOutput*)track)->recFile())
                  record->setOn(false);
            return;
            }
      song->setRecordFlag(track, val);
      }
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Strip::heartBeat()
      {
      }

//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------
// Added by Tim. p3.3.9

void Strip::setLabelFont()
{
  // Use the new font #6 I created just for these labels (so far).
  // Set the label's font.
  label->setFont(config.fonts[6]);
  // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
  autoAdjustFontSize(label, label->text(), false, true, config.fonts[6].pointSize(), 5);
}

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void Strip::setLabelText()
      {
      //label->setText(track->name());
      QString s;
      switch(track->type()) {
            case Track::AUDIO_OUTPUT:
                  label->setBackgroundColor(green);
                  s = track->name();
                  break;
            case Track::AUDIO_GROUP:
                  label->setBackgroundColor(yellow);
                  s = track->name();
                  break;
            case Track::AUDIO_AUX:
                  //label->setBackgroundColor(cyan);
                  label->setBackgroundColor(QColor(120, 255, 255)); // Light blue
                  s = track->name();
                  break;
            case Track::WAVE:
                  label->setBackgroundColor(magenta);
                  s = track->name();
                  break;
            case Track::AUDIO_INPUT:
                  label->setBackgroundColor(red);
                  s = track->name();
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  //label->setBackgroundColor(white);
                  label->setBackgroundColor(QColor(255, 130, 0)); // Med orange
                  s = track->name();
                  break;
            case Track::MIDI:
            case Track::DRUM:
                  {
                  MidiTrack* mt = (MidiTrack*)track;
                  int port = mt->outPort();
                  int channel = mt->outChannel();
                  //QString s;
                  s.sprintf("%d-%d", port + 1, channel + 1);
                  //label->setText(s);
                  //label->setBackgroundColor(gray);
                  label->setBackgroundColor(QColor(0, 160, 255)); // Med blue
                  }
                  break;
            }
            // Added by Tim. p3.3.9
            label->setText(s);
            
      }

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void Strip::muteToggled(bool val)
      {
      track->setMute(val);
      song->update(SC_MUTE);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void Strip::soloToggled(bool val)
      {
      audio->msgSetSolo(track, val);
      song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(QWidget* parent, Track* t)
   : QFrame(parent, "Strip", Qt::WDestructiveClose)
      {
      setBackgroundMode(PaletteMid);
      setFrameStyle(Panel | Raised);
      setLineWidth(2);

      useSoloIconSet2 = false;
      
      track    = t;
      meter[0] = 0;
      meter[1] = 0;
      setFixedWidth(STRIP_WIDTH);
      layout = new QVBoxLayout(this);
      layout->setMargin(3);

      //---------------------------------------------
      //    label
      //---------------------------------------------

      label = new QLabel(this);
      // Moved by Tim. p3.3.9
      //setLabelText();
      //label->setFont(config.fonts[1]);
      
      // Added by Tim. p3.3.9
      //printf("Strip::Strip w:%d frw:%d layoutmarg:%d lx:%d ly:%d lw:%d lh:%d\n", STRIP_WIDTH, frameWidth(), layout->margin(), label->x(), label->y(), label->width(), label->height());
      
      // Tested: The label's width is 100. It does not become STRIP_WIDTH - 2*layout->margin
      //  until the mixer is shown in MusE::showMixer.
      // Therefore 'fake' set the size of the label now.
      // Added by Tim. p3.3.9
      //label->setGeometry(label->x(), label->y(), STRIP_WIDTH - 2*frameWidth() - 2*layout->margin(), label->height());
      label->setGeometry(label->x(), label->y(), STRIP_WIDTH - 2*layout->margin(), label->height());
      
      // Unfortunately for the mixer labels, QLabel doesn't support the BreakAnywhere flag.
      // Changed by Tim. p3.3.9
      //label->setAlignment(AlignCenter);
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
      label->setAlignment(AlignCenter | WordBreak);
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));
      label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      
      // Added by Tim. p3.3.9
      setLabelText();
      setLabelFont();
      
      layout->addWidget(label);
      }

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

Strip::~Strip()
      {
      }

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void Strip::setAutomationType(int t,int)
      {
      track->setAutomationType(AutomationType(t));
      song->update(SC_AUTOMATION);
      }


