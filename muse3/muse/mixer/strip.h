//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.h,v 1.3.2.2 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __STRIP_H__
#define __STRIP_H__

#include <QFrame>
//#include <QIcon>
//#include <QVBoxLayout>
//#include <QGridLayout>
#include <QLabel>

#include "type_defs.h"
#include "globaldefs.h"
//#include "route.h"

class QMouseEvent;
class QResizeEvent;
class QLabel;
//class QVBoxLayout;
class QToolButton;
class QGridLayout;
class QLayout;
class QSize;

namespace MusECore {
class Track;
}

namespace MusEGui {
class ComboBox;
class Meter;

static const int STRIP_WIDTH = 65;
static const int AUDIO_STRIP_WIDTH = 73;

struct GridPosStruct
{
  int _row;
  int _col;
  int _rowSpan;
  int _colSpan;
  
  GridPosStruct() : _row(0), _col(0), _rowSpan(0), _colSpan(0) { }
  GridPosStruct(int row, int col, int rowSpan, int colSpan)
              : _row(row), _col(col), _rowSpan(rowSpan), _colSpan(colSpan) { }
};

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame {
      Q_OBJECT

   QPoint mouseWidgetOffset;
   bool dragOn;
   bool _visible;
   protected:
      //enum ResizeMode { ResizeModeNone, ResizeModeHovering, ResizeModeDragging };
 
      //ResizeMode _resizeMode;
      
      MusECore::Track* track;
      QLabel* label;
      //QVBoxLayout* layout;
      QGridLayout* grid;
      int _curGridRow;
      MusEGui::Meter* meter[MAX_CHANNELS];
      // Extra width applied to the sizeHint, from user expanding the strip.
      int _userWidth;
      
      QToolButton* record;
      QToolButton* solo;
      QToolButton* mute;
      QToolButton* iR; // Input routing button
      QToolButton* oR; // Output routing button
      QGridLayout* sliderGrid;
      MusEGui::ComboBox* autoType;
      void setLabelText();
      virtual void resizeEvent(QResizeEvent*);
      virtual void mousePressEvent(QMouseEvent *);
      virtual void mouseReleaseEvent(QMouseEvent *);
      virtual void mouseMoveEvent(QMouseEvent *);

   private slots:
      void recordToggled(bool);
      void soloToggled(bool);
      void muteToggled(bool);

   protected slots:
      virtual void heartBeat();
      void setAutomationType(int t);

   public slots:
      void resetPeaks();
      virtual void songChanged(MusECore::SongChangedFlags_t) = 0;
      virtual void configChanged() = 0;
      virtual void changeUserWidth(int delta);

   public:
      Strip(QWidget* parent, MusECore::Track* t);
      ~Strip();

      bool getStripVisible() { return _visible; }
      void setStripVisible(bool v) { _visible = v; }

      void setRecordFlag(bool flag);
      MusECore::Track* getTrack() const { return track; }
      void setLabelFont();
      QString getLabelText() { return label->text(); }
      
      void addGridWidget(QWidget* w, const GridPosStruct& pos, Qt::Alignment alignment = 0);
      void addGridLayout(QLayout* l, const GridPosStruct& pos, Qt::Alignment alignment = 0);
      
      int userWidth() const { return _userWidth; }
      void setUserWidth(int w);
      
      virtual QSize sizeHint() const;
      };

//---------------------------------------------------------
//   ExpanderHandle
//---------------------------------------------------------

class ExpanderHandle : public QFrame 
{
  Q_OBJECT

  protected:
    enum ResizeMode { ResizeModeNone, ResizeModeHovering, ResizeModeDragging };
    
  private:
    int _handleWidth;
    ResizeMode _resizeMode;
    QPoint _dragLastGlobPos;
      
  protected:
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    //virtual void leaveEvent(QEvent*);
    virtual QSize sizeHint() const;

  signals:
    void moved(int xDelta);
    
  public:
    ExpanderHandle(QWidget * parent = 0, int handleWidth = 4, Qt::WindowFlags f = 0);
};

} // namespace MusEGui

#endif

