//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: quantconfig.h,v 1.3 2006/01/25 16:24:33 wschweer Exp $
//
//  (C) Copyright 2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __QCONFIG_H__
#define __QCONFIG_H__

#include "ui_quantconfig.h"

//---------------------------------------------------------
//   QuantConfig
//---------------------------------------------------------

class QuantConfig : public QDialog, public Ui::QuantConfigBase {
      Q_OBJECT

   public:
      QuantConfig(int, int, bool, QWidget* parent = 0);
      int quantStrength() const;
      int quantLimit() const;
      bool doQuantLen() const;
      };


#endif

