//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrlcombo.h,v 1.2 2005/11/06 17:49:34 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRLGRP_H__
#define __CTRLGRP_H__

//---------------------------------------------------------
//   CtrlComboBox
//---------------------------------------------------------

class CtrlComboBox : public QComboBox {
    Q_OBJECT
   public:
      CtrlComboBox(QWidget* parent = 0);
      };


#endif

