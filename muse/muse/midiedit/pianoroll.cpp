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

#include "pianoroll.h"
#include "song.h"
#include "icons.h"
#include "cmd.h"
#include "muse.h"
#include "widgets/tools.h"
#include "quantconfig.h"
#include "shortcuts.h"
#include "audio.h"
#include "part.h"

static int pianorollTools        = PointerTool | PencilTool | RubberTool | DrawTool;
int PianoRoll::initWidth         = PianoRoll::INIT_WIDTH;
int PianoRoll::initHeight        = PianoRoll::INIT_HEIGHT;
int PianoRoll::initRaster        = PianoRoll::INIT_RASTER;
int PianoRoll::initQuant         = PianoRoll::INIT_QUANT;
int PianoRoll::initColorMode     = PianoRoll::INIT_COLOR_MODE;
bool PianoRoll::initFollow       = PianoRoll::INIT_FOLLOW;
bool PianoRoll::initSpeaker      = PianoRoll::INIT_SPEAKER;
bool PianoRoll::initMidiin       = PianoRoll::INIT_MIDIIN;
double PianoRoll::initXmag       = 0.08;	// PianoRoll::INIT_XMAG; (compiler problem?)
int PianoRoll::initApplyTo       = PianoRoll::INIT_APPLY_TO;
int PianoRoll::initQuantStrength = PianoRoll::INIT_QUANT_STRENGTH;
int PianoRoll::initQuantLimit    = PianoRoll::INIT_QUANT_LIMIT;
bool PianoRoll::initQuantLen     = PianoRoll::INIT_QUANT_LEN;

//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

PianoRoll::PianoRoll(PartList* pl, bool init)
   : MidiEditor(pl)
      {
      _applyTo       = initApplyTo;
      _colorMode     = initColorMode;
	_quantStrength = initQuantStrength;
	_quantLimit    = initQuantLimit;
	_quantLen      = initQuantLen;

      deltaMode   = false;
      selPart     = 0;
      quantConfig = 0;

      tcanvas = new PianoCanvas(this);
      QMenuBar* mb = menuBar();

      menu_ids[CMD_EVENT_COLOR]  = getAction("change_event_color", this);
      menu_ids[CMD_CONFIG_QUANT] = getAction("config_quant", this);

      //---------Menu----------------------------------
      menuEdit->addSeparator();

      QAction* a = getAction("delete", this);
      menuEdit->addAction(a);

      menuEdit->addSeparator();

      menuSelect = menuEdit->addMenu(QIcon(*selectIcon),tr("&Select"));

      menuSelect->addAction(getAction("sel_all", this));
      menuSelect->addAction(getAction("sel_none", this));
      menuSelect->addAction(getAction("sel_inv", this));
      menuSelect->addAction(getAction("sel_ins_loc", this));
      menuSelect->addAction(getAction("sel_out_loc", this));

      menuConfig = mb->addMenu(tr("&Config"));
      eventColor = menuConfig->addMenu(tr("event color"));
      menu_ids[CMD_EVENT_COLOR] = eventColor->menuAction();

      colorModeAction[0] = eventColor->addAction(tr("blue"));
      colorModeAction[0]->setData(0);
      colorModeAction[0]->setCheckable(true);
      colorModeAction[1] = eventColor->addAction(tr("pitch colors"));
      colorModeAction[1]->setData(1);
      colorModeAction[1]->setCheckable(true);
      colorModeAction[2] = eventColor->addAction(tr("velocity colors"));
      colorModeAction[2]->setData(2);
      colorModeAction[2]->setCheckable(true);
      connect(eventColor, SIGNAL(triggered(QAction*)), SLOT(setEventColorMode(QAction*)));

      menuFunctions = mb->addMenu(tr("&Functions"));

      menuFunctions->addAction(getAction("midi_over_quant", this));
      menuFunctions->addAction(getAction("midi_quant_noteon", this));
      menuFunctions->addAction(getAction("midi_quant_noteoff", this));
      menuFunctions->addAction(getAction("midi_quant_iterative", this));

      menuFunctions->addSeparator();

      menu_ids[CMD_CONFIG_QUANT] = menuFunctions->addAction(tr("Config Quant..."));
      connect(menu_ids[CMD_CONFIG_QUANT], SIGNAL(triggered()), this, SLOT(configQuant()));

      menuFunctions->addSeparator();

      menuFunctions->addAction(getAction("midi_mod_gate_time", this));
      menuFunctions->addAction(getAction("midi_mod_velo", this));

#if 0 // TODO
      menuFunctions->addAction(getAction("midi_crescendo", this));
      menuFunctions->addAction(getAction("midi_transpose", this));
      menuFunctions->addAction(getAction("midi_thin_out", this));
      menuFunctions->addAction(getAction("midi_erase_event", this));
      menuFunctions->addAction(getAction("midi_note_shift", this));
      menuFunctions->addAction(getAction("midi_move_clock", this));
      menuFunctions->addAction(getAction("midi_copy_measure", this));
      menuFunctions->addAction(getAction("midi_erase_measure", this));
      menuFunctions->addAction(getAction("midi_delete_measure", this));
      menuFunctions->addAction(getAction("midi_create_measure", this));
#endif

      connect(menuSelect,    SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));
      connect(menuFunctions, SIGNAL(triggered(QAction*)), SLOT(cmd(QAction*)));

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("Pianoroll Tools"));
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
      tcanvas->setFollow(INIT_FOLLOW);

      tools2 = new EditToolBar(this, pianorollTools);
      addToolBar(tools2);

      QToolBar* panicToolbar = addToolBar(tr("Panic"));
      panicToolbar->addAction(panicAction);

      //-------------------------------------------------------------
      //    Transport Bar

      QToolBar* transport = addToolBar(tr("Transport"));
      muse->setupTransportToolbar(transport);

      addToolBarBreak();
      toolbar = new Toolbar1(initRaster, initQuant);
      addToolBar(toolbar);

      addToolBarBreak();
      info = new NoteInfo(this);
      addToolBar(info);

      setCentralWidget(tcanvas);
      tcanvas->setCornerWidget(new QSizeGrip(tcanvas));

      connect(song,     SIGNAL(posChanged(int,const AL::Pos&,bool)), canvas(), SLOT(setLocatorPos(int,const AL::Pos&,bool)));
      connect(canvas(), SIGNAL(posChanged(int,const AL::Pos&)), SLOT(setPos(int,const AL::Pos&)));

      connect(canvas(), SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(tools2, SIGNAL(toolChanged(int)), canvas(), SLOT(setTool(int)));

      connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)), SLOT(noteinfoChanged(NoteInfo::ValType, int)));

      connect(canvas(), SIGNAL(selectionChanged(int, Event&, Part*)), this,
         SLOT(setSelection(int, Event&, Part*)));

      info->setEnabled(false);

      setWindowTitle(canvas()->getCaption());
      Pos p1(0, AL::TICKS), p2(0, AL::TICKS);
      canvas()->range(p1, p2);
      p2 += AL::sigmap.ticksMeasure(p2.tick());  // show one more measure
      canvas()->setTimeRange(p1, p2);

      // connect to toolbar
      connect(canvas(), SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));
      connect(canvas(), SIGNAL(yChanged(int)), toolbar, SLOT(setInt(int)));
      connect(canvas(), SIGNAL(cursorPos(const AL::Pos&,bool)),  toolbar, SLOT(setTime(const AL::Pos&,bool)));
      connect(toolbar,  SIGNAL(quantChanged(int)), SLOT(setQuant(int)));
      connect(toolbar,  SIGNAL(rasterChanged(int)),SLOT(setRaster(int)));
      connect(toolbar,  SIGNAL(toChanged(int)),    SLOT(setApplyTo(int)));
      connect(toolbar,  SIGNAL(soloChanged(bool)), SLOT(soloChanged(bool)));

      setEventColorMode(_colorMode);

      clipboardChanged(); 	// enable/disable "Paste"
      selectionChanged(); 	// enable/disable "Copy" & "Paste"

      //
      // install misc shortcuts
      //
      QShortcut* sc = new QShortcut(Qt::Key_Escape, this);
      sc->setContext(Qt::WindowShortcut);
      connect(sc, SIGNAL(activated()), SLOT(close()));

      QSignalMapper* cmdMap = new QSignalMapper(this);
      static const char* actions[] = { 
            "curpos_increase", "curpos_decrease",
            "midi_insert_at_loc", 
            "midi_quant_1", "midi_quant_2", "midi_quant_3", "midi_quant_4",
            "midi_quant_5", "midi_quant_6", "midi_quant_7",
            "midi_quant_punct", "midi_quant_punct2", "midi_quant_triol",
            };
      for (unsigned i = 0; i < sizeof(actions)/sizeof(*actions); ++i) {
            a = getAction(actions[i], this);
            addAction(a);
            cmdMap->setMapping(a, a);
            connect(a, SIGNAL(triggered()), cmdMap, SLOT(map()));
            }
      connect(cmdMap, SIGNAL(mapped(QObject*)), SLOT(pianoCmd(QObject*)));

      connect(song, SIGNAL(songChanged(int)), canvas(), SLOT(songChanged(int)));
      connect(followSongAction, SIGNAL(toggled(bool)), canvas(), SLOT(setFollow(bool)));
      canvas()->selectFirst();

      Part* part = canvas()->part();
	setRaster(part->raster() != -1   ? part->raster() : initRaster);
      setQuant(part->quant()   != -1   ? part->quant()  : initQuant);
      setXmag(part->xmag()     != -1.0 ? part->xmag()   : initXmag);

      if (init)
            initFromPart();
      else {
	      resize(initWidth, initHeight);
            }
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoRoll::cmd(QAction* a)
      {
      canvas()->cmd(a, _quantStrength, _quantLimit, _quantLen);
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void PianoRoll::setSelection(int tick, Event& e, Part* p)
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
//    edit currently selected Event
//---------------------------------------------------------

void PianoRoll::noteinfoChanged(NoteInfo::ValType type, int val)
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
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void PianoRoll::soloChanged(bool flag)
      {
      song->setSolo(canvas()->track(), flag);
      }

//---------------------------------------------------------
//   pianoCmd
//---------------------------------------------------------

void PianoRoll::pianoCmd(QObject* object)
      {
      QAction* a = (QAction*)object;
      QString cmd(a->data().toString());

      static int rasterTable[] = {
            //-9----8-  7    6     5     4    3(1/4)     2   1
            4,  8, 16, 32,  64, 128, 256,  512, 1024,  // triple
            6, 12, 24, 48,  96, 192, 384,  768, 1536,
            9, 18, 36, 72, 144, 288, 576, 1152, 2304   // dot
            };

      int index;
      int n = sizeof(rasterTable)/sizeof(*rasterTable);
      for (index = 0; index < n; ++index)
            if (rasterTable[index] == raster())
                  break;
      if (index == n) {
            index = 0;
            // raster 1 is not in table
            }
      int off = (index / 9) * 9;
      index   = index % 9;

      int val = 0;

      PianoCanvas* pc = canvas();

      if (cmd == "curpos_increase")
            canvas()->pianoCmd(MCMD_LEFT);
      else if (cmd == "curpos_decrease")
            canvas()->pianoCmd(MCMD_RIGHT);
      else if (cmd == "midi_insert_at_loc") {
            pc->pianoCmd(MCMD_INSERT);
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
      else if (cmd == "change_event_color") {
            _colorMode = (_colorMode + 1) % 3;
            setEventColorMode(_colorMode);
            return;
            }
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
      else
            printf("unknown cmd <%s>\n", cmd.toLatin1().data());
      setQuant(val);
      setRaster(val);
      toolbar->setQuant(quant());
      toolbar->setRaster(raster());
      }

//---------------------------------------------------------
//   configQuant
//---------------------------------------------------------

void PianoRoll::configQuant()
      {
	QuantConfig quantConfig(_quantStrength, _quantLimit, _quantLen, this);
      if (!quantConfig.exec())
            return;
      _quantStrength = quantConfig.quantStrength();
      _quantLimit    = quantConfig.quantLimit();
      _quantLen      = quantConfig.doQuantLen();
      }

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void PianoRoll::setEventColorMode(QAction* a)
      {
      setEventColorMode(a->data().toInt());
      }

void PianoRoll::setEventColorMode(int mode)
      {
      _colorMode = mode;
      for (int i = 0; i < 3; ++i)
            colorModeAction[i]->setChecked(mode == i);
      canvas()->setColorMode(mode);
      }

//---------------------------------------------------------
//   readConfiguration
//	read static init values
//---------------------------------------------------------

void PianoRoll::readConfiguration(QDomNode node)
      {
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            int i = e.text().toInt();
            if (tag == "width")
                  PianoRoll::initWidth = i;
            else if (tag == "height")
                  PianoRoll::initHeight = i;
            else if (tag == "raster")
                  PianoRoll::initRaster = i;
            else if (tag == "quant")
                  PianoRoll::initQuant = i;
            else
                  printf("MusE:PianoRoll: unknown tag %s\n", tag.toLatin1().data());
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//	write static init values
//---------------------------------------------------------

void PianoRoll::writeConfiguration(Xml& xml)
      {
      xml.stag("PianoRoll");
      if (PianoRoll::initWidth != PianoRoll::INIT_WIDTH)
            xml.tag("width", PianoRoll::initWidth);
      if (PianoRoll::initHeight != PianoRoll::INIT_HEIGHT)
            xml.tag("height", PianoRoll::initHeight);
      if (PianoRoll::initRaster != PianoRoll::INIT_RASTER)
            xml.tag("raster", PianoRoll::initRaster);
      if (PianoRoll::initQuant != PianoRoll::INIT_QUANT)
            xml.tag("quant", PianoRoll::initQuant);
      xml.etag("PianoRoll");
      }
