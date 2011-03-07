//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: fontsel.h,v 1.1.1.1.2.1 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __FONTSEL_H__
#define __FONTSEL_H__

#include <qwidget.h>
#include "song.h"

class QFont;
class QSpinBox;
class QToolButton;
class QComboBox;

//---------------------------------------------------------
//   FontSel
//---------------------------------------------------------

class FontSel : public QWidget {
      Q_OBJECT

      QFont _font;
      QSpinBox* s1;
      QToolButton* fcb1;
      QToolButton* fcb2;
      QToolButton* fcb3;
      QComboBox* cb;

      void setFont();

   private slots:
      void fontSelect();

   public:
      FontSel(QWidget* parent, const QFont&, const QString&);
      const QFont& font();
      };


#endif

