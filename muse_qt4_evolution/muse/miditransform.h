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

#ifndef __MIDITRANSFORM_H__
#define __MIDITRANSFORM_H__

#include "widgets/transformbase.h"

class MidiTransformation;
class MidiTransformPrivate;
class Event;
class Part;

namespace AL {
      class Xml;
      };
using AL::Xml;

enum ValOp {
      All=0, Ignore=0, Equal=1, Unequal=2, Higher=3, Lower=4,
      Inside=5, Outside=6
      };

enum TransformFunction {
      Select, Quantize, Delete, Transform, Insert, Copy, Extract
      };

enum TransformOperator {
      Keep, Plus, Minus, Multiply, Divide, Fix, Value, Invert,
      ScaleMap, Flip, Dynamic, Random
      };

//---------------------------------------------------------
//   MidiTransformDialog
//---------------------------------------------------------

class MidiTransformerDialog : public MidiTransformDialogBase {
      Q_OBJECT
      MidiTransformPrivate* data;

      virtual void accept();
//      virtual void reject();
      void setValOp(QWidget* a, QWidget* b, ValOp op);
      void processEvent(Event&, Part*, Part*);
      bool isSelected(Event&, Part*);
      void transformEvent(Event&, Part*, Part*);

   private slots:
      void apply();
      void presetNew();
      void presetDelete();

      void selEventOpSel(int);
      void selTypeSel(int);
      void selVal1OpSel(int);
      void selVal2OpSel(int);
      void selLenOpSel(int);
      void selRangeOpSel(int);
      void procEventOpSel(int);
      void procEventTypeSel(int);
      void procVal1OpSel(int);
      void procVal2OpSel(int);
      void procLenOpSel(int);
      void procPosOpSel(int);
      void funcOpSel(int);
      void presetChanged(QListWidgetItem*);
      void nameChanged(const QString&);
      void commentChanged();
      void selVal1aChanged(int);
      void selVal1bChanged(int);
      void selVal2aChanged(int);
      void selVal2bChanged(int);
      void selLenAChanged(int);
      void selLenBChanged(int);
      void selBarAChanged(int);
      void selBarBChanged(int);
      void procVal1aChanged(int);
      void procVal1bChanged(int);
      void procVal2aChanged(int);
      void procVal2bChanged(int);
      void procLenAChanged(int);
      void procPosAChanged(int);
      void funcQuantValSel(int);
      void processAllChanged(bool);
      void selectedTracksChanged(bool);
      void insideLoopChanged(bool);

   public:
      MidiTransformerDialog(QWidget* parent = 0, const char* name = 0,
         bool modal = false, Qt::WFlags fl = 0);
      ~MidiTransformerDialog();
      };

extern void writeMidiTransforms(Xml& xml);
extern void readMidiTransform(QDomNode);
#endif
