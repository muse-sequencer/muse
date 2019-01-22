//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: poslabel.h,v 1.2 2004/01/11 18:55:37 wschweer Exp $
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

#ifndef __POSLABEL_H__
#define __POSLABEL_H__

#include <QLabel>

namespace MusEGui {

//---------------------------------------------------------
//   PosLabel
//---------------------------------------------------------

class PosLabel : public QLabel {
      Q_OBJECT
    
      bool _smpte;
      unsigned _tickValue;
      unsigned _sampleValue;
      

      void updateValue();

   protected:
      QSize sizeHint() const;

   public slots:
      void setTickValue(unsigned);
      void setSampleValue(unsigned);
      void setValue(unsigned);

   public:
      PosLabel(QWidget* parent, const char* name = 0);
      unsigned value()       const { return _smpte ? _sampleValue : _tickValue; }
      unsigned tickValue()   const { return _tickValue; }
      unsigned sampleValue() const { return _sampleValue; }
      void setSmpte(bool);
      bool smpte() const { return _smpte; }
      };

} // namespace MusEGui

#endif


