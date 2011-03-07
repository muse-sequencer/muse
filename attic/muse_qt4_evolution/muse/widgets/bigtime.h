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

#ifndef __BIGTIME_H__
#define __BIGTIME_H__

#include "al/pos.h"

class MusE;
class AL::Pos;

//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

class BigTime : public QWidget {
      bool tickmode;
      MusE* seq;
      Q_OBJECT

      bool setString(AL::Pos);

      QLabel *barLabel, *beatLabel, *tickLabel,
             *hourLabel, *minLabel, *secLabel, *frameLabel,
             *sep1, *sep2, *sep3, *sep4, *sep5;
      int oldbar, oldbeat, oldhour, oldmin, oldsec, oldframe;
      int oldtick;

      void setFgColor(QColor c);
      void setBgColor(QColor c);

   protected:
      virtual void resizeEvent(QResizeEvent*);
      virtual void closeEvent(QCloseEvent*);

   public slots:
      void setPos(int, AL::Pos, bool);
      void configChanged();

   signals:
      void closed();

   public:
      BigTime(QWidget* parent);
      };

#endif
