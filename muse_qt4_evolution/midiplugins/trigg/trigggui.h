//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filtergui.h,v 1.4 2005/11/06 17:49:34 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __FILTERGUI_H__
#define __FILTERGUI_H__

#include "ui_trigggui.h"

class Trigg;

enum {
      MIDI_FILTER_NOTEON    = 1,
      MIDI_FILTER_POLYP     = 2,
      MIDI_FILTER_CTRL      = 4,
      MIDI_FILTER_PROGRAM   = 8,
      MIDI_FILTER_AT        = 16,
      MIDI_FILTER_PITCH     = 32,
      MIDI_FILTER_SYSEX     = 64
      };

//---------------------------------------------------------
//   TriggGui
//---------------------------------------------------------

class TriggGui : public QDialog, public Ui::TriggBase {
      Q_OBJECT

      Trigg* filter;

   signals:
      void hideWindow();

   private slots:
      void setNote(int );
      void setVelocity(int );

   public:
      TriggGui(Trigg*, QWidget* parent=0);
      void init();
      };

#endif

