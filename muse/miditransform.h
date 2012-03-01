//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: miditransform.h,v 1.2.2.2 2009/02/02 21:38:00 terminator356 Exp $
//
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

#ifndef __MIDITRANSFORM_H__
#define __MIDITRANSFORM_H__

#include "ui_transformbase.h"

class QListWidgetItem;
class QDialog;

namespace MusECore {

class Event;
class MidiPart;
class MidiTransformation;
class MidiTransformPrivate;
class Xml;

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

extern void writeMidiTransforms(int level, Xml& xml);
extern void readMidiTransform(Xml&);
extern void clearMidiTransforms();

} // namespace MusECore

namespace MusEGui {

//---------------------------------------------------------
//   MidiTransformDialog
//---------------------------------------------------------

class MidiTransformerDialog : public QDialog, public Ui::MidiTransformDialogBase {
      Q_OBJECT
      MusECore::MidiTransformPrivate* data;

      virtual void accept();
      void setValOp(QWidget* a, QWidget* b, MusECore::ValOp op);
      void processEvent(MusECore::Event&, MusECore::MidiPart*, MusECore::MidiPart*);
      bool isSelected(MusECore::Event&, MusECore::MidiPart*);
      void transformEvent(MusECore::Event&, MusECore::MidiPart*, MusECore::MidiPart*);
      bool typesMatch(MusECore::Event& e, unsigned selType);
      
      void updatePresetList();

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

   public slots:
      void songChanged(int);

   public:
      MidiTransformerDialog(QDialog* parent = 0, Qt::WFlags fl = 0);
      ~MidiTransformerDialog();
      };

} // namespace MusEGui

#endif
