//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.cpp,v 1.6.2.5 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QToolButton>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QColor>
#include <QVBoxLayout>
#include <QFrame>

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
            record->setChecked(flag);
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
                  record->setChecked(false);
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
      QColor c;
      switch(track->type()) {
            case Track::AUDIO_OUTPUT:
                  c = Qt::green;
                  break;
            case Track::AUDIO_GROUP:
                  c = Qt::yellow;
                  break;
            case Track::AUDIO_AUX:
                  c = QColor(120, 255, 255);   // Light blue
                  break;
            case Track::WAVE:
                  c = Qt::magenta;
                  break;
            case Track::AUDIO_INPUT:
                  c = Qt::red;
                  break;
            case Track::AUDIO_SOFTSYNTH:
                  c = QColor(255, 130, 0);  // Med orange
                  break;
            case Track::MIDI:
            case Track::DRUM:
                  {
                  c = QColor(0, 160, 255); // Med blue
                  }
                  break;
            default:
                  return;      
            }
      
      label->setText(track->name());
      QPalette palette;
      palette.setColor(label->backgroundRole(), c);
      label->setPalette(palette);
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
   : QFrame(parent)
      {
      _curGridRow = 0;
      setAttribute(Qt::WA_DeleteOnClose);
      iR            = 0;
      oR            = 0;
      
      setBackgroundRole(QPalette::Mid);
      setFrameStyle(Panel | Raised);
      setLineWidth(2);
      
      // NOTE: Workaround for freakin' improper disabled button text colour (at least with Oxygen colours). 
      // Just set the parent palette.
      QPalette pal(palette());
      pal.setColor(QPalette::Disabled, QPalette::ButtonText, 
                   pal.color(QPalette::Disabled, QPalette::WindowText));
      setPalette(pal);
      
      useSoloIconSet2 = false;
      
      track    = t;
      meter[0] = 0;
      meter[1] = 0;
      //setFixedWidth(STRIP_WIDTH);
      //setMinimumWidth(STRIP_WIDTH);     // TESTING Tim.
      //setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding)); // TESTING Tim.
      setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding)); // TESTING Tim.
      
      grid = new QGridLayout();
      grid->setContentsMargins(0, 0, 0, 0);
      grid->setSpacing(0);
      setLayout(grid);

      //---------------------------------------------
      //    label
      //---------------------------------------------

      //label = new QLabel(this);
      // NOTE: This was required, otherwise the strip labels have no colour in the mixer only - track info OK !
      // Not sure why...
      label = new QLabel(this);
      
      
      // Moved by Tim. p3.3.9
      //setLabelText();
      //label->setFont(config.fonts[1]);
      
      //printf("Strip::Strip w:%d frw:%d layoutmarg:%d lx:%d ly:%d lw:%d lh:%d\n", STRIP_WIDTH, frameWidth(), layout->margin(), label->x(), label->y(), label->width(), label->height());
      
      // Tested: The label's width is 100. It does not become STRIP_WIDTH - 2*layout->margin
      //  until the mixer is shown in MusE::showMixer.
      // Therefore 'fake' set the size of the label now.
      // Added by Tim. p3.3.9
      //label->setGeometry(label->x(), label->y(), STRIP_WIDTH - 2*frameWidth() - 2*layout->margin(), label->height());
      label->setGeometry(label->x(), label->y(), STRIP_WIDTH - 2*grid->margin(), label->height());
      
      label->setTextFormat(Qt::PlainText);
      
      // Unfortunately for the mixer labels, QLabel doesn't support the BreakAnywhere flag.
      // Changed by Tim. p3.3.9
      //label->setAlignment(AlignCenter);
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
      // MusE-2 Tested: TextWrapAnywhere actually works, but in fact it takes precedence 
      //  over word wrap, so I found it is not really desirable. Maybe with a user setting...
      //label->setAlignment(Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere);
      label->setAlignment(Qt::AlignCenter | Qt::TextWordWrap);
      label->setWordWrap(true);
      label->setAutoFillBackground(true);
      label->setLineWidth(2);
      label->setFrameStyle(Sunken | StyledPanel);
      
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));
      label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      
      // Added by Tim. p3.3.9
      setLabelText();
      setLabelFont();
      
      //layout->addWidget(label);
      grid->addWidget(label, _curGridRow++, 0, 1, 2);
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

