//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: quantconfig.h,v 1.1.1.1 2003/10/27 18:52:23 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __QCONFIG_H__
#define __QCONFIG_H__

#include <QDialog>

//---------------------------------------------------------
//   QuantConfig
//---------------------------------------------------------

class QuantConfig : public QDialog {
      Q_OBJECT

   signals:
      void setQuantStrength(int);
      void setQuantLimit(int);
      void setQuantLen(bool);

   public:
      QuantConfig(int, int, bool);
      };


#endif

