//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: fontsel.cpp,v 1.1.1.1 2003/10/27 18:55:02 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <qlabel.h>
#include <qspinbox.h>
#include <qpushbutton.h>
//#include <qhbuttongroup.h>
#include <q3buttongroup.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qfontdialog.h>
#include <qfontdatabase.h>
//Added by qt3to4:
#include <Q3HBoxLayout>

#include "icons.h"
#include "fontsel.h"

//---------------------------------------------------------
//   FontSel
//---------------------------------------------------------

FontSel::FontSel(QWidget* parent, const QFont& f, const QString& name)
  : QWidget(parent)
      {
      _font = f;
      Q3HBoxLayout* box = new Q3HBoxLayout(this);

      cb = new QComboBox(this);
      cb->setFixedWidth(80);
      cb->insertItem(QString("arial"));
      cb->insertItem(QString("avantgarde"));
      cb->insertItem(QString("charter"));
      cb->insertItem(QString("garamond"));
      cb->insertItem(QString("gillsans"));
      cb->insertItem(QString("helvetica"));
      cb->insertItem(QString("times"));
      cb->insertItem(QString("terminal"));
      cb->insertItem(QString("utopia"));
      cb->insertItem(QString("new century schoolbook"));

      QLabel* l1 = new QLabel(tr("Size:"), this);
      s1 = new QSpinBox(8, 48, 1, this);

      fcb1 = new QToolButton(this);
      fcb1->setToggleButton(true);
      fcb1->setPixmap(*(boldIcon));

      fcb2 = new QToolButton(this);
      fcb2->setToggleButton(true);
      fcb2->setPixmap(*(italicIcon));

      fcb3 = new QToolButton(this);
      fcb3->setToggleButton(true);
      fcb3->setPixmap(*(underlinedIcon));

      QToolButton* pb = new QToolButton(this);
      connect(pb, SIGNAL(pressed()), SLOT(fontSelect()));
      pb->setText(QString("??"));

      QLabel* l2 = new QLabel(name, this);

      box->addWidget(cb);
      box->addSpacing(8);
      box->addWidget(l1);
      box->addSpacing(5);
      box->addWidget(s1);
      box->addSpacing(5);
      box->addWidget(fcb1);
      box->addWidget(fcb2);
      box->addWidget(fcb3);
      box->addSpacing(2);
      box->addWidget(pb);
      box->addSpacing(5);
      box->addWidget(l2);
      box->addStretch(100);
      setFixedHeight(18);
      setFont();
      }

//---------------------------------------------------------
//   fontSelect
//---------------------------------------------------------

void FontSel::fontSelect()
      {
      bool ok;
      QFont f = QFontDialog::getFont(&ok, _font, this);
      if (ok) {
            _font = f;
            setFont();
            }
      }

//---------------------------------------------------------
//   setFont
//---------------------------------------------------------

void FontSel::setFont()
      {
      s1->setValue(_font.pointSize());
      fcb1->setOn(_font.weight() == QFont::Bold);
      fcb2->setOn(_font.italic());
      fcb3->setOn(_font.underline());
      int i;
      for (i = 0; i < cb->count(); ++i) {
            QString s = cb->text(i);
            if (s == _font.family()) {
                  cb->setCurrentItem(i);
                  break;
                  }
            }
      if (i == cb->count()) {
            cb->insertItem(_font.family());
            cb->setCurrentItem(i);
            }
      }

//---------------------------------------------------------
//   font
//---------------------------------------------------------

const QFont& FontSel::font()
      {
      _font.setPointSize(s1->value());
      _font.setWeight(fcb1->isOn() ? QFont::Bold : QFont::Normal);
      _font.setItalic(fcb2->isOn());
      _font.setUnderline(fcb3->isOn());
      _font.setFamily(cb->currentText());
      return _font;
      }

