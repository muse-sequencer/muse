//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

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

