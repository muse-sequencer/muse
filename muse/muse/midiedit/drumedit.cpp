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

#include "drumedit.h"
#include "icons.h"
#include "drummap.h"
#include "audio.h"
#include "shortcuts.h"
#include "part.h"
#include "muse.h"
#include "song.h"

static const int drumeditTools = PointerTool | PencilTool | RubberTool | DrawTool;

int DrumEdit::initWidth    = DrumEdit::INIT_WIDTH;
int DrumEdit::initHeight   = DrumEdit::INIT_HEIGHT;
int DrumEdit::initRaster   = DrumEdit::INIT_RASTER;
int DrumEdit::initQuant    = DrumEdit::INIT_QUANT;
bool DrumEdit::initFollow  = DrumEdit::INIT_FOLLOW;
bool DrumEdit::initSpeaker = DrumEdit::INIT_SPEAKER;
bool DrumEdit::initMidiin  = DrumEdit::INIT_MIDIIN;
double DrumEdit::initXmag  = 0.08;	// DrumEdit::INIT_XMAG;
int DrumEdit::initApplyTo  = DrumEdit::INIT_APPLY_TO;

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

DrumEdit::DrumEdit(PartList* pl, bool init)
   : MidiEditor(pl)
      {
      _applyTo = initApplyTo;

      deltaMode   = false;
      drumMap    = &noDrumMap;

      //---------Pulldown Menu----------------------------
      QMenuBar* mb = menuBar();

      menuEdit->addSeparator();
      menuEdit->addAction(getAction("delete", this));

      // Functions
      menuFunctions = mb->addMenu(tr("&Functions"));
      menuFunctions->addAction(getAction("midi_fixed_len", this));
      menuFunctions->addAction(getAction("midi_mod_velo", this));

      menuSelect = menuEdit->addMenu(tr("&Select"));
      menuSelect->setIcon(QIcon(*selectIcon));

      menuSelect->addAction(getAction("sel_all", this));
      menuSelect->addAction(getAction("sel_none", this));
      menuSelect->addAction(getAction("sel_inv", this));
      menuSelect->addSeparator();
      menuSelect->addAction(getAction("sel_ins_loc", this));
      menuSelect->addAction(getAction("sel_out_loc", this));

      connect(menuSelect,    SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuFunctions, SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));

      //---------------------------------------------------
      //    Toolbars
      //---------------------------------------------------

      tools = addToolBar(tr("Drum Tools"));
      tools->addAction(undoAction);
      tools->addAction(redoAction);
      tools->addSeparator();

      tools->addAction(stepRecAction);
      stepRecAction->setChecked(INIT_SREC);

      tools->addAction(midiInAction);
      midiInAction->setChecked(INIT_MIDIIN);

      tools->addAction(speaker);
      speaker->setChecked(INIT_SPEAKER);

      tools->addAction(followSongAction);
      followSongAction->setChecked(INIT_FOLLOW);

      tools2 = new EditToolBar(this, drumeditTools);
      addToolBar(tools2);

      QToolBar* transport = addToolBar(tr("Transport"));
      muse->setupTransportToolbar(transport);

      addToolBarBreak();
      toolbar = new Toolbar1(initRaster, initQuant, false);
      addToolBar(toolbar);
      addToolBarBreak();
      info = new NoteInfo(this);
      addToolBar(info);

      tcanvas = new DrumCanvas(this);
      setCentralWidget(tcanvas);
      tcanvas->setCornerWidget(new QSizeGrip(tcanvas));
      tcanvas->setFollow(INIT_FOLLOW);

      connect(song,   SIGNAL(posChanged(int,const AL::Pos&,bool)), canvas(), SLOT(setLocatorPos(int,const AL::Pos&,bool)));
      connect(canvas(), SIGNAL(posChanged(int,const AL::Pos&)), SLOT(setPos(int,const AL::Pos&)));

      connect(canvas(), SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(tools2, SIGNAL(toolChanged(int)), canvas(), SLOT(setTool(int)));

      connect(canvas(), SIGNAL(selectionChanged(int, Event&, Part*)),
         SLOT(setSelection(int, Event&, Part*)));

      setWindowTitle(canvas()->getCaption());

      Pos p1(0, AL::TICKS), p2(0, AL::TICKS);
      canvas()->range(p1, p2);
      p2 += AL::sigmap.ticksMeasure(p2.tick());  // show one more measure
      canvas()->setTimeRange(p1, p2);

      // connect toolbar
      connect(canvas(), SIGNAL(cursorPos(const AL::Pos&,bool)), toolbar, SLOT(setTime(const AL::Pos&,bool)));
      connect(toolbar,  SIGNAL(quantChanged(int)),  SLOT(setQuant(int)));
      connect(toolbar,  SIGNAL(rasterChanged(int)), SLOT(setRaster(int)));
      connect(toolbar,  SIGNAL(soloChanged(bool)),  SLOT(soloChanged(bool)));
      connect(toolbar,  SIGNAL(toChanged(int)),     SLOT(setApplyTo(int)));

      connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)), SLOT(noteinfoChanged(NoteInfo::ValType, int)));

      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"

      canvas()->selectFirst();

      //
      // install misc shortcuts
      //
      QShortcut* sc;
      sc = new QShortcut(Qt::Key_Escape, this);
      sc->setContext(Qt::WindowShortcut);
      connect(sc, SIGNAL(activated()), SLOT(close()));

      QSignalMapper* cmdMap = new QSignalMapper(this);
      static const char* actions[] = { 
            "curpos_increase", "curpos_decrease",
            "pointer", "pencil", "eraser",
            "midi_quant_1", "midi_quant_2", "midi_quant_3", "midi_quant_4",
            "midi_quant_5", "midi_quant_6", "midi_quant_7", 
            "midi_quant_triol", "midi_quant_punct", "midi_quant_punct2"
            };
      for (unsigned i = 0; i < sizeof(actions)/sizeof(*actions); ++i) {
            QAction* a = getAction(actions[i], this);
            addAction(a);
            cmdMap->setMapping(a, a);
            connect(a, SIGNAL(triggered()), cmdMap, SLOT(map()));
            }
      connect(cmdMap, SIGNAL(mapped(QObject*)), SLOT(drumCmd(QObject*)));


      connect(song, SIGNAL(songChanged(int)), canvas(), SLOT(songChanged(int)));
      connect(followSongAction, SIGNAL(toggled(bool)), canvas(), SLOT(setFollow(bool)));
      canvas()->selectFirst();

      Part* part = canvas()->part();

	setRaster(part->raster() != -1   ? part->raster() : initRaster);
      setQuant(part->quant()   != -1   ? part->quant()  : initQuant);
      setXmag(part->xmag()     != -1.0 ? part->xmag()   : initXmag);

      if (init)
            initFromPart();
      else
	      resize(initWidth, initHeight);
      }

//---------------------------------------------------------
//   ~DrumEdit
//---------------------------------------------------------

DrumEdit::~DrumEdit()
      {
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void DrumEdit::closeEvent(QCloseEvent* e)
      {
      MidiEditor::closeEvent(e);
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void DrumEdit::setSelection(int tick, Event& e, Part* p)
      {
      int selections = canvas()->selectionSize();
      selEvent = e;
      selPart  = p;

      if (selections > 1) {
            info->setEnabled(true);
            info->setDeltaMode(true);
            if (!deltaMode) {
                  deltaMode = true;
                  info->setValues(0, 0, 0, 0, 0);
                  tickOffset    = 0;
                  lenOffset     = 0;
                  pitchOffset   = 0;
                  veloOnOffset  = 0;
                  veloOffOffset = 0;
                  }
            }
      else if (selections == 1) {
            deltaMode = false;
            info->setEnabled(true);
            info->setDeltaMode(false);
            info->setValues(tick,
               selEvent.lenTick(),
               selEvent.pitch(),
               selEvent.velo(),
               selEvent.veloOff());
            }
      else {
            deltaMode = false;
            info->setEnabled(false);
            }
      selectionChanged();
      }

//---------------------------------------------------------
//   soloChanged
//---------------------------------------------------------

void DrumEdit::soloChanged(bool flag)
      {
      song->setSolo(canvas()->track(), flag);
      }

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void DrumEdit::noteinfoChanged(NoteInfo::ValType type, int val)
      {
      int selections = canvas()->selectionSize();

      if (selections == 0) {
            printf("noteinfoChanged while nothing selected\n");
            }
      else if (selections == 1) {
            Event event = selEvent.clone();
            switch(type) {
                  case NoteInfo::VAL_TIME:
                        event.setTick(val - selPart->tick());
                        break;
                  case NoteInfo::VAL_LEN:
                        event.setLenTick(val);
                        break;
                  case NoteInfo::VAL_VELON:
                        event.setVelo(val);
                        break;
                  case NoteInfo::VAL_VELOFF:
                        event.setVeloOff(val);
                        break;
                  case NoteInfo::VAL_PITCH:
                        event.setPitch(val);
                        break;
                  }
            audio->msgChangeEvent(selEvent, event, selPart);
            }
      else {
            // multiple events are selected; treat noteinfo values
            // as offsets to event values

            int delta = 0;
            switch (type) {
                  case NoteInfo::VAL_TIME:
                        delta = val - tickOffset;
                        tickOffset = val;
                        break;
                  case NoteInfo::VAL_LEN:
                        delta = val - lenOffset;
                        lenOffset = val;
                        break;
                  case NoteInfo::VAL_VELON:
                        delta = val - veloOnOffset;
                        veloOnOffset = val;
                        break;
                  case NoteInfo::VAL_VELOFF:
                        delta = val - veloOffOffset;
                        veloOffOffset = val;
                        break;
                  case NoteInfo::VAL_PITCH:
                        delta = val - pitchOffset;
                        pitchOffset = val;
                        break;
                  }
            if (delta)
                  canvas()->modifySelected(type, delta);
            }
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void DrumEdit::cmd(QAction* a)
      {
      canvas()->cmd(a);
      }

//---------------------------------------------------------
//   drumCmd
//---------------------------------------------------------

void DrumEdit::drumCmd(QObject* object)
      {
      QAction* a = (QAction*)object;
      QString cmd(a->data().toString());

      static const int rasterTable[] = {
            //-9----8-  7    6     5     4    3(1/4)     2   1
            4,  8, 16, 32,  64, 128, 256,  512, 1024,  // triple
            6, 12, 24, 48,  96, 192, 384,  768, 1536,
            9, 18, 36, 72, 144, 288, 576, 1152, 2304   // dot
            };

      DrumCanvas* dc = canvas();
      int index = 0;
      int n = sizeof(rasterTable);
      for (; index < n; ++index)
            if (rasterTable[index] == raster())
                  break;
      int off = (index / 9) * 9;
      index   = index % 9;
      int val;

      if (cmd == "curpos_increase") {
            dc->cmd(a);
            return;
            }
      else if (cmd == "curpos_decrease") {
            dc->cmd(a);
            return;
            }
      else if (cmd == "pointer") {
            tools2->set(PointerTool);
            return;
            }
      else if (cmd == "pencil") {
            tools2->set(PencilTool);
            return;
            }
      else if (cmd == "eraser") {
            tools2->set(RubberTool);
            return;
            }
      else if (cmd == "midi_quant_1")
            val = rasterTable[8 + off];
      else if (cmd == "midi_quant_2")
            val = rasterTable[7 + off];
      else if (cmd == "midi_quant_3")
            val = rasterTable[6 + off];
      else if (cmd == "midi_quant_4")
            val = rasterTable[5 + off];
      else if (cmd == "midi_quant_5")
            val = rasterTable[4 + off];
      else if (cmd == "midi_quant_6")
            val = rasterTable[3 + off];
      else if (cmd == "midi_quant_7")
            val = rasterTable[2 + off];
      else if (cmd == "midi_quant_triol")
            val = rasterTable[index + ((off == 0) ? 9 : 0)];
      else if (cmd == "midi_quant_punct")
            val = rasterTable[index + ((off == 18) ? 9 : 18)];

      else if (cmd == "midi_quant_punct2") {
            if ((off == 18) && (index > 2)) {
                  val = rasterTable[index + 9 - 1];
                  }
            else if ((off == 9) && (index < 8)) {
                  val = rasterTable[index + 18 + 1];
                  }
            else
                  return;
            }
      else {
            printf("DrumEdit::drumCmd: unknown cmd <%s>\n", cmd.toLatin1().data());
            return;
            }
      setQuant(val);
      setRaster(val);
      toolbar->setQuant(quant());
      toolbar->setRaster(raster());
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void DrumEdit::writeConfiguration(Xml& xml)
      {
      xml.stag("DrumEdit");
      if (DrumEdit::initWidth != DrumEdit::INIT_WIDTH)
            xml.tag("width", DrumEdit::initWidth);
      if (DrumEdit::initHeight != DrumEdit::INIT_HEIGHT)
            xml.tag("height", DrumEdit::initHeight);
      if (DrumEdit::initRaster != DrumEdit::INIT_RASTER)
            xml.tag("raster", DrumEdit::initRaster);
      xml.etag("DrumEdit");
	}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void DrumEdit::readConfiguration(QDomNode node)
      {
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            int i = e.text().toInt();
            if (tag == "width")
                  DrumEdit::initWidth = i;
            else if (tag == "height")
                  DrumEdit::initHeight = i;
            else if (tag == "raster")
                  DrumEdit::initRaster = i;
            else
                  printf("MusE:DrumEdit: unknown tag %s\n", tag.toLatin1().data());
            }
      }

