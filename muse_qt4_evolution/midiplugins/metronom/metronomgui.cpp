//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronomgui.cpp,v 1.2 2005/10/05 17:02:03 lunar_shuttle Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "metronomgui.h"
#include "metronom.h"

//---------------------------------------------------------
//   MidiMetronomConfig
//---------------------------------------------------------

MetronomGui::MetronomGui(Metronom* f, QWidget* parent)
  : QDialog(parent)
      {
      setupUi(this);
      metronom = f;
      init();
      connect(beatNote, SIGNAL(valueChanged(int)), SLOT(beatNoteChanged(int)));
      connect(measureVelocity, SIGNAL(valueChanged(int)), SLOT(measureVelocityChanged(int)));
      connect(measureNote, SIGNAL(valueChanged(int)), SLOT(measureNoteChanged(int)));
      connect(beatVelocity, SIGNAL(valueChanged(int)), SLOT(beatVelocityChanged(int)));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MetronomGui::init()
      {
      beatNote->setValue(metronom->data.beatNote);
      measureNote->setValue(metronom->data.measureNote);
      beatVelocity->setValue(metronom->data.beatVelo);
      measureVelocity->setValue(metronom->data.measureVelo);
      }

//---------------------------------------------------------
//   beatNoteChanged
//---------------------------------------------------------

void MetronomGui::beatNoteChanged(int val)
      {
      metronom->data.beatNote = val;
      }

//---------------------------------------------------------
//   measureVelocityChanged
//---------------------------------------------------------

void MetronomGui::measureVelocityChanged(int val)
      {
      metronom->data.measureVelo = val;
      }

//---------------------------------------------------------
//   measureNoteChanged
//---------------------------------------------------------

void MetronomGui::measureNoteChanged(int val)
      {
      metronom->data.measureNote = val;
      }

//---------------------------------------------------------
//   beatVelocityChanged
//---------------------------------------------------------

void MetronomGui::beatVelocityChanged(int val)
      {
      metronom->data.beatVelo = val;
      }

