//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: transform.h,v 1.5 2005/10/05 17:02:03 lunar_shuttle Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "../libmidiplugin/mempi.h"
#include "ui_transform.h"

class Transform;

namespace AL {
      class Xml;
      };
using AL::Xml;

enum ValOp {
      All=0, Ignore=0, Equal=1, Unequal=2, Higher=3, Lower=4,
      Inside=5, Outside=6
      };

enum TransformFunction {
      Select, Quantize, Delete, Trans, Insert, Copy, Extract
      };

enum TransformOperator {
      Keep, Plus, Minus, Multiply, Divide, Fix, Value, Invert,
      ScaleMap, Flip, Dynamic, Random
      };

//---------------------------------------------------------
//   TransformDialog
//---------------------------------------------------------

class TransformDialog : public QDialog, public Ui::TransformDialogBase {
      Q_OBJECT
      Transform* cmt;

      void setValOp(QWidget* a, QWidget* b, ValOp op);

   signals:
      void hideWindow();

   private slots:
      void selEventOpSel(int);
      void selTypeSel(int);
      void selVal1OpSel(int);
      void selVal2OpSel(int);
      void procEventOpSel(int);
      void procEventTypeSel(int);
      void procVal1OpSel(int);
      void procVal2OpSel(int);
      void funcOpSel(int);
      void selVal1aChanged(int);
      void selVal1bChanged(int);
      void selVal2aChanged(int);
      void selVal2bChanged(int);
      void procVal1aChanged(int);
      void procVal1bChanged(int);
      void procVal2aChanged(int);
      void procVal2bChanged(int);
      void selChannelOpSel(int);
      void selChannelValaChanged(int);
      void selChannelValbChanged(int);
      void procChannelOpSel(int);
      void procChannelValaChanged(int);
      void procChannelValbChanged(int);

   public:
      TransformDialog(Transform*, QWidget* parent);
      void init();
      };

//---------------------------------------------------------
//   Transform
//---------------------------------------------------------

class Transform : public Mempi {
      int filterEvent(MidiEvent& event);

   public:
      struct initData {
            ValOp selEventOp;
            int selType;

            ValOp selVal1;
            int selVal1a, selVal1b;
            ValOp selVal2;
            int selVal2a, selVal2b;
            ValOp selChannel;
            int selChannela, selChannelb;

            TransformOperator procEvent;
            int eventType;
            TransformOperator procVal1;
            int procVal1a, procVal1b;
            TransformOperator procVal2;
            int procVal2a, procVal2b;
            TransformOperator procChannel;
            int procChannela, procChannelb;

            TransformFunction funcOp;
            int quantVal;
            } data;

      TransformDialog* gui;
      virtual void process(unsigned, unsigned, MidiEventList*, MidiEventList*);

      Transform(const char* name, const MempiHost*);
      virtual bool init();
      virtual bool hasGui() const      { return true;             }
      virtual bool guiVisible() const  { return gui->isVisible(); }
      virtual void showGui(bool val)   { gui->setShown(val);      }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int);
      virtual void getInitData(int*, const unsigned char**) const;
      virtual void setInitData(int, const unsigned char*);
      };

#endif

