//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.cpp,v 1.7.2.8 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>

#include <QFileDialog>
#include <QRect>
#include <QShowEvent>

#include "mdisettings.h"
#include "app.h"
#include "gconfig.h"
#include "midiseq.h"
#include "globals.h"
#include "icons.h"

MdiSettings::MdiSettings(TopWin::ToplevelType t, QWidget* parent) : QWidget(parent)
{
  _type=t;
  setupUi(this);
  
  groupBox->setTitle(TopWin::typeName(t)); 
  update_settings();
}


void MdiSettings::update_settings()
{
  isSubwinCheckbox->setChecked(TopWin::_defaultSubwin[_type]);
  shareSubwinCheckbox->setChecked(TopWin::_sharesWhenSubwin[_type]);
  shareFreeCheckbox->setChecked(TopWin::_sharesWhenFree[_type]);
}

void MdiSettings::apply_settings()
{
  TopWin::_defaultSubwin[_type] = isSubwinCheckbox->isChecked();
  TopWin::_sharesWhenSubwin[_type] = shareSubwinCheckbox->isChecked();
  TopWin::_sharesWhenFree[_type] = shareFreeCheckbox->isChecked();
}
