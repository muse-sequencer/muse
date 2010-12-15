//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lcombo.cpp,v 1.1.1.1.2.3 2009/07/01 22:14:56 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "lcombo.h"

#include <QHBoxLayout>
#include <QLabel>


//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

LabelCombo::LabelCombo(const QString& txt, QWidget* parent,
   const char* name) : QWidget(parent)
      {
      setObjectName(name);
//      setFixedHeight(20);
      QHBoxLayout* layout = new QHBoxLayout(this);
      QLabel* label = new QLabel(txt, this);
      //box = new QComboBox(false, this);
      box = new QComboBox(this);
      box->setEditable(false);
      ///layout->addStretch();
      layout->addSpacing(5);
      layout->addWidget(label);
      layout->addSpacing(5);
      layout->addWidget(box);
      layout->addSpacing(5);
      ///layout->addStretch();
      connect(box, SIGNAL(activated(int)), SIGNAL(activated(int)));
      }

void LabelCombo::setCurrentIndex(int i) 
{ 
  int rc = box->model()->rowCount();
  if(rc == 0)
    return;
  int r = i % rc;
  int c = i / rc;
  if(c >= box->model()->columnCount())
    return;
  box->setModelColumn(c);
  box->setCurrentIndex(r); 
} 
