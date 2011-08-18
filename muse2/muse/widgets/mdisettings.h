//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MDISETTINGS_H__
#define __MDISETTINGS_H__

#include <QWidget>
#include "ui_mdisettings_base.h"
#include "cobject.h"

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class MdiSettings : public QWidget, private Ui::MdiSettingsBase
{
  Q_OBJECT
  
  private:
    TopWin::ToplevelType _type;

  public:
    MdiSettings(TopWin::ToplevelType t, QWidget* parent=0);
    void update_settings();
    void apply_settings();
    TopWin::ToplevelType type() { return _type; }
};

#endif
