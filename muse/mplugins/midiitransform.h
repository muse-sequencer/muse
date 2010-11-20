//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiitransform.h,v 1.1.1.1.2.1 2009/02/02 21:38:01 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIITRANSFORM_H__
#define __MIDIITRANSFORM_H__

#include "ui_itransformbase.h"

#include <QCloseEvent>

class QButtonGroup;
class MidiTransformation;
class MidiInputTransformation;
class MidiRecordEvent;
class MidiPart;
class Xml;

#include "miditransform.h"

enum InputTransformProcEventOp { KeepType, FixType };

//---------------------------------------------------------
//   MidiInputTransform
//---------------------------------------------------------

class MidiInputTransformDialog : public QDialog, public Ui::MidiInputTransformDialogBase {
      Q_OBJECT
      MidiInputTransformation* cmt;
      int cindex;                   // current index in preset list
      int cmodul;                   // current index in modules list

      virtual void accept();
      virtual void reject();
      void setValOp(QWidget* a, QWidget* b, ValOp op);
      virtual void closeEvent(QCloseEvent*);
      
      void updatePresetList();
      QButtonGroup* modulGroup;
      
   signals:
      void hideWindow();

   private slots:
      void presetNew();
      void presetDelete();

      void changeModul(int k);
      void selEventOpSel(int);
      void selTypeSel(int);
      void selVal1OpSel(int);
      void selVal2OpSel(int);
      void procEventOpSel(int);
      void procEventTypeSel(int);
      void procVal1OpSel(int);
      void procVal2OpSel(int);
      void funcOpSel(int);
      void presetChanged(QListWidgetItem*);
      void nameChanged(const QString&);
      void commentChanged();
      void selVal1aChanged(int);
      void selVal1bChanged(int);
      void selVal2aChanged(int);
      void selVal2bChanged(int);
      void procVal1aChanged(int);
      void procVal1bChanged(int);
      void procVal2aChanged(int);
      void procVal2bChanged(int);
      void modul1enableChanged(bool);
      void modul2enableChanged(bool);
      void modul3enableChanged(bool);
      void modul4enableChanged(bool);

      void selPortOpSel(int);
      void selPortValaChanged(int);
      void selPortValbChanged(int);
      void selChannelOpSel(int);
      void selChannelValaChanged(int);
      void selChannelValbChanged(int);
      void procPortOpSel(int);
      void procPortValaChanged(int);
      void procPortValbChanged(int);
      void procChannelOpSel(int);
      void procChannelValaChanged(int);
      void procChannelValbChanged(int);

   public slots:
      void songChanged(int);

   public:
      MidiInputTransformDialog(QDialog* parent = 0, Qt::WFlags fl = 0);
      };

extern void writeMidiInputTransforms(int level, Xml& xml);
extern void readMidiInputTransform(Xml&);
extern bool applyMidiInputTransformation(MidiRecordEvent& event);
extern void clearMidiInputTransforms();
#endif
