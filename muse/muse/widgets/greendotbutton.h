//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: greendotbutton.h,v 1.2 2005/11/05 11:05:25 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __GREENDOTBUTTON_H__
#define __GREENDOTBUTTON_H__

#include "simplebutton.h"

//---------------------------------------------------------
//   GreendotButton
//---------------------------------------------------------

class GreendotButton : public SimpleButton {
      Q_OBJECT

   public:
      GreendotButton(QWidget* parent = 0);
      };

#endif

