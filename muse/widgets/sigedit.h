//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sigedit.h,v 1.1.1.1.2.1 2004/12/28 23:23:51 lunar_shuttle Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
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

#ifndef __SIGEDIT_H__
#define __SIGEDIT_H__

#include <QWidget>

#include "section.h"

class QResizeEvent;
class QTimerEvent;

namespace MusEGui {

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
      virtual bool event(QEvent *e );
      void timerEvent(QTimerEvent* e);
      virtual void resizeEvent(QResizeEvent*);
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

} // namespace MusEGui

#endif
