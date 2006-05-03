//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: selectfilter.h,v 1.1 2005/01/24 14:32:34 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SELECTFILTER_H__
#define __SELECTFILTER_H__

//---------------------------------------------------------
//   SelectFilter
//---------------------------------------------------------

class SelectFilter :public QObject
      {
      QWidget* w;
      Track* t;

   protected:
      bool eventFilter(QObject*, QEvent* ev)
            {
            if (ev->type() == QEvent::MouseButtonPress) {
                  w->setFocus();
                  song->selectTrack(t);
                  }
            return false;
            }
   public:
      SelectFilter(QObject* parent, QWidget* widget, Track* track)
         : QObject(parent) {
            w = widget;
            t = track;
            }
      };

#endif

