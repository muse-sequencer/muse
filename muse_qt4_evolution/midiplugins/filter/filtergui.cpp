//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filtergui.cpp,v 1.4 2005/11/06 17:49:34 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "filtergui.h"
#include "filter.h"
#include "ctrlcombo.h"

//---------------------------------------------------------
//   MidiFilterConfig
//---------------------------------------------------------

FilterGui::FilterGui(Filter* f, QWidget* parent)
  : QDialog(parent)
      {
      setupUi(this);
      filter = f;
      connect(rf1, SIGNAL(toggled(bool)), SLOT(rf1Toggled(bool)));
      connect(rf2, SIGNAL(toggled(bool)), SLOT(rf2Toggled(bool)));
      connect(rf3, SIGNAL(toggled(bool)), SLOT(rf3Toggled(bool)));
      connect(rf4, SIGNAL(toggled(bool)), SLOT(rf4Toggled(bool)));
      connect(rf5, SIGNAL(toggled(bool)), SLOT(rf5Toggled(bool)));
      connect(rf6, SIGNAL(toggled(bool)), SLOT(rf6Toggled(bool)));
      connect(rf7, SIGNAL(toggled(bool)), SLOT(rf7Toggled(bool)));
      connect(cb1, SIGNAL(activated(int)), SLOT(cb1Activated(int)));
      connect(cb2, SIGNAL(activated(int)), SLOT(cb2Activated(int)));
      connect(cb3, SIGNAL(activated(int)), SLOT(cb3Activated(int)));
      connect(cb4, SIGNAL(activated(int)), SLOT(cb4Activated(int)));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void FilterGui::init()
      {
      int midiType = filter->midiType();
      rf1->setChecked(midiType & MIDI_FILTER_NOTEON);
      rf2->setChecked(midiType & MIDI_FILTER_POLYP);
      rf3->setChecked(midiType & MIDI_FILTER_CTRL);
      rf4->setChecked(midiType & MIDI_FILTER_PROGRAM);
      rf5->setChecked(midiType & MIDI_FILTER_AT);
      rf6->setChecked(midiType & MIDI_FILTER_PITCH);
      rf7->setChecked(midiType & MIDI_FILTER_SYSEX);
      cb1->setCurrentIndex(filter->midiCtrl(0) + 1);
      cb2->setCurrentIndex(filter->midiCtrl(1) + 1);
      cb3->setCurrentIndex(filter->midiCtrl(2) + 1);
      cb4->setCurrentIndex(filter->midiCtrl(3) + 1);
      }

//---------------------------------------------------------
//   cb1Activated
//---------------------------------------------------------

void FilterGui::cb1Activated(int idx)
      {
      filter->setMidiCtrl(0, idx - 1);
      }

//---------------------------------------------------------
//   cb2Activated
//---------------------------------------------------------

void FilterGui::cb2Activated(int idx)
      {
      filter->setMidiCtrl(1, idx - 1);
      }

//---------------------------------------------------------
//   cb3Activated
//---------------------------------------------------------

void FilterGui::cb3Activated(int idx)
      {
      filter->setMidiCtrl(2, idx - 1);
      }

//---------------------------------------------------------
//   cb4Activated
//---------------------------------------------------------

void FilterGui::cb4Activated(int idx)
      {
      filter->setMidiCtrl(3, idx - 1);
      }

//---------------------------------------------------------
//   rf1Toggled
//---------------------------------------------------------

void FilterGui::rf1Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf1->isChecked())
            midiType |= MIDI_FILTER_NOTEON;
      else
            midiType &= ~MIDI_FILTER_NOTEON;
      filter->setMidiType(midiType);
      }

//---------------------------------------------------------
//   rf2Toggled
//---------------------------------------------------------

void FilterGui::rf2Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf2->isChecked())
            midiType |= MIDI_FILTER_POLYP;
      else
            midiType &= ~MIDI_FILTER_POLYP;
      filter->setMidiType(midiType);
      }

//---------------------------------------------------------
//   rf3Toggled
//---------------------------------------------------------

void FilterGui::rf3Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf3->isChecked())
            midiType |= MIDI_FILTER_CTRL;
      else
            midiType &= ~MIDI_FILTER_CTRL;
      filter->setMidiType(midiType);
      }

//---------------------------------------------------------
//   rf4Toggled
//---------------------------------------------------------

void FilterGui::rf4Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf4->isChecked())
            midiType |= MIDI_FILTER_PROGRAM;
      else
            midiType &= ~MIDI_FILTER_PROGRAM;
      filter->setMidiType(midiType);
      }

//---------------------------------------------------------
//   rf5Toggled
//---------------------------------------------------------

void FilterGui::rf5Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf5->isChecked())
            midiType |= MIDI_FILTER_AT;
      else
            midiType &= ~MIDI_FILTER_AT;
      filter->setMidiType(midiType);
      }

//---------------------------------------------------------
//   rf6Toggled
//---------------------------------------------------------

void FilterGui::rf6Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf1->isChecked())
            midiType |= MIDI_FILTER_PITCH;
      else
            midiType &= ~MIDI_FILTER_PITCH;
      filter->setMidiType(midiType);
      }

//---------------------------------------------------------
//   rf7Toggled
//---------------------------------------------------------

void FilterGui::rf7Toggled(bool)
      {
      int midiType = filter->midiType();
      if (rf7->isChecked())
            midiType |= MIDI_FILTER_SYSEX;
      else
            midiType &= ~MIDI_FILTER_SYSEX;
      filter->setMidiType(midiType);
      }
