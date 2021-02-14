//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: posedit.h,v 1.1.1.1.2.1 2004/12/27 19:47:25 lunar_shuttle Exp $
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

#ifndef __POSEDIT_H__
#define __POSEDIT_H__

#include <QWidget>

#include "pos.h"
#include "section.h"

class QResizeEvent;
class QTimerEvent;

class PosEditor;
class SpinBox;

namespace MusEGui {

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
      SpinBox* controls;

   private slots:
      void stepUp();
      void stepDown();

   signals:
      void valueChanged(const Pos&);
      void returnPressed();

   protected:
      virtual bool event(QEvent *e );
      void timerEvent(QTimerEvent* e);
      virtual void resizeEvent(QResizeEvent*);
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
      PosEdit(QWidget* = 0,  const char* = 0);
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

} // namespace MusEGui

#endif
