//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: gatetime.h,v 1.1.1.1.2.1 2008/01/19 13:33:47 wschweer Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __GATETIME_H__
#define __GATETIME_H__

#include "ui_gatetimebase.h"

class QButtonGroup;

namespace MusECore {
class Xml;
}

namespace MusEGui {

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

class GateTime : public QDialog, public Ui::GateTimeBase {
 	Q_OBJECT
   private:
      
      QButtonGroup *rangeGroup;

   protected slots:
      void accept();
      void pullValues();

   public:
      GateTime(QWidget* parent=0);

      static int range;
      static int rateVal;
      static int offsetVal;
      
      static void read_configuration(MusECore::Xml& xml);
      void write_configuration(int level, MusECore::Xml& xml);

      
   public slots:
      int exec();
      };

} // namespace MusEGui

#endif

