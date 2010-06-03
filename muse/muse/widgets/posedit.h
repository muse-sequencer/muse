//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: posedit.h,v 1.1.1.1.2.1 2004/12/27 19:47:25 lunar_shuttle Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __POSEDIT_H__
#define __POSEDIT_H__

#include <qwidget.h>
#include <qstring.h>
#include "pos.h"

class PosEditor;
class QSpinWidget;

#include "section.h"

//---------------------------------------------------------
//   PosEdit
//---------------------------------------------------------

class PosEdit : public QWidget
      {
      Q_OBJECT
      Q_PROPERTY(QString separator READ separator WRITE setSeparator)
      Q_PROPERTY(bool smpte READ smpte WRITE setSmpte)

      void init();
      void setSections();
      QString sectionText(int sec);
      Section midiSections[3];
      Section smpteSections[4];
      Section* sec;

      bool _smpte;

      bool adv;
      bool overwrite;
      int timerId;
      bool typing;
      Pos min;
      Pos max;
      bool changed;
      PosEditor *ed;
      QSpinWidget* controls;

   private slots:
      void stepUp();
      void stepDown();

   signals:
      void valueChanged(const Pos&);
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
      friend class PosEditor;

   protected slots:
      void updateButtons();

   public slots:
      virtual void setValue(const Pos& time);
      void setValue(int t);
      void setValue(const QString& s);
      // Added p3.3.43
      virtual void setEnabled(bool);

   public:
      PosEdit(QWidget*,  const char* = 0);
      PosEdit(const Pos& time, QWidget*,  const char* = 0);
      ~PosEdit();

      QSize sizeHint() const;
      Pos pos() const;
      virtual void setAutoAdvance(bool advance) { adv = advance; }
      bool autoAdvance() const                  { return adv; }

      virtual void setMinValue(const Pos& d)    { setRange(d, maxValue()); }
      Pos minValue() const;
      virtual void setMaxValue( const Pos& d )  { setRange(minValue(), d ); }
      Pos maxValue() const;
      virtual void setRange(const Pos& min, const Pos& max);
      QString separator() const;
      virtual void setSeparator(const QString& s);
      void setSmpte(bool);
      bool smpte() const;
      void enterPressed();
      };

#endif
