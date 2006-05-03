//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: action.h,v 1.2 2004/09/14 18:17:46 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ACTION_H__
#define __ACTION_H__

//---------------------------------------------------------
//   Action
//---------------------------------------------------------

class Action : public QAction {
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

