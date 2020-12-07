//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.h,v 1.17.2.15 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#ifndef __ARRANGER_H__
#define __ARRANGER_H__

#include <vector>
#include <QWidget>
#include <QString>

#include "type_defs.h"


// Forward declarations:
class QKeyEvent;
class QPoint;
class QComboBox;
class QScrollBar;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QGridLayout;
class QToolButton;

namespace MusECore {
class Track;
class Xml;
class PartList;
}

namespace MusEGui {
class ArrangerView;
class Header;
class LabelCombo;
class MTScale;
class PosLabel;
class ScrollScale;
class SpinBox;
class Splitter;
class TrackInfoWidget;
class TList;
class ArrangerCanvasLayout;
class ArrangerHScrollLayout;
class PartCanvas;
class CompactToolButton;
class RasterizerModel;
class RasterLabelCombo;

//---------------------------------------------------------
//   Arranger
//---------------------------------------------------------

class Arranger : public QWidget {
      Q_OBJECT

      static QByteArray header_state;

      ArrangerView* _parentWin;
      QWidget* editor;
      int _quant, _raster;
      RasterizerModel *_rasterizerModel;
      RasterLabelCombo* _rasterCombo;
      PartCanvas* canvas;
      ScrollScale* hscroll;
      QScrollBar* vscroll;
      QVBoxLayout* tlistLayout;
      QGridLayout* egrid;
      QHBoxLayout* bottomHLayout;
      TList* list;
      Header* header;
      MTScale* time;
      SpinBox* lenEntry;
      QToolButton* gridOnButton;
      bool showTrackinfoFlag;
      TrackInfoWidget* trackInfoWidget;
      QScrollArea* tracklistScroll;
      QWidget* tracklist;
      // The X origin that is applied to any canvases.
      int _canvasXOrigin;
      // The X mag minimum that is applied to any ScrollScale controls.
      // Note that global midi division will also be taken into account.
      int _minXMag;
      // The X mag maximum that is applied to any ScrollScale controls.
      int _maxXMag;

      MusECore::Track* selected;

      Splitter* split;
      PosLabel* cursorPos;
      SpinBox* globalTempoSpinBox;
      SpinBox* globalPitchSpinBox;
      
      unsigned cursVal;
      void genTrackInfo(TrackInfoWidget*);
      void genMidiTrackInfo();
      void genWaveTrackInfo();
      void switchInfo(int);
      void trackInfoSongChange(MusECore::SongChangedStruct_t flags);
      void setHeaderToolTips();
      void setHeaderWhatsThis();
      void setHeaderStatusTips();
      void setHeaderSizes();
      void initTracklistHeader();

      // Sets up a reasonable zoom minimum and/or maximum based on
      //  the current global midi division (ticks per quarter note)
      //  which has a very wide range (48 - 12288).
      // Also sets the canvas and time scale offsets accordingly.
      void setupHZoomRange();

   private slots:
      void rasterChanged(int raster);
      void songlenChanged(int);
      void showTrackInfo(bool);
      void trackSelectionChanged();
      void songChanged(MusECore::SongChangedStruct_t);
      void setTime(unsigned);
      void globalPitchChanged(int);
      void globalTempoChanged(int);
      void setTempo50();
      void setTempo100();
      void setTempo200();
      void gridOnChanged(bool v);
      void verticalScrollSetYpos(unsigned);
      void horizontalZoom(bool zoom_in, const QPoint& glob_pos);
      void horizontalZoom(int mag, const QPoint& glob_pos);
      void updateTracklist();
      
   signals:
      void editPart(MusECore::Track*);
      void dropSongFile(const QString&);
      void dropMidiFile(const QString&);
      void startEditor(MusECore::PartList*, int);
      void toolChanged(int);
      void setUsedTool(int);


   protected:
      virtual void keyPressEvent(QKeyEvent* event);

   public slots:
      void dclickPart(MusECore::Track*);
      void setTool(int);
      void updateTrackInfo(MusECore::SongChangedStruct_t flags);
      void configChanged();
      void controllerChanged(MusECore::Track *t, int ctrlId);
      void focusCanvas();

   public:
      enum { CMD_CUT_PART, CMD_COPY_PART, CMD_COPY_PART_IN_RANGE, CMD_PASTE_PART, CMD_PASTE_CLONE_PART,
             CMD_PASTE_PART_TO_TRACK, CMD_PASTE_CLONE_PART_TO_TRACK, CMD_PASTE_DIALOG, CMD_INSERT_EMPTYMEAS };
      
      struct custom_col_t
      {
        enum affected_pos_t {AFFECT_BEGIN, AFFECT_CPOS};

        int ctrl;
        QString name;
        affected_pos_t affected_pos;
        
        custom_col_t(int c, QString n, affected_pos_t a=AFFECT_BEGIN)
        {
          ctrl=c;
          name=n;
          affected_pos=a;
        }
      };
      static std::vector<custom_col_t> custom_columns;

      Arranger(ArrangerView* parent, const char* name = 0);

      PartCanvas* getCanvas() { return canvas; }
      void reset();
      
      void writeStatus(int level, MusECore::Xml&);
      void readStatus(MusECore::Xml&);
      void writeConfiguration(int level, MusECore::Xml&);
      static void readConfiguration(MusECore::Xml&);
      static void writeCustomColumns(int level, MusECore::Xml&);
      static void readCustomColumns(MusECore::Xml&);
      static custom_col_t readOneCustomColumn(MusECore::Xml&);

      MusECore::Track* curTrack() const { return selected; }
      void cmd(int);
      bool isSingleSelection() const;
      int selectionSize() const;
      bool itemsAreSelected() const;
      void setGlobalTempo(int);
      void clear();
      void songIsClearing() const;
      void setDefaultSplitterSizes();
      void updateHeaderCustomColumns();
      void toggleTrackInfo();
      void storeSplitterSizes();
      
      unsigned cursorValue() { return cursVal; }
      
      ArrangerView* parentWin() const { return _parentWin; }

      int rasterVal() const;

      bool setRasterVal(int);

      TList *getTrackList() { return list; }
      };

} // namespace MusEGui

#endif

