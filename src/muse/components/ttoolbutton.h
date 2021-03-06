//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbutton.h,v 1.1 2004/02/21 16:53:51 wschweer Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __TTOOLBUTTON_H__
#define __TTOOLBUTTON_H__

#include <QToolButton>
#include <QIcon>
#include <QPaintEvent>

namespace MusEGui {

//---------------------------------------------------------
//   TransparentToolButton
//---------------------------------------------------------

class TransparentToolButton : public QToolButton {
      Q_OBJECT

      virtual void drawButton(QPainter*);

   public:
      TransparentToolButton(QWidget* parent, const char* name = 0)
         : QToolButton(parent) {setObjectName(name);}
      };

//---------------------------------------------------------
//   CompactToolButton
//   Supports two icons.      
//---------------------------------------------------------

class CompactToolButton : public QToolButton {
      Q_OBJECT
   
   private:
     QIcon _icon;
     bool _scaleDownIcon;
     bool _hasFixedIconSize;
     bool _drawFlat;
     bool _blinkPhase;

   protected:
     virtual void paintEvent(QPaintEvent*);

   public:
      CompactToolButton(
        QWidget* parent = nullptr, const QIcon& icon = QIcon(),
        bool hasFixedIconSize = true, bool drawFlat = false, const char* name = nullptr);
         
      bool hasFixedIconSize() const { return _hasFixedIconSize; }
      void setHasFixedIconSize(bool v);

      bool drawFlat() const { return _drawFlat; }
      void setDrawFlat(bool v);

      bool scaleDownIcon() const { return _scaleDownIcon; }
      void setScaleDownIcon(bool v) { _scaleDownIcon = v; }

      // If _hasFixedIconSize is true, this relies on iconSize(). Be sure to set iconSize to the desired value.
      virtual QSize sizeHint() const;
      
      void setIcon(const QIcon & icon);
      bool blinkPhase() const { return _blinkPhase; }
      void setBlinkPhase(bool v);
      };

} // namespace MusEGui

#endif

