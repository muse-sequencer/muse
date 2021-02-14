//=========================================================
//  MusE
//  Linux Music Editor
//    scrollbar.h
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __SCROLLBAR_H__
#define __SCROLLBAR_H__

#include <QScrollBar>

class QWheelEvent;
class QResizeEvent;

namespace MusEGui {

//---------------------------------------------------------
//   ScrollBar
//---------------------------------------------------------

class ScrollBar : public QScrollBar {
      Q_OBJECT
    
  private:
      bool _autoPageStep;
      
  public slots:
      void redirectedWheelEvent(QWheelEvent*);

  protected:
      virtual void resizeEvent(QResizeEvent*);
      
  public:    
    ScrollBar(Qt::Orientation orientation, bool autoPageStep = false, QWidget* parent = 0);
    
    bool autoPageStep() const { return _autoPageStep; }
    void setAutoPageStep(bool v) { _autoPageStep = v; update(); }
};

} // namespace MusEGui

#endif
