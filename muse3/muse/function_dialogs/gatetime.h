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
#include "function_dialog_base.h"

class QButtonGroup;

namespace MusECore {
class Xml;
}

namespace MusEGui {

//---------------------------------------------------------
//   GateTime
//---------------------------------------------------------

class GateTime : public FunctionDialogBase, public Ui::GateTimeBase {
 	Q_OBJECT

   protected slots:
      void pull_values();

   public:
      GateTime(QWidget* parent=0);

      static int _range;
      static int _parts;
      static int rateVal;
      static int offsetVal;
      static FunctionReturnDialogFlags_t _ret_flags;
      static FunctionDialogElements_t _elements;
      
      static void setElements(FunctionDialogElements_t elements =
        FunctionAllEventsButton | FunctionSelectedEventsButton |
        FunctionLoopedButton | FunctionSelectedLoopedButton) { _elements = elements; }
        
      static void read_configuration(MusECore::Xml& xml);
      void write_configuration(int level, MusECore::Xml& xml);
      
      void setupDialog();
      
      int curRange() const { return _range; }
      void setCurRange(int range) { _range = range; }
      int curParts() const { return _parts; }
      void setCurParts(int parts) { _parts = parts; }
      void setReturnFlags(FunctionReturnDialogFlags_t f) { _ret_flags = f; }
      FunctionDialogElements_t elements() const { return _elements; }
      };

} // namespace MusEGui

#endif

