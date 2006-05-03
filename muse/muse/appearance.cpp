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
#include "appearance.h"
#include "track.h"
#include "muse.h"
#include "song.h"
#include "event.h"
#include "arranger/arranger.h"
#include "widgets/filedialog.h"
#include "waveedit/waveedit.h"
#include "globals.h"
#include "conf.h"
#include "gconfig.h"

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
//   Appearance
//---------------------------------------------------------

Appearance::Appearance(Arranger* a, QWidget* parent)
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
      twi(id, "Audio Aux",      0x500 + Track::AUDIO_AUX);
      twi(id, "Wave Track",     0x500 + Track::WAVE);
      twi(id, "Audio Input",    0x500 + Track::AUDIO_INPUT);
      twi(id, "Synthesizer",    0x500 + Track::AUDIO_SOFTSYNTH);
      twi(id, "Midi Track",     0x500 + Track::MIDI);
      twi(id, "Midi Output",    0x500 + Track::MIDI_OUT);
      twi(id, "Midi Input",     0x500 + Track::MIDI_IN);
      twi(id, "Midi Channel",   0x500 + Track::MIDI_CHANNEL);
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

      //---------------------------------------------------
	//    STYLE
      //---------------------------------------------------

      themeComboBox->clear();

      QString cs = muse->style()->objectName();
      cs = cs.toLower();

      themeComboBox->addItems(QStyleFactory::keys());
      for (int i = 0; i < themeComboBox->count(); ++i) {
            if (themeComboBox->itemText(i).toLower() == cs) {
                  themeComboBox->setCurrentIndex(i);
                  }
            }

      //---------------------------------------------------
	//    Fonts
      //---------------------------------------------------

      fontBrowse0->setIcon(*openIcon);
      fontBrowse1->setIcon(*openIcon);
      fontBrowse2->setIcon(*openIcon);
      fontBrowse3->setIcon(*openIcon);
      fontBrowse4->setIcon(*openIcon);
      fontBrowse5->setIcon(*openIcon);
      connect(fontBrowse0, SIGNAL(clicked()), SLOT(browseFont0()));
      connect(fontBrowse1, SIGNAL(clicked()), SLOT(browseFont1()));
      connect(fontBrowse2, SIGNAL(clicked()), SLOT(browseFont2()));
      connect(fontBrowse3, SIGNAL(clicked()), SLOT(browseFont3()));
      connect(fontBrowse4, SIGNAL(clicked()), SLOT(browseFont4()));
      connect(fontBrowse5, SIGNAL(clicked()), SLOT(browseFont5()));

      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(selectCanvasBgPixmap, SIGNAL(clicked()), SLOT(configCanvasBgPixmap()));
      connect(selectCanvasBgColor, SIGNAL(clicked()), SLOT(configCanvasBgColor()));
      connect(partShowevents, SIGNAL(toggled(bool)), eventButtonGroup, SLOT(setEnabled(bool)));
      updateColor();
      }

//---------------------------------------------------------
//   resetValues
//---------------------------------------------------------

void Appearance::resetValues()
      {
      *config = ::config;  // init with global config values
      updateFonts();
      QPalette p(palette0->palette());
      p.setColor(QPalette::Button, config->palette[0]);
      palette0->setPalette(p);
      p.setColor(QPalette::Button, config->palette[1]);
      palette1->setPalette(p);
      p.setColor(QPalette::Button, config->palette[2]);
      palette2->setPalette(p);
      p.setColor(QPalette::Button, config->palette[3]);
      palette3->setPalette(p);
      p.setColor(QPalette::Button, config->palette[4]);
      palette4->setPalette(p);
      p.setColor(QPalette::Button, config->palette[5]);
      palette5->setPalette(p);
      p.setColor(QPalette::Button, config->palette[6]);
      palette6->setPalette(p);
      p.setColor(QPalette::Button, config->palette[7]);
      palette7->setPalette(p);
      p.setColor(QPalette::Button, config->palette[8]);
      palette8->setPalette(p);
      p.setColor(QPalette::Button, config->palette[9]);
      palette9->setPalette(p);
      p.setColor(QPalette::Button, config->palette[10]);
      palette10->setPalette(p);
      p.setColor(QPalette::Button, config->palette[11]);
      palette11->setPalette(p);
      p.setColor(QPalette::Button, config->palette[12]);
      palette12->setPalette(p);
      p.setColor(QPalette::Button, config->palette[13]);
      palette13->setPalette(p);
      p.setColor(QPalette::Button, config->palette[14]);
      palette14->setPalette(p);
      p.setColor(QPalette::Button, config->palette[15]);
      palette15->setPalette(p);
      }

//---------------------------------------------------------
//   Appearance
//---------------------------------------------------------

Appearance::~Appearance()
      {
      delete config;
      }

//---------------------------------------------------------
//   updateFonts
//---------------------------------------------------------

void Appearance::updateFonts()
      {
      fontSize0->setValue(config->fonts[0]->pointSize());
      fontName0->setText(config->fonts[0]->family());
      italic0->setChecked(config->fonts[0]->italic());
      bold0->setChecked(config->fonts[0]->bold());

      fontSize1->setValue(config->fonts[1]->pointSize());
      fontName1->setText(config->fonts[1]->family());
      italic1->setChecked(config->fonts[1]->italic());
      bold1->setChecked(config->fonts[1]->bold());

      fontSize2->setValue(config->fonts[2]->pointSize());
      fontName2->setText(config->fonts[2]->family());
      italic2->setChecked(config->fonts[2]->italic());
      bold2->setChecked(config->fonts[2]->bold());

      fontSize3->setValue(config->fonts[3]->pointSize());
      fontName3->setText(config->fonts[3]->family());
      italic3->setChecked(config->fonts[3]->italic());
      bold3->setChecked(config->fonts[3]->bold());

      fontSize4->setValue(config->fonts[4]->pointSize());
      fontName4->setText(config->fonts[4]->family());
      italic4->setChecked(config->fonts[4]->italic());
      bold4->setChecked(config->fonts[4]->bold());

      fontSize5->setValue(config->fonts[5]->pointSize());
      fontName5->setText(config->fonts[5]->family());
      italic5->setChecked(config->fonts[5]->italic());
      bold5->setChecked(config->fonts[5]->bold());
      }

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

void Appearance::apply()
      {
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
      config->fonts[0]->setPointSize(fontSize0->value());
      config->fonts[0]->setItalic(italic0->isChecked());
      config->fonts[0]->setBold(bold0->isChecked());

	QApplication::setFont(*config->fonts[0]);

      config->fonts[1]->setPointSize(fontSize1->value());
      config->fonts[1]->setItalic(italic1->isChecked());
      config->fonts[1]->setBold(bold1->isChecked());

      config->fonts[2]->setPointSize(fontSize2->value());
      config->fonts[2]->setItalic(italic2->isChecked());
      config->fonts[2]->setBold(bold2->isChecked());

      config->fonts[3]->setPointSize(fontSize3->value());
      config->fonts[3]->setItalic(italic3->isChecked());
      config->fonts[3]->setBold(bold3->isChecked());

      config->fonts[4]->setPointSize(fontSize4->value());
      config->fonts[4]->setItalic(italic4->isChecked());
      config->fonts[4]->setBold(bold4->isChecked());

      config->fonts[5]->setPointSize(fontSize5->value());
      config->fonts[5]->setItalic(italic5->isChecked());
      config->fonts[5]->setBold(bold5->isChecked());

	config->style = themeComboBox->currentText();
	// setting up a new theme might change the fontsize, so re-read
      fontSize0->setValue(QApplication::font().pointSize());

      config->canvasShowGrid = arrGrid->isChecked();
	// set colors...
      ::config = *config;
      muse->changeConfig(true);
      }

//---------------------------------------------------------
//   ok
//---------------------------------------------------------

void Appearance::ok()
      {
      apply();
      close();
      }

//---------------------------------------------------------
//   cancel
//---------------------------------------------------------

void Appearance::cancel()
      {
      close();
      }

//---------------------------------------------------------
//   configCanvasBgPixmap
//---------------------------------------------------------

void Appearance::configCanvasBgPixmap()
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

void Appearance::configCanvasBgColor()
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

void Appearance::colorItemSelectionChanged()
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

void Appearance::updateColor()
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

void Appearance::hsliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(val, s, v);
            }
      updateColor();
      }

void Appearance::ssliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(h, val, v);
            }
      updateColor();
      }

void Appearance::vsliderChanged(int val)
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

void Appearance::addToPaletteClicked()
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
            config->palette[id] = *color;
            QPalette p(button->palette());
            // p.setColor(QPalette::Active, QPalette::Button, *color);
            p.setColor(QPalette::Button, *color);
            p.setColor(button->backgroundRole(), *color);

            p.setColor(QPalette::Inactive, QPalette::Button, *color);
            button->setPalette(p);
            }
      }

//---------------------------------------------------------
//   paletteClicked
//---------------------------------------------------------

void Appearance::paletteClicked(QAbstractButton* button)
      {
      QColor c = button->palette().color(QPalette::Button);
      int r, g, b;
      c.getRgb(&r, &g, &b);
      if (r == 0xff && g == 0xff && b == 0xff)
            return;     // interpret palette slot as empty
      *color = c;
      updateColor();
      }

//---------------------------------------------------------
//   browseFont
//---------------------------------------------------------

void Appearance::browseFont0() { browseFont(0); }
void Appearance::browseFont1() { browseFont(1); }
void Appearance::browseFont2() { browseFont(2); }
void Appearance::browseFont3() { browseFont(3); }
void Appearance::browseFont4() { browseFont(4); }
void Appearance::browseFont5() { browseFont(5); }

void Appearance::browseFont(int n)
      {
      bool ok;
      QFont font = QFontDialog::getFont(&ok, *config->fonts[n], this);
      if (ok) {
            config->fonts[n] = new QFont(font);
            updateFonts();
            }
      }

//---------------------------------------------------------
//   usePixmapToggled
//---------------------------------------------------------

void Appearance::usePixmapToggled(bool val)
      {
      useColor->setChecked(!val);
      }

//---------------------------------------------------------
//   useColorToggled
//---------------------------------------------------------

void Appearance::useColorToggled(bool val)
      {
      usePixmap->setChecked(!val);
      }

