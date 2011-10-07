//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/bigtime.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================
#ifndef __BIGTIME_H__
#define __BIGTIME_H__

#include <QWidget>

class QCheckBox;
class QLabel;

namespace MusEGui {

class MusE;

//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

class BigTime : public QWidget {
      Q_OBJECT
    
      bool tickmode;
      MusE* seq;
      

      bool setString(unsigned);

      QWidget *dwin;
      QCheckBox *fmtButton;
      QLabel *absTickLabel;
      QLabel *absFrameLabel;
      QLabel *barLabel, *beatLabel, *tickLabel,
             //*hourLabel, *minLabel, *secLabel, *frameLabel,
             *minLabel, *secLabel, *frameLabel, *subFrameLabel, 
             *sep1, *sep2, *sep3, *sep4, *sep5;
      
      //int oldbar, oldbeat, oldhour, oldmin, oldsec, oldframe;
      int oldbar, oldbeat, oldmin, oldsec, oldframe, oldsubframe;
      unsigned oldtick;
      unsigned oldAbsTick, oldAbsFrame;
      void setFgColor(QColor c);
      void setBgColor(QColor c);

   protected:
      virtual void resizeEvent(QResizeEvent*);
      virtual void closeEvent(QCloseEvent*);

   public slots:
      void setPos(int, unsigned, bool);
      void configChanged();
      void fmtButtonToggled(bool);
   signals:
      void closed();

   public:
      BigTime(QWidget* parent);
      };

} // namespace MusEGui

#endif
