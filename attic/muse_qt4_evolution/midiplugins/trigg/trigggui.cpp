//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: filtergui.cpp,v 1.4 2005/11/06 17:49:34 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#include "trigggui.h"
#include "trigg.h"

//---------------------------------------------------------
//   MidiTriggConfig
//---------------------------------------------------------

TriggGui::TriggGui(Trigg* f, QWidget* parent)
  : QDialog(parent)
      {
      setupUi(this);
      filter = f;

      connect(noteEntry, SIGNAL(valueChanged(int)), SLOT(setNote(int)));
      connect(velocityEntry, SIGNAL(valueChanged(int)), SLOT(setVelocity(int)));
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------
void TriggGui::init()
      {
      Trigg::initData *data;
      int n;
      filter->getInitData(&n,(const unsigned char **)&data);
      printf("::init note=%d vel=%d\n",data->note,data->velocity);
      noteEntry->setValue(data->note);
      velocityEntry->setValue(data->velocity);
      }

//---------------------------------------------------------
//   setNote
//---------------------------------------------------------
void TriggGui::setNote(int value)
      {
      printf("TriggGui::setNote %d\n",value);
      filter->setNote(value);
      }

//---------------------------------------------------------
//   setVelocity
//---------------------------------------------------------
void TriggGui::setVelocity(int value)
      {
      printf("TriggGui::setVelocity %d\n",value);
      filter->setVelocity(value);
      }

