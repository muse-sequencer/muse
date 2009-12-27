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

#ifndef __TRANSPORT_H__
#define __TRANSPORT_H__

#include "ui_transport.h"

namespace AL {
      class Pos;
      };
using AL::Pos;

//---------------------------------------------------------
//   Transport
//---------------------------------------------------------

class Transport : public QWidget, public Ui::TransportBase
      {
      QToolButton* buttons[6];      // transport buttons
      Q_OBJECT

      virtual void closeEvent(QCloseEvent*);

   private slots:
      void cposChanged(const Pos&);
      void cposChanged(int);
      void lposChanged(const Pos&);
      void rposChanged(const Pos&);
      void setRecMode(int);
      void setCycleMode(int);
      void songChanged(int);
      void syncChanged(bool);
      void setRecord(bool flag);
      void stopToggled(bool);
      void playToggled(bool);
	void setLen(const AL::Pos& len);

   public slots:
      void setTimesig(int a, int b);
      void setPos(int idx, const AL::Pos& pos);
      void setMasterFlag(bool);
      void setQuantizeFlag(bool);
      void setSyncFlag(bool);
      void setPlay(bool f);
      void syncButtonClicked(bool);
      void syncChanged();

   signals:
      void closed();

   public:
      Transport();
      void setValues();
      };
#endif

