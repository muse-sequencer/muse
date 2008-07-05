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

#include "icons.h"
#include "preferences.h"
#include "track.h"
#include "muse.h"
#include "song.h"
#include "event.h"
#include "arranger.h"
#include "widgets/filedialog.h"
#include "waveedit/waveedit.h"
#include "globals.h"
#include "conf.h"
#include "gconfig.h"

#include "audio.h"
#include "mixer.h"
#include "midirc.h"
#include "instruments/minstrument.h"
#include "midiedit/pianoroll.h"
#include "midiedit/drumedit.h"

static int rtcResolutions[] = {
      1024, 2048, 4096, 8192
      };
static int divisions[] = {
      48, 96, 192, 384, 768, 1536, 3072, 6144, 12288
      };

//---------------------------------------------------------
//   twi
//---------------------------------------------------------

static QTreeWidgetItem* twi(QTreeWidget* tw, const char* txt, int data)
      {
      QTreeWidgetItem* i  = new QTreeWidgetItem(tw);
      i->setText(0, txt);
      i->setData(0, 1, data);
      return i;
      }

static QTreeWidgetItem* twi(QTreeWidgetItem* tw, const char* txt, int data)
      {
      QTreeWidgetItem* i  = new QTreeWidgetItem(tw);
      i->setText(0, txt);
      i->setData(0, 1, data);
      return i;
      }

//---------------------------------------------------------
//   PreferencesDialog
//---------------------------------------------------------

PreferencesDialog::PreferencesDialog(Arranger* a, QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      colorframe->setAutoFillBackground(true);
      palette0->setAutoFillBackground(true);
      palette1->setAutoFillBackground(true);
      palette2->setAutoFillBackground(true);
      palette3->setAutoFillBackground(true);
      arr    = a;
      color  = 0;
      config = new GlobalConfigValues;
      resetValues();

	// ARRANGER

      usePixmap->setChecked(config->canvasUseBgPixmap);
      useColor->setChecked(!config->canvasUseBgPixmap);
      connect(usePixmap, SIGNAL(toggled(bool)), SLOT(usePixmapToggled(bool)));
      connect(useColor, SIGNAL(toggled(bool)), SLOT(useColorToggled(bool)));


      styleSheetPath->setText(config->styleSheetFile);
      currentBg = config->canvasBgPixmap;
      if (currentBg.isEmpty())
            currentBg = "<none>";
      else {
            QBrush b;
            b.setTexture(QPixmap(currentBg));
            QPalette p;
            p.setBrush(QPalette::Window, b);
            currentBgLabel->setPalette(p);
            }
      QPalette p;
      canvasBackgroundColor->setAutoFillBackground(true);
      p.setColor(QPalette::Window, config->canvasBgColor);
      canvasBackgroundColor->setPalette(p);

      currentBgLabel->setAutoFillBackground(true);
      currentBgLabel->setText(currentBg);

      partShownames->setChecked(config->canvasShowPartType & 1);
      partShowevents->setChecked(config->canvasShowPartType & 2);
      partShowCakes->setChecked(!(config->canvasShowPartType & 2));

      eventNoteon->setChecked(config->canvasShowPartEvent & (1 << 0));
      eventPolypressure->setChecked(config->canvasShowPartEvent & (1 << 1));
      eventController->setChecked(config->canvasShowPartEvent & (1 << 2));
      eventProgramchange->setChecked(config->canvasShowPartEvent & (1 << 3));
      eventAftertouch->setChecked(config->canvasShowPartEvent & (1 << 4));
      eventPitchbend->setChecked(config->canvasShowPartEvent & (1 << 5));
      eventSpecial->setChecked(config->canvasShowPartEvent & (1 << 6));
      eventButtonGroup->setEnabled(config->canvasShowPartType == 2);
      arrGrid->setChecked(config->canvasShowGrid);

	// COLORS
      QTreeWidgetItem* id;
      QTreeWidgetItem* aid;
      itemList->setSortingEnabled(false);
      itemList->clear();

      aid = twi(itemList, "Arranger", 0);
      id  = twi(aid, "PartColors", 0);
      twi(id, "Selected",   0x41d);

      twi(id, "Default",    0x400);
      twi(id, "Refrain",    0x401);
      twi(id, "Bridge",     0x402);
      twi(id, "Intro",      0x403);
      twi(id, "Coda",       0x404);
      twi(id, "Chorus",     0x405);
      twi(id, "Solo",       0x406);
      twi(id, "Brass",      0x407);
      twi(id, "Percussion", 0x408);
      twi(id, "Drums",      0x409);
      twi(id, "Guitar",     0x40a);
      twi(id, "Bass",       0x40b);
      twi(id, "Flute",      0x40c);
      twi(id, "Strings",    0x40d);
      twi(id, "Keyboard",   0x40e);
      twi(id, "Piano",      0x40f);
      twi(id, "Saxophon",   0x410);

      twi(id, "part canvas background", 0x41c);

      id = twi(aid, "Track List", 0);
      twi(id, "Audio Output",   0x500 + Track::AUDIO_OUTPUT);
      twi(id, "Audio Group",    0x500 + Track::AUDIO_GROUP);
      twi(id, "Wave Track",     0x500 + Track::WAVE);
      twi(id, "Audio Input",    0x500 + Track::AUDIO_INPUT);
      twi(id, "Synthesizer",    0x500 + Track::AUDIO_SOFTSYNTH);
      twi(id, "Midi Track",     0x500 + Track::MIDI);
      twi(id, "Midi Output",    0x500 + Track::MIDI_OUT);
      twi(id, "Midi Input",     0x500 + Track::MIDI_IN);
//      twi(id, "Midi Channel",   0x500 + Track::MIDI_CHANNEL);
      twi(id, "Midi Synti",     0x500 + Track::MIDI_SYNTI);

      id = twi(itemList, "BigTime", 0);
      twi(id, "background", 0x100);
      twi(id, "foreground", 0x101);

      id = twi(itemList, "Transport", 0);
      twi(id, "handle", 0x200);

      id = twi(itemList, "Editor", 0);
      twi(id, "background", 0x300);

      colorGroup = new QButtonGroup(this);
      colorGroup->setExclusive(true);
      colorGroup->addButton(palette0, 0);
      colorGroup->addButton(palette1, 1);
      colorGroup->addButton(palette2, 2);
      colorGroup->addButton(palette3, 3);
      colorGroup->addButton(palette4, 4);
      colorGroup->addButton(palette5, 5);
      colorGroup->addButton(palette6, 6);
      colorGroup->addButton(palette7, 7);
      colorGroup->addButton(palette8, 8);
      colorGroup->addButton(palette9, 9);
      colorGroup->addButton(palette10, 10);
      colorGroup->addButton(palette11, 11);
      colorGroup->addButton(palette12, 12);
      colorGroup->addButton(palette13, 13);
      colorGroup->addButton(palette14, 14);
      colorGroup->addButton(palette15, 15);
      connect(itemList, SIGNAL(itemSelectionChanged()), SLOT(colorItemSelectionChanged()));
      connect(colorGroup, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(paletteClicked(QAbstractButton*)));
      connect(hslider, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
      connect(sslider, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
      connect(vslider, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

      connect(addToPalette, SIGNAL(clicked()), SLOT(addToPaletteClicked()));

      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(selectCanvasBgPixmap, SIGNAL(clicked()), SLOT(configCanvasBgPixmap()));
      connect(selectCanvasBgColor, SIGNAL(clicked()), SLOT(configCanvasBgColor()));
      connect(partShowevents, SIGNAL(toggled(bool)), eventButtonGroup, SLOT(setEnabled(bool)));
      updateColor();

      for (unsigned i = 0; i < sizeof(rtcResolutions)/sizeof(*rtcResolutions); ++i) {
            if (rtcResolutions[i] == config->rtcTicks) {
                  rtcResolutionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      for (unsigned i = 0; i < sizeof(divisions)/sizeof(*divisions); ++i) {
            if (divisions[i] == config->division) {
                  midiDivisionSelect->setCurrentIndex(i);
                  break;
                  }
            }
      int i = 0;
      for (iMidiInstrument mi = midiInstruments.begin(); mi != midiInstruments.end(); ++mi, ++i) {
            preferredInstrument->addItem((*mi)->iname());
            if ((*mi)->iname() == config->defaultMidiInstrument)
                  preferredInstrument->setCurrentIndex(i);
            }

      connectToAllDevices->setChecked(config->connectToAllMidiDevices);
      connectToAllTracks->setChecked(config->connectToAllMidiTracks);
      createDefaultInput->setChecked(config->createDefaultMidiInput);

      guiRefreshSelect->setValue(config->guiRefresh);
      minSliderSelect->setValue(int(config->minSlider));
      maxSliderSelect->setValue(int(config->maxSlider));
      minMeterSelect->setValue(int(config->minMeter));
      maxMeterSelect->setValue(int(config->maxMeter));
      peakHoldTime->setValue(config->peakHoldTime);
      helpBrowser->setText(config->helpBrowser);
      startProjectEntry->setText(config->startProject);

      startProjectGroup = new QButtonGroup(this);
      startProjectGroup->addButton(alwaysAsk);
      startProjectGroup->addButton(startWithLastProject);
      startProjectGroup->addButton(startWithProject);

      switch(config->startMode) {
            case START_ASK_FOR_PROJECT:
                  alwaysAsk->setChecked(true);
                  break;
            case START_LAST_PROJECT:
                  startWithLastProject->setChecked(true);
                  break;
            case START_START_PROJECT:
                  startWithProject->setChecked(true);
                  break;
            }

      showTransport->setChecked(config->transportVisible);
      showBigtime->setChecked(config->bigTimeVisible);
      showMixer1->setChecked(config->mixer1Visible);
      showMixer2->setChecked(config->mixer2Visible);

      transportX->setValue(config->geometryTransport.x());
      transportY->setValue(config->geometryTransport.y());

      bigtimeX->setValue(config->geometryBigTime.x());
      bigtimeY->setValue(config->geometryBigTime.y());
      bigtimeW->setValue(config->geometryBigTime.width());
      bigtimeH->setValue(config->geometryBigTime.height());

      mixerX1->setValue(config->mixer1.geometry.x());
      mixerY1->setValue(config->mixer1.geometry.y());
      mixerW1->setValue(config->mixer1.geometry.width());
      mixerH1->setValue(config->mixer1.geometry.height());

      mixerX2->setValue(config->mixer2.geometry.x());
      mixerY2->setValue(config->mixer2.geometry.y());
      mixerW2->setValue(config->mixer2.geometry.width());
      mixerH2->setValue(config->mixer2.geometry.height());

      setMixerCurrent1->setEnabled(muse->mixer1Window());
      setMixerCurrent1->setEnabled(muse->mixer2Window());

      setBigtimeCurrent->setEnabled(muse->bigtimeWindow());
      setTransportCurrent->setEnabled(muse->transportWindow());
      freewheelMode->setChecked(config->useJackFreewheelMode);
      showSplash->setChecked(config->showSplashScreen);
      projectPath->setText(config->projectPath);
      templatePath->setText(config->templatePath);
      instrumentPath->setText(config->instrumentPath);
      midiImportPath->setText(config->importMidiPath);
      waveImportPath->setText(config->importWavePath);

      stopActive->setChecked(midiRCList.isActive(RC_STOP));
      playActive->setChecked(midiRCList.isActive(RC_PLAY));
      gotoLeftMarkActive->setChecked(midiRCList.isActive(RC_GOTO_LEFT_MARK));
      recordActive->setChecked(midiRCList.isActive(RC_RECORD));

      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(setMixerCurrent1, SIGNAL(clicked()), SLOT(mixerCurrent1()));
      connect(setMixerCurrent2, SIGNAL(clicked()), SLOT(mixerCurrent2()));
      connect(setBigtimeCurrent, SIGNAL(clicked()), SLOT(bigtimeCurrent()));
      connect(setArrangerCurrent, SIGNAL(clicked()), SLOT(arrangerCurrent()));
      connect(setTransportCurrent, SIGNAL(clicked()), SLOT(transportCurrent()));

      recordStop->setChecked(false);
      recordRecord->setChecked(false);
      recordGotoLeftMark->setChecked(false);
      recordPlay->setChecked(false);
      rcGroup->setChecked(rcEnable);

      pianorollWidth->setValue(PianoRoll::initWidth);
      pianorollHeight->setValue(PianoRoll::initHeight);
      pianorollRaster->setRaster(PianoRoll::initRaster);
      pianorollQuant->setQuant(PianoRoll::initQuant);

      drumEditorWidth->setValue(DrumEdit::initWidth);
      drumEditorHeight->setValue(DrumEdit::initHeight);

      waveEditorWidth->setValue(WaveEdit::initWidth);
      waveEditorHeight->setValue(WaveEdit::initHeight);

      connect(recordStop,         SIGNAL(clicked(bool)), SLOT(recordStopToggled(bool)));
      connect(recordRecord,       SIGNAL(clicked(bool)), SLOT(recordRecordToggled(bool)));
      connect(recordGotoLeftMark, SIGNAL(clicked(bool)), SLOT(recordGotoLeftMarkToggled(bool)));
      connect(recordPlay,         SIGNAL(clicked(bool)), SLOT(recordPlayToggled(bool)));

      }

//---------------------------------------------------------
//   setButtonColor
//---------------------------------------------------------

static void setButtonColor(QAbstractButton* b, const QRgb c)
      {
      QPalette p(b->palette());
      p.setColor(QPalette::Button, QColor(c));
      p.setColor(b->backgroundRole(), QColor(c));
      b->setPalette(p);
      }

//---------------------------------------------------------
//   resetValues
//---------------------------------------------------------

void PreferencesDialog::resetValues()
      {
      *config = ::config;  // init with global config values

      setButtonColor(palette0,  QColorDialog::customColor(0));
      setButtonColor(palette1,  QColorDialog::customColor(1));
      setButtonColor(palette2,  QColorDialog::customColor(2));
      setButtonColor(palette3,  QColorDialog::customColor(3));
      setButtonColor(palette4,  QColorDialog::customColor(4));
      setButtonColor(palette5,  QColorDialog::customColor(5));
      setButtonColor(palette6,  QColorDialog::customColor(6));
      setButtonColor(palette7,  QColorDialog::customColor(7));
      setButtonColor(palette8,  QColorDialog::customColor(8));
      setButtonColor(palette9,  QColorDialog::customColor(9));
      setButtonColor(palette10, QColorDialog::customColor(10));
      setButtonColor(palette11, QColorDialog::customColor(11));
      setButtonColor(palette12, QColorDialog::customColor(12));
      setButtonColor(palette13, QColorDialog::customColor(13));
      setButtonColor(palette14, QColorDialog::customColor(14));
      }

//---------------------------------------------------------
//   PreferencesDialog
//---------------------------------------------------------

PreferencesDialog::~PreferencesDialog()
      {
      delete config;
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void PreferencesDialog::apply()
      {
      ::config.styleSheetFile = styleSheetPath->text();
	int showPartEvent = 0;
	int showPartType = 0;

	if (partShownames->isChecked())
            showPartType  |= 1;
	if (partShowevents->isChecked())
            showPartType  |= 2;
 	if (partShowCakes->isChecked())
            showPartType  |= 4;

	if (eventNoteon->isChecked())
            showPartEvent |= (1 << 0);
	if (eventPolypressure->isChecked())
            showPartEvent |= (1 << 1);
	if (eventController->isChecked())
            showPartEvent |= (1 << 2);
	if (eventProgramchange->isChecked())
            showPartEvent |= (1 << 3);
	if (eventAftertouch->isChecked())
            showPartEvent |= (1 << 4);
	if (eventPitchbend->isChecked())
            showPartEvent |= (1 << 5);
	if (eventSpecial->isChecked())
            showPartEvent |= (1 << 6);

      config->canvasUseBgPixmap = usePixmap->isChecked();
      if (currentBg != "<none>")
            config->canvasBgPixmap = currentBg;

      config->canvasShowGrid = arrGrid->isChecked();
	// set colors...
      ::config = *config;

      rcEnable     = rcGroup->isChecked();
      int rtcticks = rtcResolutionSelect->currentIndex();
      int div      = midiDivisionSelect->currentIndex();

      ::config.connectToAllMidiDevices = connectToAllDevices->isChecked();
      ::config.connectToAllMidiTracks  = connectToAllTracks->isChecked();
      ::config.createDefaultMidiInput  = createDefaultInput->isChecked();
      ::config.defaultMidiInputDevice  = preferredInput->currentText();
      ::config.defaultMidiOutputDevice = preferredOutput->currentText();
      ::config.defaultMidiInstrument   = preferredInstrument->currentText();

      ::config.guiRefresh   = guiRefreshSelect->value();
      ::config.minSlider    = minSliderSelect->value();
      ::config.maxSlider    = maxSliderSelect->value();
      ::config.minMeter     = minMeterSelect->value();
      ::config.maxMeter     = maxMeterSelect->value();
      ::config.peakHoldTime = peakHoldTime->value();
      ::config.rtcTicks     = rtcResolutions[rtcticks];
      ::config.guiDivision  = divisions[div];
      ::config.helpBrowser  = helpBrowser->text();
      ::config.startProject = startProjectEntry->text();

      if (alwaysAsk->isChecked())
            ::config.startMode = START_ASK_FOR_PROJECT;
      else if (startWithLastProject->isChecked())
            ::config.startMode = START_LAST_PROJECT;
      else if (startWithProject->isChecked())
            ::config.startMode = START_START_PROJECT;

      ::config.transportVisible = showTransport->isChecked();
      ::config.bigTimeVisible   = showBigtime->isChecked();
      ::config.mixer1Visible    = showMixer1->isChecked();
      ::config.mixer2Visible    = showMixer2->isChecked();

      ::config.geometryTransport.setX(transportX->value());
      ::config.geometryTransport.setY(transportY->value());
      ::config.geometryTransport.setWidth(0);
      ::config.geometryTransport.setHeight(0);

      ::config.geometryBigTime.setX(bigtimeX->value());
      ::config.geometryBigTime.setY(bigtimeY->value());
      ::config.geometryBigTime.setWidth(bigtimeW->value());
      ::config.geometryBigTime.setHeight(bigtimeH->value());

      ::config.mixer1.geometry.setX(mixerX1->value());
      ::config.mixer1.geometry.setY(mixerY1->value());
      ::config.mixer1.geometry.setWidth(mixerW1->value());
      ::config.mixer1.geometry.setHeight(mixerH1->value());

      ::config.mixer2.geometry.setX(mixerX2->value());
      ::config.mixer2.geometry.setY(mixerY2->value());
      ::config.mixer2.geometry.setWidth(mixerW2->value());
      ::config.mixer2.geometry.setHeight(mixerH2->value());

      ::config.useJackFreewheelMode = freewheelMode->isChecked();
      ::config.showSplashScreen = showSplash->isChecked();

      ::config.projectPath    = projectPath->text();
      ::config.templatePath   = templatePath->text();
      ::config.instrumentPath = instrumentPath->text();
      ::config.importMidiPath = midiImportPath->text();
      ::config.importWavePath = waveImportPath->text();

      lastMidiPath = museUser + "/" + ::config.importMidiPath;
      lastWavePath = museUser + "/" + ::config.importWavePath;

      PianoRoll::initWidth  = pianorollWidth->value();
      PianoRoll::initHeight = pianorollHeight->value();
      PianoRoll::initRaster = pianorollRaster->raster();
      PianoRoll::initQuant  = pianorollQuant->quant();

      DrumEdit::initWidth   = drumEditorWidth->value();
      DrumEdit::initHeight  = drumEditorHeight->value();

      muse->showMixer1(::config.mixer1Visible);
      muse->showMixer2(::config.mixer2Visible);
      muse->showBigtime(::config.bigTimeVisible);
      muse->showTransport(::config.transportVisible);
      QWidget* w = muse->transportWindow();
      if (w) {
            w->resize(::config.geometryTransport.size());
            w->move(::config.geometryTransport.topLeft());
            }
      w = muse->mixer1Window();
      if (w) {
            w->resize(::config.mixer1.geometry.size());
            w->move(::config.mixer1.geometry.topLeft());
            }
      w = muse->mixer2Window();
      if (w) {
            w->resize(::config.mixer2.geometry.size());
            w->move(::config.mixer2.geometry.topLeft());
            }
      w = muse->bigtimeWindow();
      if (w) {
            w->resize(::config.geometryBigTime.size());
            w->move(::config.geometryBigTime.topLeft());
            }

      muse->setHeartBeat();        // set guiRefresh
      audio->msgSetRtc();          // set midi tick rate
      muse->changeConfig(true);    // save settings
      }

//---------------------------------------------------------
//   ok
//---------------------------------------------------------

void PreferencesDialog::ok()
      {
      apply();
      close();
      }

//---------------------------------------------------------
//   cancel
//---------------------------------------------------------

void PreferencesDialog::cancel()
      {
      close();
      }

//---------------------------------------------------------
//   configCanvasBgPixmap
//---------------------------------------------------------

void PreferencesDialog::configCanvasBgPixmap()
      {
      QString cur(currentBg);
      if (cur == "<none>")
            cur = museGlobalShare + "/wallpapers";

      QStringList pattern;
      const char** p = image_file_pattern;
      while(*p)
            pattern << *p++;
      QString s = getImageFileName(cur, pattern, this, tr("MusE: load image"));
      if (!s.isEmpty()) {
            QBrush b;
            currentBg = s;
            b.setTexture(QPixmap(s));
            QPalette p;
            p.setBrush(QPalette::Window, b);
            currentBgLabel->setPalette(p);
            currentBgLabel->setText(currentBg);
            }
      }

//---------------------------------------------------------
//   configCanvasBgColor
//---------------------------------------------------------

void PreferencesDialog::configCanvasBgColor()
      {
      QColor color = QColorDialog::getColor(config->canvasBgColor, this);
      if (color.isValid()) {
            config->canvasBgColor = color;
            QPalette p;
            p.setColor(QPalette::Window, color);
            canvasBackgroundColor->setPalette(p);
            }
      }

//---------------------------------------------------------
//    selectionChanged
//---------------------------------------------------------

void PreferencesDialog::colorItemSelectionChanged()
      {
      QTreeWidgetItem* item = (QTreeWidgetItem*)itemList->selectedItems().at(0);
      QString txt = item->text(0);
      int id = item->data(0, 1).toInt();
      if (id == 0) {
            color = 0;
            return;
            }
      switch(id) {
            case 0x400 ... 0x410: // "Default"
                  color = &config->partColors[id & 0xff];
                  break;
            case 0x100:
                  color = &config->bigTimeBackgroundColor;
                  break;
            case 0x101:
                  color = &config->bigTimeForegroundColor;
                  break;
            case 0x200:
                  color = &config->transportHandleColor;
                  break;
            case 0x300:
                  color = &config->waveEditBackgroundColor;
                  break;
            case 0x500 ... 0x5ff:
                  color = &config->trackBg[id & 0xff];
                  break;
            case 0x41c:
                  color = &config->selectPartBg;
                  break;
            default:
                  color = 0;
                  break;
            }
      updateColor();
      }

//---------------------------------------------------------
//   updateColor
//---------------------------------------------------------

void PreferencesDialog::updateColor()
      {
      hslider->setEnabled(color);
      sslider->setEnabled(color);
      vslider->setEnabled(color);
      if (color == 0)
            return;
      QPalette p(colorframe->palette());
      p.setColor(QPalette::Window, *color);
      colorframe->setPalette(p);
      int r, g, b, h, s, v;
      color->getRgb(&r, &g, &b);
      color->getHsv(&h, &s, &v);

      hslider->blockSignals(true);
      sslider->blockSignals(true);
      vslider->blockSignals(true);

      hslider->setValue(h);
      sslider->setValue(s);
      vslider->setValue(v);

      hslider->blockSignals(false);
      sslider->blockSignals(false);
      vslider->blockSignals(false);
      }

void PreferencesDialog::hsliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(val, s, v);
            }
      updateColor();
      }

void PreferencesDialog::ssliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(h, val, v);
            }
      updateColor();
      }

void PreferencesDialog::vsliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(h, s, val);
            }
      updateColor();
      }

//---------------------------------------------------------
//   addToPaletteClicked
//---------------------------------------------------------

void PreferencesDialog::addToPaletteClicked()
      {
      if (!color)
            return;
      QAbstractButton* button = colorGroup->checkedButton();
      int r, g, b;
      QColor c;
      if (button) {
            c = button->palette().color(QPalette::Button);
            c.getRgb(&r, &g, &b);
            }
      if (button == 0 || r != 0xff || g != 0xff || b != 0xff) {
            for (int i = 0; i < 16; ++i) {
                  button = colorGroup->button(i);
                  c = button->palette().color(QPalette::Button);
                  c.getRgb(&r, &g, &b);
                  if (r == 0xff && g == 0xff && b == 0xff) {
                        // found empty slot
                        button->setChecked(true);
                        break;
                        }
                  }
            }
      if (button) {
            int id = colorGroup->checkedId();
            QColorDialog::setCustomColor(id, color->rgb());
            setButtonColor(button,  color->rgb());
            }
      }

//---------------------------------------------------------
//   paletteClicked
//---------------------------------------------------------

void PreferencesDialog::paletteClicked(QAbstractButton* button)
      {
      if (color == 0)
            return;
      QColor c = button->palette().color(QPalette::Button);
      int r, g, b;
      c.getRgb(&r, &g, &b);
      if (r == 0xff && g == 0xff && b == 0xff)
            return;     // interpret palette slot as empty
      *color = c;
      updateColor();
      }

//---------------------------------------------------------
//   usePixmapToggled
//---------------------------------------------------------

void PreferencesDialog::usePixmapToggled(bool val)
      {
      useColor->setChecked(!val);
      }

//---------------------------------------------------------
//   useColorToggled
//---------------------------------------------------------

void PreferencesDialog::useColorToggled(bool val)
      {
      usePixmap->setChecked(!val);
      }

//---------------------------------------------------------
//   mixerCurrent1
//---------------------------------------------------------

void PreferencesDialog::mixerCurrent1()
      {
      QWidget* w = muse->mixer1Window();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixerX1->setValue(r.x());
      mixerY1->setValue(r.y());
      mixerW1->setValue(r.width());
      mixerH1->setValue(r.height());
      }

//---------------------------------------------------------
//   mixerCurrent2
//---------------------------------------------------------

void PreferencesDialog::mixerCurrent2()
      {
      QWidget* w = muse->mixer2Window();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      mixerX2->setValue(r.x());
      mixerY2->setValue(r.y());
      mixerW2->setValue(r.width());
      mixerH2->setValue(r.height());
      }

//---------------------------------------------------------
//   bigtimeCurrent
//---------------------------------------------------------

void PreferencesDialog::bigtimeCurrent()
      {
      QWidget* w = muse->bigtimeWindow();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      bigtimeX->setValue(r.x());
      bigtimeY->setValue(r.y());
      bigtimeW->setValue(r.width());
      bigtimeH->setValue(r.height());
      }

//---------------------------------------------------------
//   arrangerCurrent
//---------------------------------------------------------

void PreferencesDialog::arrangerCurrent()
      {
      QRect r(muse->frameGeometry());
      arrangerX->setValue(r.x());
      arrangerY->setValue(r.y());
      arrangerW->setValue(r.width());
      arrangerH->setValue(r.height());
      }

//---------------------------------------------------------
//   transportCurrent
//---------------------------------------------------------

void PreferencesDialog::transportCurrent()
      {
      QWidget* w = muse->transportWindow();
      if (!w)
            return;
      QRect r(w->frameGeometry());
      transportX->setValue(r.x());
      transportY->setValue(r.y());
      }

//---------------------------------------------------------
//   recordStopToggled
//---------------------------------------------------------

void PreferencesDialog::recordStopToggled(bool f)
      {
      recordStop->setChecked(!f);
      if (!f) {
            recordRecord->setChecked(false);
            recordGotoLeftMark->setChecked(false);
            recordPlay->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   recordRecordToggled
//---------------------------------------------------------

void PreferencesDialog::recordRecordToggled(bool f)
      {
      recordRecord->setChecked(!f);
      if (!f) {
            recordStop->setChecked(false);
            recordGotoLeftMark->setChecked(false);
            recordPlay->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   recordGotoLeftMarkToggled
//---------------------------------------------------------

void PreferencesDialog::recordGotoLeftMarkToggled(bool f)
      {
      recordGotoLeftMark->setChecked(!f);
      if (!f) {
            recordStop->setChecked(false);
            recordRecord->setChecked(false);
            recordPlay->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   recordPlayToggled
//---------------------------------------------------------

void PreferencesDialog::recordPlayToggled(bool f)
      {
      recordPlay->setChecked(!f);
      if (!f) {
            recordStop->setChecked(false);
            recordRecord->setChecked(false);
            recordGotoLeftMark->setChecked(false);
            connect(song, SIGNAL(midiEvent(MidiEvent)), SLOT(midiEventReceived(MidiEvent)));
            }
      else
            disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }

//---------------------------------------------------------
//   midiEventReceived
//---------------------------------------------------------

void PreferencesDialog::midiEventReceived(MidiEvent event)
      {
      printf("event received\n");
      if (recordPlay->isChecked()) {
            recordPlay->setChecked(false);
            playActive->setChecked(true);
            midiRCList.setAction(event, RC_PLAY);
            }
      else if (recordStop->isChecked()) {
            recordStop->setChecked(false);
            stopActive->setChecked(true);
            midiRCList.setAction(event, RC_STOP);
            }
      else if (recordRecord->isChecked()) {
            recordRecord->setChecked(false);
            recordActive->setChecked(true);
            midiRCList.setAction(event, RC_RECORD);
            }
      else if (recordGotoLeftMark->isChecked()) {
            recordGotoLeftMark->setChecked(false);
            gotoLeftMarkActive->setChecked(true);
            midiRCList.setAction(event, RC_GOTO_LEFT_MARK);
            }
      // only one shot
      disconnect(song, SIGNAL(midiEvent(MidiEvent)), this, SLOT(midiEventReceived(MidiEvent)));
      }


