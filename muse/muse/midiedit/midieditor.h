//=============================================================================
//  MusE
//  Linux Music Editor
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

#ifndef __MIDIEDITOR_H__
#define __MIDIEDITOR_H__

#include "editor.h"

namespace AL {
      class Xml;
      class Pos;
      };
using AL::Xml;

class EditToolBar;
class NoteInfo;
class Toolbar1;
class Part;
class PartList;
class EventCanvas;
class ScrollScale;
class MTScale;
class WaveView;

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

class MidiEditor : public Editor  {
      Q_OBJECT

      Q_PROPERTY(int raster       READ raster     WRITE setRaster)
      Q_PROPERTY(int quant        READ quant      WRITE setQuant)
      Q_PROPERTY(bool stepRec     READ stepRec    WRITE setStepRec)
      Q_PROPERTY(bool midiIn      READ midiIn     WRITE setMidiIn)
      Q_PROPERTY(bool playEvents  READ playEvents WRITE setPlayEvents)
      Q_PROPERTY(bool followSong  READ followSong WRITE setFollowSong)
      Q_PROPERTY(double xmag      READ xmag       WRITE setXmag)
      Q_PROPERTY(int applyTo      READ applyTo    WRITE setApplyTo)
      Q_PROPERTY(QPoint canvasPos READ canvasPos  WRITE setCanvasPos)
      Q_PROPERTY(int tool         READ tool       WRITE setTool)

      bool _playEvents;

      EventCanvas* canvas() { return (EventCanvas*)tcanvas; }
      const EventCanvas* canvas() const { return (EventCanvas*)tcanvas; }
      void copy();

   protected:
      int _raster;
      int _quant;
      int _applyTo;

      PartList* _pl;
      Part* selPart;

      QMenu *menuEdit;
      QAction* speaker;
      QAction* stepRecAction;
      QAction* midiInAction;
      QAction* followSongAction;
      QAction* cutAction;
      QAction* copyAction;
      QAction* pasteAction;

      EditToolBar* tools2;
      NoteInfo* info;
      QToolBar* tools;
      Toolbar1* toolbar;

      void writePartList(Xml&) const;
      void genPartlist();
      void writeStatus(Xml&) const;
	void initFromPart();

   private slots:
      void midiCmd(QAction*);

   protected slots:
      void clipboardChanged(); // enable/disable "Paste"
      void selectionChanged(); // enable/disable "Copy" & "Paste"
      virtual void songChanged(int); //add virtual to allow editors that do not use
                                     //ecancav to use there own songChanged slot
                                     //and avoid crashing, like MidiTrackerEditor
      void setPos(int, const AL::Pos&);
      virtual void cmd(QAction*) = 0;

   public slots:
      void setQuant(int val);
      void setApplyTo(int val);
      void setRaster(int val);

   public:
      MidiEditor(PartList*);
      virtual ~MidiEditor();

      void read(QDomNode);
      void write(Xml&) const;
      void readStatus(QDomNode);

      PartList* parts()            { return _pl;  }
      int rasterStep(unsigned tick) const;
      unsigned rasterVal(unsigned v)  const;
      unsigned rasterVal1(unsigned v) const;
      unsigned rasterVal2(unsigned v) const;
      int quantVal(int v) const;
      int raster() const           { return _raster; }
      int quant() const            { return _quant;  }
      int applyTo() const          { return _applyTo; }
      bool playEvents() const      { return speaker->isChecked();       }
      void setPlayEvents(bool val) { speaker->setChecked(val);          }
      bool stepRec() const         { return stepRecAction->isChecked(); }
      void setStepRec(bool val)    { stepRecAction->setChecked(val);    }
      bool midiIn() const          { return midiInAction->isChecked();  }
      void setMidiIn(bool val)     { midiInAction->setChecked(val);     }
      bool followSong() const      { return followSongAction->isChecked(); }
      void setFollowSong(bool val) { followSongAction->setChecked(val); }
      double xmag() const;
      void setXmag(double val);
      QPoint canvasPos() const;
      void setCanvasPos(const QPoint&);
      void setTool(int);
      int tool() const;
      enum {
         CMD_CUT, CMD_COPY, CMD_PASTE
         };
      };

#endif

