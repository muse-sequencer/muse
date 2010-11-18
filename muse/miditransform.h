//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: miditransform.h,v 1.2.2.2 2009/02/02 21:38:00 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDITRANSFORM_H__
#define __MIDITRANSFORM_H__

#include "ui_transformbase.h"

class QListWidgetItem;
class QDialog;
class MidiTransformation;
class MidiTransformPrivate;
class Event;
class MidiPart;
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

//---------------------------------------------------------
//   MidiTransformDialogBaseWidget
//   Wrapper around Ui::MidiTransformDialogBase
//---------------------------------------------------------

class MidiTransformDialogBaseWidget : public QDialog, public Ui::MidiTransformDialogBase
{
      Q_OBJECT

   public:
      MidiTransformDialogBaseWidget(QDialog *parent = 0, Qt::WFlags f = 0) : QDialog(parent, f) { setupUi(this); }
};

//---------------------------------------------------------
//   MidiTransformDialog
//---------------------------------------------------------

class MidiTransformerDialog : public MidiTransformDialogBaseWidget {
      Q_OBJECT
      MidiTransformPrivate* data;

      virtual void accept();
//      virtual void reject();
      void setValOp(QWidget* a, QWidget* b, ValOp op);
      void processEvent(Event&, MidiPart*, MidiPart*);
      bool isSelected(Event&, MidiPart*);
      void transformEvent(Event&, MidiPart*, MidiPart*);
      bool typesMatch(Event& e, unsigned selType);
      
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

extern void writeMidiTransforms(int level, Xml& xml);
extern void readMidiTransform(Xml&);
extern void clearMidiTransforms();
#endif
