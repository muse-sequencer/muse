//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: appearance.cpp,v 1.11.2.5 2009/11/14 03:37:48 terminator356 Exp $
//=========================================================

#include <stdio.h>
#include <q3button.h>
#include <qstring.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qcolor.h>
#include <qcolordialog.h>
#include <q3listbox.h>
#include <qlineedit.h>
#include <qtoolbutton.h>
#include <qfontdialog.h>
#include <qapplication.h>
#include <qstylefactory.h>
#include <q3listview.h>
#include <qslider.h>
#include <qstyle.h>
#include <qtooltip.h>

#include "icons.h"
#include "appearance.h"
#include "track.h"
#include "app.h"
#include "song.h"
#include "event.h"
#include "arranger.h"
#include "widgets/filedialog.h"
#include "waveedit/waveedit.h"
#include "globals.h"
#include "conf.h"
#include "gconfig.h"

//---------------------------------------------------------
//   IdListViewItem
//---------------------------------------------------------

class IdListViewItem : public Q3ListViewItem {
      int _id;

   public:
      IdListViewItem(int id, Q3ListViewItem* parent, QString s)
         : Q3ListViewItem(parent, s)
            {
            _id = id;
            }
      IdListViewItem(int id, Q3ListView* parent, QString s)
         : Q3ListViewItem(parent, s)
            {
            _id = id;
            }
      int id() const { return _id; }
      };

//---------------------------------------------------------
//   Appearance
//---------------------------------------------------------

Appearance::Appearance(Arranger* a, QWidget* parent, const char* name)
   : AppearanceDialogBase(parent, name)
      {
      arr    = a;
      color  = 0;
      config = new GlobalConfigValues;

      QToolTip::add(fontName0, tr("Main application font, and default font for any\n controls not defined here."));
      QToolTip::add(fontName1, tr("For small controls like mixer strips.\nAlso timescale small numbers, arranger part name overlay,\n and effects rack."));
      QToolTip::add(fontName2, tr("Midi track info panel. Transport controls."));
      QToolTip::add(fontName3, tr("Controller graph and S/X buttons. Large numbers for time\n and tempo scale, and time signature."));
      QToolTip::add(fontName4, tr("Time scale markers."));
      QToolTip::add(fontName5, tr("List editor: meta event edit dialog multi-line edit box."));
      QToolTip::add(fontName6, tr("Mixer label font. Auto-font-sizing up to chosen font size.\nWord-breaking but only with spaces."));
      QToolTip::add(fontSize6, tr("Maximum mixer label auto-font-sizing font size."));
      
      // ARRANGER

      /*
      currentBg = ::config.canvasBgPixmap;
      if (currentBg.isEmpty())
            currentBg = "<none>";
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
      */

	// COLORS
      IdListViewItem* id;
      IdListViewItem* aid;
      itemList->clear();
      aid = new IdListViewItem(0, itemList, "Arranger");
      id = new IdListViewItem(0, aid, "PartColors");
           new IdListViewItem(0x400, id, "Default");
           new IdListViewItem(0x401, id, "Refrain");
           new IdListViewItem(0x402, id, "Bridge");
           new IdListViewItem(0x403, id, "Intro");
           new IdListViewItem(0x404, id, "Coda");
           new IdListViewItem(0x405, id, "Chorus");
           new IdListViewItem(0x406, id, "Solo");
           new IdListViewItem(0x407, id, "Brass");
           new IdListViewItem(0x408, id, "Percussion");
           new IdListViewItem(0x409, id, "Drums");
           new IdListViewItem(0x40a, id, "Guitar");
           new IdListViewItem(0x40b, id, "Bass");
           new IdListViewItem(0x40c, id, "Flute");
           new IdListViewItem(0x40d, id, "Strings");
           new IdListViewItem(0x40e, id, "Keyboard");
           new IdListViewItem(0x40f, id, "Piano");
           new IdListViewItem(0x410, id, "Saxophon");
           new IdListViewItem(0x41c, aid, "part canvas background");
      id = new IdListViewItem(0, aid, "Track List");
           new IdListViewItem(0x411, id, "background");
           new IdListViewItem(0x412, id, "midi background");
           new IdListViewItem(0x413, id, "drum background");
           new IdListViewItem(0x414, id, "wave background");
           new IdListViewItem(0x415, id, "output background");
           new IdListViewItem(0x416, id, "input background");
           new IdListViewItem(0x417, id, "group background");
           new IdListViewItem(0x418, id, "aux background");
           new IdListViewItem(0x419, id, "synth background");
           new IdListViewItem(0x41a, id, "selected track background");
           new IdListViewItem(0x41b, id, "selected track foreground");
      id = new IdListViewItem(0, itemList, "BigTime");
           new IdListViewItem(0x100, id, "background");
           new IdListViewItem(0x101, id, "foreground");
      id = new IdListViewItem(0, itemList, "Transport");
           new IdListViewItem(0x200, id, "handle");
      id = new IdListViewItem(0, itemList, "Midi Editor");
           new IdListViewItem(0x41d, id, "controller graph");
      id = new IdListViewItem(0, itemList, "Wave Editor");
           new IdListViewItem(0x300, id, "background");

      connect(itemList, SIGNAL(selectionChanged()), SLOT(colorItemSelectionChanged()));
      connect(aPalette, SIGNAL(clicked(int)), SLOT(paletteClicked(int)));
      connect(rslider, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
      connect(gslider, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
      connect(bslider, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
      connect(hslider, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
      connect(sslider, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
      connect(vslider, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

      connect(rval, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
      connect(gval, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
      connect(bval, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
      connect(hval, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
      connect(sval, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
      connect(vval, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

      connect(addToPalette, SIGNAL(clicked()), SLOT(addToPaletteClicked()));

      //---------------------------------------------------
	//    STYLE
      //---------------------------------------------------

      /*
      themeComboBox->clear();
      QString cs = muse->style().name();
      cs = cs.lower();

      themeComboBox->insertStringList(QStyleFactory::keys());
      for (int i = 0; i < themeComboBox->count(); ++i) {
            if (themeComboBox->text(i).lower() == cs) {
                  themeComboBox->setCurrentItem(i);
                  }
            }
      */

      //---------------------------------------------------
	//    Fonts
      //---------------------------------------------------

      fontBrowse0->setPixmap(*openIcon);
      fontBrowse1->setPixmap(*openIcon);
      fontBrowse2->setPixmap(*openIcon);
      fontBrowse3->setPixmap(*openIcon);
      fontBrowse4->setPixmap(*openIcon);
      fontBrowse5->setPixmap(*openIcon);
      fontBrowse6->setPixmap(*openIcon);
      connect(fontBrowse0, SIGNAL(clicked()), SLOT(browseFont0()));
      connect(fontBrowse1, SIGNAL(clicked()), SLOT(browseFont1()));
      connect(fontBrowse2, SIGNAL(clicked()), SLOT(browseFont2()));
      connect(fontBrowse3, SIGNAL(clicked()), SLOT(browseFont3()));
      connect(fontBrowse4, SIGNAL(clicked()), SLOT(browseFont4()));
      connect(fontBrowse5, SIGNAL(clicked()), SLOT(browseFont5()));
      connect(fontBrowse6, SIGNAL(clicked()), SLOT(browseFont6()));

      connect(applyButton, SIGNAL(clicked()), SLOT(apply()));
      connect(okButton, SIGNAL(clicked()), SLOT(ok()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(selectBgButton, SIGNAL(clicked()), SLOT(configBackground()));
      connect(clearBgButton, SIGNAL(clicked()), SLOT(clearBackground()));
      connect(partShowevents, SIGNAL(toggled(bool)), eventButtonGroup, SLOT(setEnabled(bool)));
      //updateColor();
      }

//---------------------------------------------------------
//   resetValues
//---------------------------------------------------------

void Appearance::resetValues()
      {
      *config = ::config;  // init with global config values
      updateFonts();
      palette0->setPaletteBackgroundColor(config->palette[0]);
      palette1->setPaletteBackgroundColor(config->palette[1]);
      palette2->setPaletteBackgroundColor(config->palette[2]);
      palette3->setPaletteBackgroundColor(config->palette[3]);
      palette4->setPaletteBackgroundColor(config->palette[4]);
      palette5->setPaletteBackgroundColor(config->palette[5]);
      palette6->setPaletteBackgroundColor(config->palette[6]);
      palette7->setPaletteBackgroundColor(config->palette[7]);
      palette8->setPaletteBackgroundColor(config->palette[8]);
      palette9->setPaletteBackgroundColor(config->palette[9]);
      palette10->setPaletteBackgroundColor(config->palette[10]);
      palette11->setPaletteBackgroundColor(config->palette[11]);
      palette12->setPaletteBackgroundColor(config->palette[12]);
      palette13->setPaletteBackgroundColor(config->palette[13]);
      palette14->setPaletteBackgroundColor(config->palette[14]);
      palette15->setPaletteBackgroundColor(config->palette[15]);
      
      currentBg = ::config.canvasBgPixmap;
      if (currentBg.isEmpty())
            currentBg = tr("<none>");
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
      //eventButtonGroup->setEnabled(config->canvasShowPartType == 2);
      eventButtonGroup->setEnabled(config->canvasShowPartType & 2);
      arrGrid->setChecked(config->canvasShowGrid);

      themeComboBox->clear();
      QString cs = muse->style()->name();
      cs = cs.lower();

      themeComboBox->insertStringList(QStyleFactory::keys());
      for (int i = 0; i < themeComboBox->count(); ++i) {
            if (themeComboBox->text(i).lower() == cs) {
                  themeComboBox->setCurrentItem(i);
                  }
            }

      updateColor();
      
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
      fontSize0->setValue(config->fonts[0].pointSize());
      fontName0->setText(config->fonts[0].family());
      italic0->setChecked(config->fonts[0].italic());
      bold0->setChecked(config->fonts[0].bold());

      fontSize1->setValue(config->fonts[1].pointSize());
      fontName1->setText(config->fonts[1].family());
      italic1->setChecked(config->fonts[1].italic());
      bold1->setChecked(config->fonts[1].bold());

      fontSize2->setValue(config->fonts[2].pointSize());
      fontName2->setText(config->fonts[2].family());
      italic2->setChecked(config->fonts[2].italic());
      bold2->setChecked(config->fonts[2].bold());

      fontSize3->setValue(config->fonts[3].pointSize());
      fontName3->setText(config->fonts[3].family());
      italic3->setChecked(config->fonts[3].italic());
      bold3->setChecked(config->fonts[3].bold());

      fontSize4->setValue(config->fonts[4].pointSize());
      fontName4->setText(config->fonts[4].family());
      italic4->setChecked(config->fonts[4].italic());
      bold4->setChecked(config->fonts[4].bold());

      fontSize5->setValue(config->fonts[5].pointSize());
      fontName5->setText(config->fonts[5].family());
      italic5->setChecked(config->fonts[5].italic());
      bold5->setChecked(config->fonts[5].bold());
      
      fontSize6->setValue(config->fonts[6].pointSize());
      fontName6->setText(config->fonts[6].family());
      italic6->setChecked(config->fonts[6].italic());
      bold6->setChecked(config->fonts[6].bold());
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
      //if (partShowCakes->isChecked())		
      //          showPartType  |= 4;

      config->canvasShowPartType = showPartType;

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

      config->canvasShowPartEvent = showPartEvent;

      if (currentBg == tr("<none>"))
            config->canvasBgPixmap = QString();
      else      
            config->canvasBgPixmap = currentBg;
      
      // Added by Tim. p3.3.9
      config->fonts[0].setFamily(fontName0->text());
      
      config->fonts[0].setPointSize(fontSize0->value());
      config->fonts[0].setItalic(italic0->isChecked());
      config->fonts[0].setBold(bold0->isChecked());
    	QApplication::setFont(config->fonts[0], true);

      config->fonts[1].setFamily(fontName1->text());
      config->fonts[1].setPointSize(fontSize1->value());
      config->fonts[1].setItalic(italic1->isChecked());
      config->fonts[1].setBold(bold1->isChecked());

      config->fonts[2].setFamily(fontName2->text());
      config->fonts[2].setPointSize(fontSize2->value());
      config->fonts[2].setItalic(italic2->isChecked());
      config->fonts[2].setBold(bold2->isChecked());

      config->fonts[3].setFamily(fontName3->text());
      config->fonts[3].setPointSize(fontSize3->value());
      config->fonts[3].setItalic(italic3->isChecked());
      config->fonts[3].setBold(bold3->isChecked());

      config->fonts[4].setFamily(fontName4->text());
      config->fonts[4].setPointSize(fontSize4->value());
      config->fonts[4].setItalic(italic4->isChecked());
      config->fonts[4].setBold(bold4->isChecked());

      config->fonts[5].setFamily(fontName5->text());
      config->fonts[5].setPointSize(fontSize5->value());
      config->fonts[5].setItalic(italic5->isChecked());
      config->fonts[5].setBold(bold5->isChecked());

      config->fonts[6].setFamily(fontName6->text());
      config->fonts[6].setPointSize(fontSize6->value());
      config->fonts[6].setItalic(italic6->isChecked());
      config->fonts[6].setBold(bold6->isChecked());

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
      close(false);
      }

//---------------------------------------------------------
//   cancel
//---------------------------------------------------------

void Appearance::cancel()
      {
      close(false);
      }

//---------------------------------------------------------
//   configBackground
//---------------------------------------------------------

void Appearance::configBackground()
      {
      QString cur(currentBg);
      if (cur == tr("<none>"))
            cur = museGlobalShare + "/wallpapers";
      currentBg = getImageFileName(cur, image_file_pattern, this,
         tr("MusE: load image"));
      if (currentBg.isEmpty())
            currentBg = tr("<none>");
      currentBgLabel->setText(currentBg);
      }

//---------------------------------------------------------
//   clearBackground
//---------------------------------------------------------

void Appearance::clearBackground()
      {
      currentBg = tr("<none>");
      currentBgLabel->setText(currentBg);
      }

//---------------------------------------------------------
//    selectionChanged
//---------------------------------------------------------

void Appearance::colorItemSelectionChanged()
      {
      IdListViewItem* item = (IdListViewItem*)itemList->selectedItem();
      QString txt = item->text(0);
      int id = item->id();
      if (id == 0) {
            color = 0;
            return;
            }
      switch(id) {
            case 0x400: // "Default"
            case 0x401: // "Refrain"
            case 0x402: // "Bridge"
            case 0x403: // "Intro"
            case 0x404: // "Coda"
            case 0x405: // "Chorus"
            case 0x406: // "Solo"
            case 0x407: // "Brass"
            case 0x408: // "Percussion"
            case 0x409: // "Drums"
            case 0x40a: // "Guitar"
            case 0x40b: // "Bass"
            case 0x40c: // "Flute"
            case 0x40d: // "Strings
            case 0x40e: // "Keyboard
            case 0x40f: // "Piano
            case 0x410: // "Saxophon
                  color = &config->partColors[id & 0xff];
                  break;
            case 0x100: color = &config->bigTimeBackgroundColor; break;
            case 0x101: color = &config->bigTimeForegroundColor; break;
            case 0x200: color = &config->transportHandleColor; break;
            case 0x300: color = &config->waveEditBackgroundColor; break;
            case 0x411: color = &config->trackBg;       break;
            case 0x412: color = &config->midiTrackBg;   break;
            case 0x413: color = &config->drumTrackBg;   break;
            case 0x414: color = &config->waveTrackBg;   break;
            case 0x415: color = &config->outputTrackBg; break;
            case 0x416: color = &config->inputTrackBg;  break;
            case 0x417: color = &config->groupTrackBg;  break;
            case 0x418: color = &config->auxTrackBg;    break;
            case 0x419: color = &config->synthTrackBg;  break;
            case 0x41a: color = &config->selectTrackBg;  break;
            case 0x41b: color = &config->selectTrackFg;  break;
            case 0x41c: color = &config->partCanvasBg; break;
            case 0x41d: color = &config->ctrlGraphFg; break;

            default:
                  color = 0;
                  break;
            }
      updateColor();
      }

void Appearance::updateColor()
      {
      int r, g, b, h, s, v;
      rslider->setEnabled(color);
      gslider->setEnabled(color);
      bslider->setEnabled(color);
      hslider->setEnabled(color);
      sslider->setEnabled(color);
      vslider->setEnabled(color);
      rval->setEnabled(color);
      gval->setEnabled(color);
      bval->setEnabled(color);
      hval->setEnabled(color);
      sval->setEnabled(color);
      vval->setEnabled(color);
      if (color == 0)
            return;
      colorframe->setBackgroundColor(*color);
      color->getRgb(&r, &g, &b);
      color->getHsv(&h, &s, &v);

      rslider->blockSignals(true);
      gslider->blockSignals(true);
      bslider->blockSignals(true);
      hslider->blockSignals(true);
      sslider->blockSignals(true);
      vslider->blockSignals(true);
      rval->blockSignals(true);
      gval->blockSignals(true);
      bval->blockSignals(true);
      hval->blockSignals(true);
      sval->blockSignals(true);
      vval->blockSignals(true);

      rslider->setValue(r);
      gslider->setValue(g);
      bslider->setValue(b);
      hslider->setValue(h);
      sslider->setValue(s);
      vslider->setValue(v);
      rval->setValue(r);
      gval->setValue(g);
      bval->setValue(b);
      hval->setValue(h);
      sval->setValue(s);
      vval->setValue(v);

      rslider->blockSignals(false);
      gslider->blockSignals(false);
      bslider->blockSignals(false);
      hslider->blockSignals(false);
      sslider->blockSignals(false);
      vslider->blockSignals(false);
      rval->blockSignals(false);
      gval->blockSignals(false);
      bval->blockSignals(false);
      hval->blockSignals(false);
      sval->blockSignals(false);
      vval->blockSignals(false);
      }

void Appearance::rsliderChanged(int val)
      {
      int r, g, b;
      if (color) {
            color->getRgb(&r, &g, &b);
            color->setRgb(val, g, b);
            }
      updateColor();
      }

void Appearance::gsliderChanged(int val)
      {
      int r, g, b;
      if (color) {
            color->getRgb(&r, &g, &b);
            color->setRgb(r, val, b);
            }
      updateColor();
      }

void Appearance::bsliderChanged(int val)
      {
      int r, g, b;
      if (color) {
            color->getRgb(&r, &g, &b);
            color->setRgb(r, g, val);
            }
      updateColor();
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
      Q3Button* button = (Q3Button*)aPalette->selected(); // ddskrjo
      int r, g, b;
      QColor c;
      if (button) {
            int id = aPalette->id(button);
            c  = config->palette[id];
            c.getRgb(&r, &g, &b);
            }
      if (button == 0 || r != 0xff || g != 0xff || b != 0xff) {
            for (int i = 0; i < 16; ++i) {
                  c = config->palette[i];
                  c.getRgb(&r, &g, &b);
                  if (r == 0xff && g == 0xff && b == 0xff) {
                        // found empty slot
                        aPalette->setButton(i);
                        //aPalette->moveFocus(i); ddskrjo
                        button = (Q3Button*)aPalette->find(i); // ddskrjo
                        break;
                        }
                  }
            }
      if (button) {
            int id = aPalette->id(button);
            config->palette[id] = *color;
            button->setPaletteBackgroundColor(*color);
            button->update();   //??
            }
      }

//---------------------------------------------------------
//   paletteClicked
//---------------------------------------------------------

void Appearance::paletteClicked(int id)
      {
      if (!color)
            return;
      Q3Button* button = (Q3Button*)aPalette->find(id); // ddskrjo
      if (button) {
            QColor c = button->paletteBackgroundColor();
            int r, g, b;
            c.getRgb(&r, &g, &b);
            if (r == 0xff && g == 0xff && b == 0xff)
                  return;     // interpret palette slot as empty
            *color = c;
            updateColor();
            }
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
void Appearance::browseFont6() { browseFont(6); }

void Appearance::browseFont(int n)
      {
      bool ok;
      QFont font = QFontDialog::getFont(&ok, config->fonts[n], this, "browseFont");
      if (ok) {
            config->fonts[n] = font;
            updateFonts();
            }
      }

