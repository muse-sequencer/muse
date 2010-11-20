//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrlcombo.h,v 1.1.1.1 2003/10/27 18:54:30 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRLGRP_H__
#define __CTRLGRP_H__

#include <QComboBox>

class CtrlComboBox : public QComboBox {
      Q_OBJECT
   public:
      CtrlComboBox(QWidget* parent);
      };


#endif

