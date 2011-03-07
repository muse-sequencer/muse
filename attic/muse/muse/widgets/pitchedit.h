//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchedit.h,v 1.2 2004/01/09 17:12:54 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PITCHEDIT_H__
#define __PITCHEDIT_H__

#include <qspinbox.h>

//---------------------------------------------------------
//   PitchEdit
//---------------------------------------------------------

class PitchEdit : public QSpinBox {
      Q_OBJECT

      bool deltaMode;

   protected:
      virtual QString mapValueToText(int v);
      virtual int mapTextToValue(bool* ok);

   public:
      PitchEdit(QWidget* parent, const char* name = 0);
      void setDeltaMode(bool);
      };

extern QString pitch2string(int v);

#endif
