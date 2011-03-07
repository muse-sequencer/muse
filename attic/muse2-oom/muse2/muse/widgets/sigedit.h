//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sigedit.h,v 1.1.1.1.2.1 2004/12/28 23:23:51 lunar_shuttle Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SIGEDIT_H__
#define __SIGEDIT_H__

#include <QWidget>

#include "section.h"

class QResizeEvent;
class QTimerEvent;

class SigEditor;
class SpinBox;

struct Sig {
      int z;
      int n;
   public:
      Sig(int _z, int _n) : z(_z), n(_n) {}
      bool isValid() const;
      };

//---------------------------------------------------------
//   SigEdit
//---------------------------------------------------------

class SigEdit : public QWidget
      {
      Q_OBJECT
      void init();

      QString sectionText(int sec);
      Section sec[2];

      bool adv;
      bool overwrite;
      int timerId;
      bool typing;
      bool changed;
      SigEditor *ed;
      SpinBox* controls;

   private slots:
      void stepUp();
      void stepDown();

   signals:
      void valueChanged(int, int);
      void returnPressed();

   protected:
      bool event(QEvent *e );
      void timerEvent(QTimerEvent* e);
      void resizeEvent(QResizeEvent*);
      QString sectionFormattedText(int sec);
      void addNumber(int sec, int num);
      void removeLastNumber(int sec);
      bool setFocusSection(int s);

      virtual bool outOfRange(int, int) const;
      virtual void setSec(int, int);
      friend class SigEditor;

   protected slots:
      void updateButtons();

   public slots:
      virtual void setValue(const Sig& sig);
      void setValue(const QString& s);

   public:
      SigEdit(QWidget*,  const char* = 0);
      ~SigEdit();

      QSize sizeHint() const;
      Sig sig() const;
      virtual void setAutoAdvance(bool advance) { adv = advance; }
      bool autoAdvance() const                  { return adv; }
      void enterPressed();
      };

#endif
