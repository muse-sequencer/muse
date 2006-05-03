//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filtergui.h,v 1.4 2005/11/06 17:49:34 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __FILTERGUI_H__
#define __FILTERGUI_H__

#include "ui_filtergui.h"

class Filter;

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
//   FilterGui
//---------------------------------------------------------

class FilterGui : public QDialog, public Ui::FilterBase {
      Q_OBJECT

      Filter* filter;

   signals:
      void hideWindow();

   private slots:
      void rf1Toggled(bool);
      void rf2Toggled(bool);
      void rf3Toggled(bool);
      void rf4Toggled(bool);
      void rf5Toggled(bool);
      void rf6Toggled(bool);
      void rf7Toggled(bool);
      void cb1Activated(int);
      void cb2Activated(int);
      void cb3Activated(int);
      void cb4Activated(int);

   public:
      FilterGui(Filter*, QWidget* parent=0);
      void init();
      };

#endif

