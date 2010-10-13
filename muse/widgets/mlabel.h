//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mlabel.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MLABEL_H__
#define __MLABEL_H__

#include <qlabel.h>
//Added by qt3to4:
#include <QMouseEvent>

//---------------------------------------------------------
//   MLabel
//    label widged which sends signal mousePressed
//    on mousePressEvent
//---------------------------------------------------------

class MLabel : public QLabel {

      Q_OBJECT

   protected:
      virtual void mousePressEvent(QMouseEvent*);

   signals:
      void mousePressed();

   public:
      MLabel(const QString& txt, QWidget* parent, const char* name = 0)
         : QLabel(txt, parent, name) {};

      MLabel(QWidget* parent, const char* name = 0)
         : QLabel(parent, name) {};
      };
#endif

