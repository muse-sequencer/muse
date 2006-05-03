//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: fontsel.h,v 1.2 2004/09/14 18:17:47 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __FONTSEL_H__
#define __FONTSEL_H__

#include "song.h"

class QFont;
class QSpinBox;
class QToolButton;
class QComboBox;

//---------------------------------------------------------
//   FontSel
//---------------------------------------------------------

class FontSel : public QWidget {
      QFont _font;
      QSpinBox* s1;
      QToolButton* fcb1;
      QToolButton* fcb2;
      QToolButton* fcb3;
      QComboBox* cb;

      Q_OBJECT

      void setFont();

   private slots:
      void fontSelect();

   public:
      FontSel(QWidget* parent, const QFont&, const QString&);
      const QFont& font();
      };


#endif

