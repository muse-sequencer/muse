//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __TCANVAS_H__
#define __TCANVAS_H__

#include "al/pos.h"
#include "al/marker.h"

#define MAP_OFFSET  20

enum TimeCanvasType {
      TIME_CANVAS, TIME_CANVAS_PIANOROLL, TIME_CANVAS_DRUMEDIT,
      TIME_CANVAS_WAVEEDIT
      };

enum Tool {
      PointerTool=1, PencilTool=2, RubberTool=4, CutTool=8,
      GlueTool=16, QuantTool=32, DrawTool=64, MuteTool=128
      };

enum FollowMode {
      FOLLOW_NO, FOLLOW_JUMP, FOLLOW_CONTINUOUS
      };

static const int rulerHeight = 28;
static const int pianoWidth  = 40;
static const int waveWidth   = 40;
static const int drumWidth   = 120;
static const int drumHeight  = 18;
static const int keyHeight   = 13;

//---------------------------------------------------------
//   TimeCanvas
//
//     Layout:
//      Button   Ruler
//      panelA   canvasA
//      panelB   canvasB
//
//     Pianoroll:
//       panelA -> keyboard
//
//     Drumeditor:
//       panelA -> instrument list
//
//---------------------------------------------------------

class TimeCanvas : public QFrame {
      Q_OBJECT

      int yRange;
      bool _yFit;
      AL::Pos pos1;     // time scroll range
      AL::Pos pos2;

      int metronomeRulerMag;

      double _xmagMin, _xmagMax;
      double _ymagMin, _ymagMax;

      AL::TType _timeType;
      AL::MarkerList* marker;
      QScrollBar* hbar;
      QScrollBar* vbar;
      QSlider* vmag;
      QSlider* hmag;
      QToolButton* timeTypeButton;
      QGridLayout* grid;
      QColor canvasBackgroundColor;
      QPixmap canvasBackgroundPixmap;
      int dragType;
      bool followPos;

      //
      // pianoroll variables
      //
      static QPixmap* octave;
      static QPixmap* mk1;
      static QPixmap* mk2;
      static QPixmap* mk3;
      static QPixmap* mk4;
      QPushButton* addCtrlButton;

      bool mouseInB;

      void updateScrollBars();
      void canvasPaintEvent(const QRect&, QPainter&);
      void paintCanvas(QPainter&, const QRect&);
      void paintMetronomRuler(QPainter&, const QRect&);
      void paintClockRuler(QPainter&, const QRect&);
      void initPianoroll();
      void paintPiano(QPainter&, QRect);
      void paintPianorollHorizontalGrid(QPainter&, QRect);
      void paintDrumeditHorizontalGrid(QPainter&, QRect);
      void updateGeometry();
      double s2xmag(int val);
      int xmag2s(double m);
	void updateRulerMag();

   protected:
      TimeCanvasType type;
      Tool _tool;
      QRect rButton, rPanelA, rPanelB, rRuler, rCanvasA, rCanvasB;
      AL::Pos pos[3];
      bool showCursor;
      AL::Pos cursor;

      QWidget* _widget;

      AL::Pos partPos1;	// active time range for midi editors
      AL::Pos partPos2;

      QPoint wpos;      // "widget" position
      double _xmag, _ymag;

      int ctrlHeight;
      int curPitch;

      int _raster;
      int button;
      Qt::KeyboardModifiers keyState;

      bool eventFilter(QObject*, QEvent*);

      virtual void paint(QPainter&, QRect) { printf("paint method not overloaded\n"); }
      virtual void mousePress(QMouseEvent*) {  printf("mousePress method not overloaded\n"); }
      virtual void mouseMove(QPoint)  {  printf("mouseMove method not overloaded\n"); }
      virtual void mouseRelease(QMouseEvent*) { printf("mouseRelease method not overloaded\n"); }
      virtual void mouseDoubleClick(QMouseEvent*) { printf("mouseDoubleClick method not overloaded\n"); }
      virtual void paintDrumList(QPainter&, QRect) {}
      virtual void layout() { printf("layout method not overloaded\n"); }
      virtual void enterB() { printf("enterB method not overloaded\n"); }
      virtual void leaveB() { printf("leaveB method not overloaded\n"); }
      
      virtual void setCursor();
      
      virtual void timeTypeChanged() { printf("timeTypeChanged method not overloaded\n");}
      virtual void magChanged() { printf("magChanged method not overloaded\n");}

      virtual void paintControllerCanvas(QPainter&, QRect) { printf("paintControllerCanvas method not overloaded\n"); }
      virtual void paintControllerPanel(QPainter&, QRect) { printf("paintControllerPanel method not overloaded\n"); }

      virtual void dragEnter(QDragEnterEvent*) { printf("dragEnter method not overloaded\n"); }
      virtual void drop(QDropEvent*) { printf("drop method not overloaded\n"); }
      virtual void dragMove(QDragMoveEvent*) { printf("dragMove method not overloaded\n"); }
      virtual void dragLeave(QDragLeaveEvent*) { printf("dragLeave method not overloaded\n"); }

      virtual void addController() { printf("addController method not overloaded\n"); }
      
      virtual void keyPressEvent(QKeyEvent *e);
      virtual void keyboardNavigate(QKeyEvent *) { printf("keyboardNavigate method not overloaded\n"); }

      // map logical coordinates to physical coordinates (pixel)
      int mapx(int x)  const;
      int mapy(int y)  const { return lrint(y * _ymag) - wpos.y(); }
      int rmapx(int x) const { return lrint(x * _xmag); }
      int rmapy(int y) const { return lrint(y * _ymag); }
      QPoint map(const QPoint& p) const {
            return QPoint(mapx(p.x()), mapy(p.y()));
            }
      QPoint rmap(const QPoint& p) const {
            return QPoint(rmapx(p.x()), rmapy(p.y()));
            }

      // map physical coordinates (pixel) to logical coordinates
      int mapxDev(int x) const;
      int rmapxDev(int x) const { return lrint(x / _xmag);  }
      int mapyDev(int y) const {
            int val = lrint((y + wpos.y()) / _ymag);
            return val < 0 ? 0 : val;
            }
      QPoint mapDev(const QPoint& p) const {
            return QPoint(mapxDev(p.x()), mapyDev(p.y()));
            }

      virtual int y2pitch(int y) const;
      virtual int pitch2y(int pitch) const;
      void setTimeType1(AL::TType t);
      void setPart(const AL::Pos& p1, const AL::Pos& p2);

   private slots:
      void moveX(int);
      void scaleX(int);
      void scaleY(int);
      void toggleTimeType();
      void addCtrlClicked();

   signals:
      void posChanged(int, const AL::Pos&);  // emitted from ruler
      void cursorPos(const AL::Pos&, bool);
      void contentsMoving(int, int);
      void addMarker(const AL::Pos&);
      void removeMarker(const AL::Pos&);
      void pitchChanged(int);
      void toolChanged(int);

   public slots:
      void setLocatorPos(int idx, const AL::Pos&, bool follow);
      void setVSize(int);
      virtual void setRaster(int);
      void setYPos(int);
      void moveY(int);
      void setEndPos(const AL::Pos&);
      void tempoChanged();
      void setTool(int);
      void setFollow(bool val) { followPos = val; }

   public:
      TimeCanvas(TimeCanvasType = TIME_CANVAS);
      void setTimeRange(const AL::Pos&, const AL::Pos&);
      void setMag(double, double);
      double xmag() const     { return _xmag; }
      double ymag() const     { return _ymag; }

      void setYMagRange(double, double);
      void setYFit(bool val)  { _yFit = val; }

      QPoint getWPos() const  { return wpos; }
      QWidget* widget() const { return _widget; }
      void setWPos(const QPoint& p);
      void setTimeType(AL::TType t);
      AL::TType timeType() const { return _timeType; }
      void setMarkerList(AL::MarkerList*);
      void setCornerWidget(QWidget* w);
      void resizeController(int);
      int raster() const { return _raster; }

      AL::Pos pix2pos(int x) const;
      int pos2pix(const AL::Pos&) const;
      QScrollBar* verticalScrollBar() const { return vbar; }
      Tool tool() const { return _tool; }
      void setCanvasBackground(const QColor& color);
      void setCanvasBackground(const QPixmap& pm);

      void updateCanvasB() { _widget->update(rCanvasB); }
      void updateRuler()   { _widget->update(rRuler);   }

      static FollowMode followMode;
      friend class CtrlEdit;
      };

#endif

