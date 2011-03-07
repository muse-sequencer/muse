//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: action.h,v 1.1.1.1.2.1 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ACTION_H__
#define __ACTION_H__

#include <qaction.h>

//---------------------------------------------------------
//   Action
//---------------------------------------------------------

class Action : public QAction {
      Q_OBJECT
      int _id;

   public:
      Action(QObject* parent, int i, const char* name = 0, bool toggle = false)
         : QAction(parent, name, toggle) {
            _id = i;
            }
      void setId(int i) { _id = i; }
      int id() const    { return _id; }
      };


#endif

