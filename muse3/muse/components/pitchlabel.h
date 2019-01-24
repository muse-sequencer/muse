//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pitchlabel.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
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

#ifndef __PITCHLABEL_H__
#define __PITCHLABEL_H__

#include <QLabel>

namespace MusEGui {

//---------------------------------------------------------
//   PitchLabel
//---------------------------------------------------------

class PitchLabel : public QLabel {
      Q_OBJECT
    
      bool _pitchMode;
      int _value;
      

   protected:
      QSize sizeHint() const;

   public slots:
      void setValue(int);
      void setInt(int);
      void setPitch(int);

   public:
      PitchLabel(QWidget* parent, const char* name = 0);
      int value() const { return _value; }
      void setPitchMode(bool val);
      bool pitchMode() const { return _pitchMode; }
      };

} // namespace MusEGui

#endif



