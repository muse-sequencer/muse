//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: velocity.h,v 1.1.1.1 2003/10/27 18:54:51 wschweer Exp $
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

#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include "ui_velocitybase.h"

class QButtonGroup;

namespace MusECore {
class Xml;
}

namespace MusEGui {

//---------------------------------------------------------
//   Velocity
//---------------------------------------------------------

class Velocity : public QDialog, public Ui::VelocityBase {
      Q_OBJECT
   private:
      
      QButtonGroup* rangeGroup;

   protected slots:
      void accept();
      void pullValues();

   public:
      Velocity(QWidget* parent = 0);

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

