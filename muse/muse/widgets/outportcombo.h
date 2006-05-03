//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: outportcombo.h,v 1.2 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer
//=========================================================

#ifndef __OUTPORTCOMBO_H__
#define __OUTPORTCOMBO_H__

//---------------------------------------------------------
//   OutportCombo
//---------------------------------------------------------

class OutportCombo : public QComboBox {
      Q_OBJECT

   private slots:
      void populate();

   public:
      OutportCombo(QWidget* parent = 0);
      };

#endif

