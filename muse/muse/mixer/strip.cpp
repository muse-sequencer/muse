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

#include "gconfig.h"
#include "song.h"
#include "strip.h"
#include "muse.h"
#include "widgets/simplebutton.h"
#include "widgets/utils.h"

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Strip::resetPeaks()
      {
      track->resetPeaks();
      }

//---------------------------------------------------------
//   updateLabel
//---------------------------------------------------------

void Strip::updateLabel()
      {
      QPalette p = label->palette();
      p.setColor(label->backgroundRole(), track->ccolor());
      label->setPalette(p);
      label->setAutoFillBackground(true);
      label->setFont(config.fonts[4]);
      label->setText(track->name());
      label->setToolTip(track->name());
      }

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(Mixer* m, Track* t, bool align)
   : QFrame()
      {
      setAttribute(Qt::WA_DeleteOnClose, true);
      mixer = m;
      _align = align;
      setFrameStyle(QFrame::Panel | QFrame::Raised);
      setLineWidth(2);

      track = t;
      layout = new QVBoxLayout;
      layout->setMargin(0);
      layout->setSpacing(0);
      setLayout(layout);
//      setFixedWidth(STRIP_WIDTH);

      //---------------------------------------------
      //    label
      //---------------------------------------------

      label = new QLabel;
      label->setFixedHeight(LABEL_HEIGHT);
      label->setTextFormat(Qt::PlainText);
      label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      label->setLineWidth(2);
      label->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
      updateLabel();
      layout->addWidget(label);
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      connect(track, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      }

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

Strip::~Strip()
      {
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Strip::configChanged()
      {
      updateLabel();
      }

//---------------------------------------------------------
//   addAutomationButtons
//---------------------------------------------------------

void Strip::addAutomationButtons()
      {
      QHBoxLayout* aBox = new QHBoxLayout;
      SimpleButton* ar = newAutoReadButton();
      ar->setFixedSize(buttonSize);
      ar->setChecked(track->autoRead());
      aBox->addWidget(ar);
      SimpleButton* aw = newAutoWriteButton();
      aw->setFixedSize(buttonSize);
      aw->setChecked(track->autoWrite());
      aBox->addWidget(aw);
      layout->addLayout(aBox);
      connect(ar, SIGNAL(clicked(bool)), SLOT(autoReadToggled(bool)));
      connect(aw, SIGNAL(clicked(bool)), SLOT(autoWriteToggled(bool)));
      connect(track, SIGNAL(autoReadChanged(bool)), ar, SLOT(setChecked(bool)));
      connect(track, SIGNAL(autoWriteChanged(bool)), aw, SLOT(setChecked(bool)));
      }


