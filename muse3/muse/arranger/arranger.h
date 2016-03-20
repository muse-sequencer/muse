//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.h,v 1.17.2.15 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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
#include <QString>

#include "type_defs.h"
#include "midieditor.h"
#include "pcanvas.h"
#include "trackautomationview.h"

class QAction;
class QCheckBox;
class QMainWindow;
class QMenu;
class QToolButton;
class QWheelEvent;
class QKeyEvent;
class QPoint;
class QComboBox;
class QScrollBar;
class QVBoxLayout;
class QHBoxLayout;
//class QSpacerItem;

namespace MusECore {
class Track;
class Xml;
}

namespace MusEGui {
class ArrangerView;
class AudioStrip;
class Header;
class LabelCombo;
class MTScale;
class MidiTrackInfo;
class PosLabel;
class ScrollScale;
class SpinBox;
class Splitter;
//class TLLayout;
class TrackInfoLayout;
class TList;
class WidgetStack;
class ScrollBar;
class ArrangerCanvasLayout;

//---------------------------------------------------------
//   Arranger
//---------------------------------------------------------

class Arranger : public QWidget {
      Q_OBJECT

      static QByteArray header_state;

      ArrangerView* _parentWin;
      QWidget* editor;
      int _quant, _raster;
      QComboBox* _rasterCombo;
      PartCanvas* canvas;
      ScrollScale* hscroll;
      QScrollBar* vscroll;
      QVBoxLayout* tlistLayout;
      ArrangerCanvasLayout* egrid;
      QHBoxLayout* bottomHLayout;
      TList* list;
      Header* header;
      //QSpacerItem* tlistSpacerItem;
      MTScale* time;
      SpinBox* lenEntry;
      bool showTrackinfoFlag;
      bool showTrackinfoAltFlag;
      QWidget* trackInfoWidget;
      WidgetStack* trackInfo;
      ScrollBar* infoScroll;
      MidiTrackInfo* midiTrackInfo;
      AudioStrip* waveTrackInfo;
      QWidget* noTrackInfo;
      QWidget* tracklist;
//       TLLayout* tgrid;
      TrackInfoLayout* tgrid;

      MusECore::Track* selected;

      QToolButton* trackInfoButton;
      QToolButton* trackInfoAltButton;
      int trackInfoType;
      Splitter* split;
      int songType;
      PosLabel* cursorPos;
      SpinBox* globalTempoSpinBox;
      SpinBox* globalPitchSpinBox;
      
      unsigned cursVal;
      void genTrackInfo(QWidget* parent);
      void genMidiTrackInfo();
      void genWaveTrackInfo();
      void switchInfo(int);
      void setHeaderToolTips();
      void setHeaderWhatsThis();

   private slots:
      void rasterChanged(int);
      void songlenChanged(int);
      void showTrackInfo(bool);
      void showTrackInfoAlt(bool);
      void trackSelectionChanged();
      void trackInfoScroll(int);
      void songChanged(MusECore::SongChangedFlags_t);
      void setTime(unsigned);
      void globalPitchChanged(int);
      void globalTempoChanged(int);
      void setTempo50();
      void setTempo100();
      void setTempo200();
      void verticalScrollSetYpos(unsigned);
      void horizontalZoom(bool zoom_in, const QPoint& glob_pos);
      void horizontalZoom(int mag, const QPoint& glob_pos);
      
   signals:
      void editPart(MusECore::Track*);
      void selectionChanged(); // NOTE: This is emitted upon EITHER a part or track selection change.
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
      void updateTrackInfo(MusECore::SongChangedFlags_t flags);
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
      static std::vector<custom_col_t> custom_columns;     //FINDMICH TODO: eliminate all usage of new_custom_columns
      static std::vector<custom_col_t> new_custom_columns; //and instead let the arranger update without restarting muse!

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
      bool isSingleSelection() const { return canvas->isSingleSelection(); }
      int selectionSize() const { return canvas->selectionSize(); }
      bool itemsAreSelected() const { return canvas->itemsAreSelected(); }
      void setGlobalTempo(int);
      void clear();
      void songIsClearing() { canvas->songIsClearing(); }
      void setDefaultSplitterSizes();
      
      unsigned cursorValue() { return cursVal; }
      
      ArrangerView* parentWin() const { return _parentWin; }

      bool setRasterVal(int);
      };

} // namespace MusEGui

#endif

