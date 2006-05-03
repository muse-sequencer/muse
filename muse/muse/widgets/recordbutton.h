//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: recordbutton.h,v 1.2 2005/11/05 11:05:25 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __RECORDBUTTON_H__
#define __RCORDBUTTON_H__

#include "simplebutton.h"

//---------------------------------------------------------
//   RecordButton
//---------------------------------------------------------

class RecordButton : public SimpleButton {
      Q_OBJECT

   public:
      RecordButton(QWidget* parent = 0);
      };

#endif

