//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: fontsel.cpp,v 1.5 2005/11/04 12:03:48 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "icons.h"
#include "fontsel.h"

//---------------------------------------------------------
//   FontSel
//---------------------------------------------------------

FontSel::FontSel(QWidget* parent, const QFont& f, const QString& name)
  : QWidget(parent)
      {
      _font = f;
      QHBoxLayout* box = new QHBoxLayout(this);

      cb = new QComboBox(this);
      cb->setFixedWidth(80);
      cb->addItem(QString("arial"));
      cb->addItem(QString("avantgarde"));
      cb->addItem(QString("charter"));
      cb->addItem(QString("garamond"));
      cb->addItem(QString("gillsans"));
      cb->addItem(QString("helvetica"));
      cb->addItem(QString("times"));
      cb->addItem(QString("terminal"));
      cb->addItem(QString("utopia"));
      cb->addItem(QString("new century schoolbook"));

      QLabel* l1 = new QLabel(tr("Size:"), this);
      s1 = new QSpinBox(this);
      s1->setRange(8, 48);

      fcb1 = new QToolButton(this);
      fcb1->setCheckable(true);
      fcb1->setIcon(*boldIcon);

      fcb2 = new QToolButton(this);
      fcb2->setCheckable(true);
      fcb2->setIcon(*italicIcon);

      fcb3 = new QToolButton(this);
      fcb3->setCheckable(true);
      fcb3->setIcon(*underlinedIcon);

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
      fcb1->setChecked(_font.weight() == QFont::Bold);
      fcb2->setChecked(_font.italic());
      fcb3->setChecked(_font.underline());
      int i;
      for (i = 0; i < cb->count(); ++i) {
            QString s = cb->itemText(i);
            if (s == _font.family()) {
                  cb->setCurrentIndex(i);
                  break;
                  }
            }
      if (i == cb->count()) {
            cb->addItem(_font.family());
            cb->setCurrentIndex(i);
            }
      }

//---------------------------------------------------------
//   font
//---------------------------------------------------------

const QFont& FontSel::font()
      {
      _font.setPointSize(s1->value());
      _font.setWeight(fcb1->isChecked() ? QFont::Bold : QFont::Normal);
      _font.setItalic(fcb2->isChecked());
      _font.setUnderline(fcb3->isChecked());
      _font.setFamily(cb->currentText());
      return _font;
      }

