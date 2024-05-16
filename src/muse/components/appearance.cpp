//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: appearance.cpp,v 1.11.2.5 2009/11/14 03:37:48 terminator356 Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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

#include <stdio.h>
#include <errno.h>

#include <QAbstractButton>
#include <QFontDialog>
#include <QStyleFactory>
#include <QToolTip>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QtGlobal>
#include <QMessageBox>
#include <QTreeWidgetItemIterator>
#include <QMenu>

#include "icons.h"
#include "appearance.h"
#include "app.h"
#include "arranger.h"
#include "pcanvas.h"
#include "event.h"
#include "components/filedialog.h"
#include "globals.h"
// Specify muse here, two reports of "conf.h" pointing to ALSA conf.h !
#include "muse/conf.h"
#include "config.h"

// Forwards from header:
#include <QTimer>
#include <QColorDialog>
#include <QCloseEvent>
#include <QButtonGroup>
#include <QTreeWidget>
#include <QColor>
#include "gconfig.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_APPEARANCE(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

int BG_ITEM_HEIGHT = 30;

class BgPreviewWidget : public QWidget {
      QPixmap pixmap;
      QString imagefile;
      QTreeWidget* t_widget;
      int text_h;
      int text_w;

   protected:
      virtual void paintEvent(QPaintEvent* event)
            {
            QPainter p(this);
            int w = t_widget->width() - 65;
            p.drawTiledPixmap(1,1,w,BG_ITEM_HEIGHT-2, pixmap);

           const QPalette& pal = palette();
           QColor dark = pal.dark().color();
           // We can also draw a rectangle behind the text:
           //p.fillRect(QRect(w/2 - text_w/2,6,text_w + 20,text_h+4), dark);

           QFontMetrics fm = p.fontMetrics();
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
           text_w = fm.horizontalAdvance(imagefile);
#else
           text_w = fm.width(imagefile);
#endif
           text_h = fm.height();

           // Do the text shadow first
           p.save();
           p.setPen(dark);
           p.drawText(w/2 - text_w/2 + 1, 7, text_w + 20, text_h+4, Qt::AlignCenter, imagefile);
           p.restore();

           p.drawText(w/2 - text_w/2,6, text_w + 20, text_h+4, Qt::AlignCenter, imagefile);
           QWidget::paintEvent(event);
           }
   public:
      BgPreviewWidget(QString imagepath, QTreeWidget *treewidget)
            { 
            pixmap = QPixmap(imagepath);
            imagefile = imagepath.right(imagepath.length() - imagepath.lastIndexOf("/") - 1 );
            t_widget = treewidget;
            }
      };

IdListViewItem::IdListViewItem(int id, QTreeWidgetItem* parent, QString s)
    : QTreeWidgetItem(parent, QStringList(s))
      {
      _id = id;
      }
IdListViewItem::IdListViewItem(int id, QTreeWidget* parent, QString s)
    : QTreeWidgetItem(parent, QStringList(s))
      {
      _id = id;
      }
int IdListViewItem::id() const { return _id; }

//---------------------------------------------------------
//   Appearance
//---------------------------------------------------------

Appearance::Appearance(QWidget* parent)
   : QDialog(parent, Qt::Window)
      {
      setupUi(this);
      
      itemList->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(itemList, SIGNAL(customContextMenuRequested(QPoint)), SLOT(colorListCustomContextMenuReq(QPoint)));
      
      color  = nullptr;
      _colorDialog = nullptr;
      config = new MusEGlobal::GlobalConfigValues;
      backupConfig = new MusEGlobal::GlobalConfigValues;

      _configChangedTimer = new QTimer(this);
      _configChangedTimer->setObjectName("configChangedTimer");
      _configChangedTimer->setTimerType(Qt::CoarseTimer);
      _configChangedTimer->setSingleShot(true);
      _configChangedTimer->setInterval(500);
      connect(_configChangedTimer, SIGNAL(timeout()), SLOT(configChangeTimeOut()));
      
      lastSelectedColorItem = nullptr;
      lastSelectedBgItem = nullptr;
      
      fontName0->setToolTip(tr("Main application font, and default font\n"
                               "for any controls not defined here."));
      fontName1->setToolTip(tr("Mixer strips and effects racks."));
      fontName2->setToolTip(tr("Arranger part name overlay."));
      fontName3->setToolTip(tr("Time scale upper, and time signature.\n"
                               "Controller graph and S/X buttons."));
      fontName4->setToolTip(tr("Time scale lower."));
      fontName5->setToolTip(tr("Tempo scale, and markers."));
      fontName6->setToolTip(tr("Mixer labels. Auto-font-sizing up to chosen font size."));
      fontSize6->setToolTip(tr("Maximum mixer label auto-font-sizing font size."));
      
      globalAlphaSlider->setToolTip(tr("Global opacity (opposite of transparency)."));
      
      // ARRANGER

      global_bg = new QTreeWidgetItem(backgroundTree, QStringList(tr("Standard")), 0);
      global_bg->setFlags(Qt::ItemIsEnabled);
      user_bg = new QTreeWidgetItem(backgroundTree, QStringList(tr("Custom")), 0);
      user_bg->setFlags(Qt::ItemIsEnabled);
      colorwidget->setAutoFillBackground(true);
      aPalette = new QButtonGroup(aPaletteBox);

      aPalette->addButton(palette0, 0);
      aPalette->addButton(palette1, 1);
      aPalette->addButton(palette2, 2);
      aPalette->addButton(palette3, 3);
      aPalette->addButton(palette4, 4);
      aPalette->addButton(palette5, 5);
      aPalette->addButton(palette6, 6);
      aPalette->addButton(palette7, 7);
      aPalette->addButton(palette8, 8);
      aPalette->addButton(palette9, 9);
      aPalette->addButton(palette10, 10);
      aPalette->addButton(palette11, 11);
      aPalette->addButton(palette12, 12);
      aPalette->addButton(palette13, 13);
      aPalette->addButton(palette14, 14);
      aPalette->addButton(palette15, 15);
      aPalette->setExclusive(true);

	// COLORS
      IdListViewItem* id;
      IdListViewItem* aid;
      itemList->clear();
      aid = new IdListViewItem(0, itemList, "Arranger");
      id = new IdListViewItem(0, aid, "PartColors");

           for(int i = 0; i < NUM_PARTCOLORS; ++i)
             new IdListViewItem(0x600 + i, id, MusEGlobal::config.partColorNames[i]);
           
      id = new IdListViewItem(0, aid, "Track List");
           new IdListViewItem(0x411, id, "Background");
           new IdListViewItem(0x412, id, "Midi background");
           new IdListViewItem(0x413, id, "Drum background");
           // Obsolete. There is only 'New' drum tracks now.
           //new IdListViewItem(0x41e, id, "New drum background");
           new IdListViewItem(0x414, id, "Wave background");
           new IdListViewItem(0x415, id, "Output background");
           new IdListViewItem(0x416, id, "Input background");
           new IdListViewItem(0x417, id, "Group background");
           new IdListViewItem(0x418, id, "Aux background");
           new IdListViewItem(0x419, id, "Synth background");
           new IdListViewItem(0x41a, id, "Selected track background");
           new IdListViewItem(0x41b, id, "Selected track foreground");
           new IdListViewItem(0x430, id, "Last selected track background");
           new IdListViewItem(0x42b, id, "Section dividers");
           //   0x41c - 0x420 is already used (see above)

      new IdListViewItem(0x41c, aid, "Part canvas background");
      new IdListViewItem(0x42c, aid, "Part canvas raster coarse");
      new IdListViewItem(0x457, aid, "Part canvas raster beat");
      new IdListViewItem(0x42d, aid, "Part canvas raster fine");

      new IdListViewItem(0x453, aid, "Markers");
      new IdListViewItem(0x454, aid, "Range markers");
      new IdListViewItem(0x455, aid, "Position marker");
      new IdListViewItem(0x456, aid, "Current mouse position hint");

      new IdListViewItem(0x41f, aid, "Ruler background");
      new IdListViewItem(0x420, aid, "Ruler text");
      new IdListViewItem(0x424, aid, "Ruler current marker space");
      new IdListViewItem(0x425, aid, "Part wave peak");
      new IdListViewItem(0x426, aid, "Part wave rms");
      new IdListViewItem(0x427, aid, "Part midi event for light part color");
      new IdListViewItem(0x428, aid, "Part midi event for dark part color");
      new IdListViewItem(0x433, aid, "Dummy part color");

      id = new IdListViewItem(0, itemList, "BigTime");
           new IdListViewItem(0x100, id, "Background");
           new IdListViewItem(0x101, id, "Foreground");

      id = new IdListViewItem(0, itemList, "Transport");
           new IdListViewItem(0x200, id, "Handle");

      aid = new IdListViewItem(0, itemList, "Midi Editor");
      id = new IdListViewItem(0, aid, "Piano");
      new IdListViewItem(0x450, id, "Current key");
      new IdListViewItem(0x451, id, "Pressed key");
      new IdListViewItem(0x452, id, "Selected key");

      id = new IdListViewItem(0, aid, "Drum list");
      new IdListViewItem(0x422, id, "Drum list background");
      new IdListViewItem(0x440, id, "Drum list font");
      new IdListViewItem(0x441, id, "Drum list selection");
      new IdListViewItem(0x442, id, "Drum list selection font");

           new IdListViewItem(0x421, aid, "Background");
           new IdListViewItem(0x431, aid, "Item");
           new IdListViewItem(0x432, aid, "Item selected");
           new IdListViewItem(0x42a, aid, "Raster bar");
           new IdListViewItem(0x429, aid, "Raster beat");
           new IdListViewItem(0x42e, aid, "Raster fine");
           new IdListViewItem(0x41d, aid, "Controller graph");
           new IdListViewItem(0x41e, aid, "Controller graph selected");
           new IdListViewItem(0x423, aid, "Controller graph background");
           new IdListViewItem(0x42f, aid, "Divider line");


      id = new IdListViewItem(0, itemList, "Wave Editor");
           new IdListViewItem(0x300, id, "Background");
           new IdListViewItem(0x301, id, "Wave peak color");
           new IdListViewItem(0x302, id, "Wave rms color");
           new IdListViewItem(0x303, id, "Wave peak color selected");
           new IdListViewItem(0x304, id, "Wave rms color selected");
           new IdListViewItem(0x305, id, "Wave nonselected part");

      aid = new IdListViewItem(0, itemList, "Mixer");
      id = new IdListViewItem(0, aid, "Track labels (theme-dependent)");

      new IdListViewItem(0x501, id, "Midi");
      new IdListViewItem(0x502, id, "Drum");
      new IdListViewItem(0x504, id, "Wave");
      new IdListViewItem(0x505, id, "Audio output");
      new IdListViewItem(0x506, id, "Audio input");
      new IdListViewItem(0x507, id, "Group");
      new IdListViewItem(0x508, id, "Aux");
      new IdListViewItem(0x509, id, "Synth");

      id = new IdListViewItem(0, aid, "Effect rack");
      new IdListViewItem(0x530, id, "Slot background");
      new IdListViewItem(0x531, id, "Slot active");
      new IdListViewItem(0x532, id, "Font");
      new IdListViewItem(0x533, id, "Font active");
      new IdListViewItem(0x535, id, "Font mouse hover");
      new IdListViewItem(0x534, id, "Border");

      id = new IdListViewItem(0, aid, "Midi instrument");
      new IdListViewItem(0x550, id, "Background");
      new IdListViewItem(0x551, id, "Background active");
      new IdListViewItem(0x552, id, "Font");
      new IdListViewItem(0x553, id, "Font active");
      new IdListViewItem(0x554, id, "Border");
      new IdListViewItem(0x555, id, "Patch LCD font");

      id = new IdListViewItem(0, aid, "Sliders and knobs");
      new IdListViewItem(0x50b, id, "Background");
      new IdListViewItem(0x50a, id, "Slider bar (theme-dependent)");
      new IdListViewItem(0x50c, id, "Pan slider");
      new IdListViewItem(0x50d, id, "Gain slider");
      new IdListViewItem(0x50e, id, "Aux slider");
      new IdListViewItem(0x511, id, "Audio controller");
      new IdListViewItem(0x512, id, "Audio property");
      new IdListViewItem(0x513, id, "Midi controller");
      new IdListViewItem(0x514, id, "Midi property");
      new IdListViewItem(0x515, id, "Knob font (theme-dependent");

//      new IdListViewItem(0x500, aid, "Mixer background");
      new IdListViewItem(0x50f, aid, "Audio volume");
      new IdListViewItem(0x510, aid, "Midi volume");
      new IdListViewItem(0x518, aid, "Audio volume handle");
      new IdListViewItem(0x519, aid, "Midi volume handle");
      new IdListViewItem(0x516, aid, "Audio meter primary");
      new IdListViewItem(0x517, aid, "Midi meter primary");
      new IdListViewItem(0x520, aid, "Meter background (theme-dependent)");

      colorNameLineEdit->setEnabled(false);

      connect(loadColorsButton, SIGNAL(clicked(bool)), SLOT(loadColors()));
      connect(saveColorsButton, SIGNAL(clicked(bool)), SLOT(saveColors()));
      connect(pickColorButton, SIGNAL(clicked(bool)), SLOT(chooseColorClicked()));
      connect(colorwidget,     SIGNAL(clicked()),     SLOT(chooseColorClicked()));
      
      connect(colorNameLineEdit, SIGNAL(editingFinished()), SLOT(colorNameEditFinished()));
      connect(itemList, SIGNAL(itemSelectionChanged()), SLOT(colorItemSelectionChanged()));
      connect(aPalette, SIGNAL(buttonClicked(int)), SLOT(paletteClicked(int)));
      connect(globalAlphaSlider, SIGNAL(valueChanged(int)), SLOT(asliderChanged(int)));
      connect(rslider, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
      connect(gslider, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
      connect(bslider, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
      connect(hslider, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
      connect(sslider, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
      connect(vslider, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

      connect(globalAlphaVal, SIGNAL(valueChanged(int)), SLOT(aValChanged(int)));
      connect(rval, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
      connect(gval, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
      connect(bval, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
      connect(hval, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
      connect(sval, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
      connect(vval, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

      connect(addToPalette, SIGNAL(clicked()), SLOT(addToPaletteClicked()));

      //---------------------------------------------------
      //    THEMES
      //---------------------------------------------------

      QDir themeDir(MusEGlobal::museGlobalShare + QString("/themes"));
      QStringList fileTypes;
      fileTypes.append("*.qss");
      QFileInfoList list = themeDir.entryInfoList(fileTypes);
      for (const auto& item : qAsConst(list))
          themeComboBox->addItem(item.baseName());

      themeDir.setPath(MusEGlobal::configPath + QString("/themes"));
      if (themeDir.exists()) {
          list = themeDir.entryInfoList(fileTypes);
          for (const auto& item : qAsConst(list)) {
              if (themeComboBox->findText(item.baseName()) == -1)
              themeComboBox->addItem(item.baseName());
          }
      }

      themeComboBox->setCurrentText(MusEGlobal::config.theme);

      //---------------------------------------------------
      //    Fonts
      //---------------------------------------------------

      fontBrowse1->setIcon(*fileopenSVGIcon);
      fontBrowse2->setIcon(*fileopenSVGIcon);
      fontBrowse3->setIcon(*fileopenSVGIcon);
      fontBrowse4->setIcon(*fileopenSVGIcon);
      fontBrowse5->setIcon(*fileopenSVGIcon);
      fontBrowse6->setIcon(*fileopenSVGIcon);
      connect(fontBrowse1, SIGNAL(clicked()), SLOT(browseFont1()));
      connect(fontBrowse2, SIGNAL(clicked()), SLOT(browseFont2()));
      connect(fontBrowse3, SIGNAL(clicked()), SLOT(browseFont3()));
      connect(fontBrowse4, SIGNAL(clicked()), SLOT(browseFont4()));
      connect(fontBrowse5, SIGNAL(clicked()), SLOT(browseFont5()));
      connect(fontBrowse6, SIGNAL(clicked()), SLOT(browseFont6()));

      connect(applyButton, SIGNAL(clicked()), SLOT(applyClicked()));
      connect(okButton, SIGNAL(clicked()), SLOT(okClicked()));
      connect(cancelButton, SIGNAL(clicked()), SLOT(cancel()));
      connect(addBgButton, SIGNAL(clicked()), SLOT(addBackground()));
      connect(removeBgButton, SIGNAL(clicked()), SLOT(removeBackground()));
      connect(clearBgButton, SIGNAL(clicked()), SLOT(clearBackground()));
      connect(partShowevents, SIGNAL(toggled(bool)), eventButtonGroup, SLOT(setEnabled(bool)));

      //updateColor();
      
      }

Appearance::~Appearance()
      {
      delete config;
      delete backupConfig;
      }

QColor* Appearance::globalConfigColorFromId(int id) const
{
  if(id == 0) 
    return nullptr;
    
  if(id >= 0x600 && id < (0x600 + NUM_PARTCOLORS))
    return &MusEGlobal::config.partColors[id & 0xff];
  else
  {
    switch(id) 
    {
      case 0x100: return &MusEGlobal::config.bigTimeBackgroundColor;
      case 0x101: return &MusEGlobal::config.bigTimeForegroundColor;
      case 0x200: return &MusEGlobal::config.transportHandleColor;
      case 0x300: return &MusEGlobal::config.waveEditBackgroundColor;
      case 0x301: return &MusEGlobal::config.wavePeakColor;
      case 0x302: return &MusEGlobal::config.waveRmsColor;
      case 0x303: return &MusEGlobal::config.wavePeakColorSelected;
      case 0x304: return &MusEGlobal::config.waveRmsColorSelected;
      case 0x305: return &MusEGlobal::config.waveNonselectedPart;

      case 0x411: return &MusEGlobal::config.trackBg;
      case 0x412: return &MusEGlobal::config.midiTrackBg;
      // Obsolete. There is only 'New' drum tracks now.
      //case 0x413: return &MusEGlobal::config.drumTrackBg;
      //case 0x41e: return &MusEGlobal::config.newDrumTrackBg;
      case 0x413: return &MusEGlobal::config.newDrumTrackBg;
      case 0x414: return &MusEGlobal::config.waveTrackBg;
      case 0x415: return &MusEGlobal::config.outputTrackBg;
      case 0x416: return &MusEGlobal::config.inputTrackBg;
      case 0x417: return &MusEGlobal::config.groupTrackBg;
      case 0x418: return &MusEGlobal::config.auxTrackBg;
      case 0x419: return &MusEGlobal::config.synthTrackBg;
      case 0x41a: return &MusEGlobal::config.selectTrackBg;
      case 0x41b: return &MusEGlobal::config.selectTrackFg;
      case 0x430: return &MusEGlobal::config.selectTrackCurBg;

      case 0x41c: return &MusEGlobal::config.partCanvasBg;
      case 0x41d: return &MusEGlobal::config.ctrlGraphFg;
      case 0x41e: return &MusEGlobal::config.ctrlGraphSel;

      //   0x41e is already used (between 413 and 414)

      case 0x41f: return &MusEGlobal::config.rulerBg;
      case 0x420: return &MusEGlobal::config.rulerFg;
      case 0x421: return &MusEGlobal::config.midiCanvasBg;
      case 0x422: return &MusEGlobal::config.drumListBg;
      case 0x423: return &MusEGlobal::config.midiControllerViewBg;
      case 0x424: return &MusEGlobal::config.rulerCurrent;
      case 0x425: return &MusEGlobal::config.partWaveColorPeak;
      case 0x426: return &MusEGlobal::config.partWaveColorRms;
      case 0x427: return &MusEGlobal::config.partMidiDarkEventColor;
      case 0x428: return &MusEGlobal::config.partMidiLightEventColor;
      case 0x429: return &MusEGlobal::config.midiCanvasBeatColor;
      case 0x42a: return &MusEGlobal::config.midiCanvasBarColor;
      case 0x42b: return &MusEGlobal::config.trackSectionDividerColor;
      case 0x42c: return &MusEGlobal::config.partCanvasCoarseRasterColor;
      case 0x42d: return &MusEGlobal::config.partCanvasFineRasterColor;
      case 0x42e: return &MusEGlobal::config.midiCanvasFineColor;
      case 0x42f: return &MusEGlobal::config.midiDividerColor;
      case 0x431: return &MusEGlobal::config.midiItemColor;
      case 0x432: return &MusEGlobal::config.midiItemSelectedColor;
      case 0x433: return &MusEGlobal::config.dummyPartColor;

      case 0x440: return &MusEGlobal::config.drumListFont;
      case 0x441: return &MusEGlobal::config.drumListSel;
      case 0x442: return &MusEGlobal::config.drumListSelFont;

      case 0x450: return &MusEGlobal::config.pianoCurrentKey;
      case 0x451: return &MusEGlobal::config.pianoPressedKey;
      case 0x452: return &MusEGlobal::config.pianoSelectedKey;

    case 0x453: return &MusEGlobal::config.markerColor;
    case 0x454: return &MusEGlobal::config.rangeMarkerColor;
    case 0x455: return &MusEGlobal::config.positionMarkerColor;
    case 0x456: return &MusEGlobal::config.currentPositionColor;
    case 0x457: return &MusEGlobal::config.partCanvasBeatRasterColor;

//      case 0x500: return &MusEGlobal::config.mixerBg;
      case 0x501: return &MusEGlobal::config.midiTrackLabelBg;
      // Obsolete. There is only 'New' drum tracks now.
      //case 0x502: return &MusEGlobal::config.drumTrackLabelBg;
      //case 0x503: return &MusEGlobal::config.newDrumTrackLabelBg;
      case 0x502: return &MusEGlobal::config.newDrumTrackLabelBg;
      case 0x504: return &MusEGlobal::config.waveTrackLabelBg;
      case 0x505: return &MusEGlobal::config.outputTrackLabelBg;
      case 0x506: return &MusEGlobal::config.inputTrackLabelBg;
      case 0x507: return &MusEGlobal::config.groupTrackLabelBg;
      case 0x508: return &MusEGlobal::config.auxTrackLabelBg;
      case 0x509: return &MusEGlobal::config.synthTrackLabelBg;
      
      case 0x50a: return &MusEGlobal::config.sliderBarColor;
      case 0x50b: return &MusEGlobal::config.sliderBackgroundColor;
      case 0x50c: return &MusEGlobal::config.panSliderColor;
      case 0x50d: return &MusEGlobal::config.gainSliderColor;
      case 0x50e: return &MusEGlobal::config.auxSliderColor;
      case 0x50f: return &MusEGlobal::config.audioVolumeSliderColor;
      case 0x510: return &MusEGlobal::config.midiVolumeSliderColor;
      case 0x511: return &MusEGlobal::config.audioControllerSliderColor;
      case 0x512: return &MusEGlobal::config.audioPropertySliderColor;
      case 0x513: return &MusEGlobal::config.midiControllerSliderColor;
      case 0x514: return &MusEGlobal::config.midiPropertySliderColor;
      case 0x515: return &MusEGlobal::config.knobFontColor;
      case 0x516: return &MusEGlobal::config.audioMeterPrimaryColor;
      case 0x517: return &MusEGlobal::config.midiMeterPrimaryColor;
    case 0x518: return &MusEGlobal::config.audioVolumeHandleColor;
    case 0x519: return &MusEGlobal::config.midiVolumeHandleColor;
    case 0x520: return &MusEGlobal::config.meterBackgroundColor;

    case 0x530: return &MusEGlobal::config.rackItemBackgroundColor;
    case 0x531: return &MusEGlobal::config.rackItemBgActiveColor;
    case 0x532: return &MusEGlobal::config.rackItemFontColor;
    case 0x533: return &MusEGlobal::config.rackItemFontActiveColor;
    case 0x534: return &MusEGlobal::config.rackItemBorderColor;
    case 0x535: return &MusEGlobal::config.rackItemFontColorHover;

    case 0x550: return &MusEGlobal::config.midiInstrumentBackgroundColor;
    case 0x551: return &MusEGlobal::config.midiInstrumentBgActiveColor;
    case 0x552: return &MusEGlobal::config.midiInstrumentFontColor;
    case 0x553: return &MusEGlobal::config.midiInstrumentFontActiveColor;
    case 0x554: return &MusEGlobal::config.midiInstrumentBorderColor;
    case 0x555: return &MusEGlobal::config.midiPatchReadoutColor;

    default:
            return nullptr;
    }
  }
  return nullptr;
}

long int Appearance::configOffsetFromColorId(int id) const
{
  QColor* c = globalConfigColorFromId(id);
  if(!c)
    return -1;
  
  // Memory pointer HACK.
  return ((const char*)c) - ((const char*)&MusEGlobal::config);
}

QColor* Appearance::workingConfigColorFromId(int id) const
{
  long int itemOffset = configOffsetFromColorId(id);
  if(itemOffset == -1)
    return nullptr;
  return (QColor*)(((const char*)config) + itemOffset);
}

QColor* Appearance::backupConfigColorFromId(int id) const
{
  long int itemOffset = configOffsetFromColorId(id);
  if(itemOffset == -1)
    return nullptr;
  return (QColor*)(((const char*)backupConfig) + itemOffset);
}

//---------------------------------------------------------
//   setConfigurationColors
//---------------------------------------------------------

void Appearance::setConfigurationColors()
{
  palette0->setStyleSheet(QString("background-color: ") + config->palette[0].name());
  palette1->setStyleSheet(QString("background-color: ") + config->palette[1].name());
  palette2->setStyleSheet(QString("background-color: ") + config->palette[2].name());
  palette3->setStyleSheet(QString("background-color: ") + config->palette[3].name());
  palette4->setStyleSheet(QString("background-color: ") + config->palette[4].name());
  palette5->setStyleSheet(QString("background-color: ") + config->palette[5].name());
  palette6->setStyleSheet(QString("background-color: ") + config->palette[6].name());
  palette7->setStyleSheet(QString("background-color: ") + config->palette[7].name());
  palette8->setStyleSheet(QString("background-color: ") + config->palette[8].name());
  palette9->setStyleSheet(QString("background-color: ") + config->palette[9].name());
  palette10->setStyleSheet(QString("background-color: ") + config->palette[10].name());
  palette11->setStyleSheet(QString("background-color: ") + config->palette[11].name());
  palette12->setStyleSheet(QString("background-color: ") + config->palette[12].name());
  palette13->setStyleSheet(QString("background-color: ") + config->palette[13].name());
  palette14->setStyleSheet(QString("background-color: ") + config->palette[14].name());
  palette15->setStyleSheet(QString("background-color: ") + config->palette[15].name());
}
      
//---------------------------------------------------------
//   resetValues
//---------------------------------------------------------

void Appearance::resetValues()
      {
      *config = MusEGlobal::config;  // init with global config values
      *backupConfig = MusEGlobal::config;  // init with global config values
      updateFonts();
      cbAutoAdjustFontSize->setChecked(config->autoAdjustFontSize);

      setConfigurationColors();
      
      global_bg->takeChildren();
      user_bg->takeChildren();

      if (config->waveDrawing == MusEGlobal::WaveRmsPeak)
        radioButtonDrawRmsPeak->setChecked(true);
      else
        radioButtonDrawOutline->setChecked(true);

      QDir bgdir = MusEGlobal::museGlobalShare + "/wallpapers/";
      QStringList filters;
      filters << "*.jpg" << "*.jpeg" << "*.png" << "*.gif";
      bgdir.setNameFilters(filters);
      backgroundTree->model()->setData(backgroundTree->model()->index(0,0), 
                                       QVariant(QSize(200,BG_ITEM_HEIGHT)), 
                                       Qt::SizeHintRole);
      QStringList bglist = bgdir.entryList(QDir::Files, QDir::Name);
      foreach (const QString &bgfile, bglist)
            {
            QTreeWidgetItem* item = new QTreeWidgetItem(global_bg, 0);
            item->setData(0, Qt::UserRole, QVariant(MusEGlobal::museGlobalShare + "/wallpapers/" + bgfile));
            BgPreviewWidget* bgw = new BgPreviewWidget(MusEGlobal::museGlobalShare + "/wallpapers/" + bgfile, backgroundTree);
            backgroundTree->setItemWidget(item, 0, bgw);
            if (config->canvasBgPixmap == MusEGlobal::museGlobalShare + "/wallpapers/" + bgfile)
                  backgroundTree->setCurrentItem(item);
            }

      foreach (const QString &bgfile, config->canvasCustomBgList)
            {
            QTreeWidgetItem* item = new QTreeWidgetItem(user_bg, 0);
            BgPreviewWidget* bgw = new BgPreviewWidget(bgfile, backgroundTree);
            backgroundTree->setItemWidget(item, 0, bgw);
            item->setData(0, Qt::UserRole, QVariant(bgfile));
            if (config->canvasBgPixmap == bgfile)
                  backgroundTree->setCurrentItem(item);
            }

      removeBgButton->setEnabled(false);

      backgroundTree->expandAll();
      connect(backgroundTree, 
              SIGNAL(itemClicked(QTreeWidgetItem*, int )), 
              SLOT(bgSelectionChanged(QTreeWidgetItem*)));

      partShownames->setChecked(config->canvasShowPartType & 1);
      partShowevents->setChecked(config->canvasShowPartType & 2);
      partShowCakes->setChecked(!(config->canvasShowPartType & 2));
      partCakeStretch->setChecked(config->canvasShowPartType & 4);

      eventNoteon->setChecked(config->canvasShowPartEvent & (1 << 0));
      eventPolypressure->setChecked(config->canvasShowPartEvent & (1 << 1));
      eventController->setChecked(config->canvasShowPartEvent & (1 << 2));
      eventProgramchange->setChecked(config->canvasShowPartEvent & (1 << 3));
      eventAftertouch->setChecked(config->canvasShowPartEvent & (1 << 4));
      eventPitchbend->setChecked(config->canvasShowPartEvent & (1 << 5));
      eventSpecial->setChecked(config->canvasShowPartEvent & (1 << 6));
      eventButtonGroup->setEnabled(config->canvasShowPartType & 2);
      arrGrid->setChecked(config->canvasShowGrid);
      gridAlwaysShowH->setChecked(config->canvasShowGridHorizontalAlways);
      gridAlwaysShowBeats->setChecked(config->canvasShowGridBeatsAlways);

      cbUseTrackColorForParts->setChecked(config->useTrackColorForParts);

//      qtStyleComboBox->clear();
////      QString cs = MusEGlobal::muse->style()->objectName();
////      cs = cs.toLower();

////      qtStyleComboBox->insertItem(0,tr("Keep Qt system style"));
//      qtStyleComboBox->insertItems(0, QStyleFactory::keys());

//      if (QStyleFactory::keys().indexOf(config->style) == -1)
//          qtStyleComboBox->setCurrentText("Fusion"); // if none is found use the default
//      else
//          qtStyleComboBox->setCurrentText(config->style);

      //      if (QStyleFactory::keys().indexOf(config->style) == -1)
      //          qtStyleComboBox->setCurrentIndex(0); // if none is found use the default
      //      else {
      //          for (int i = 0; i < qtStyleComboBox->count(); ++i) {
      //              if (qtStyleComboBox->itemText(i).toLower() == cs) {
      //                  qtStyleComboBox->setCurrentIndex(i);
      //              }
      //          }
      //      }

      globalAlphaSlider->blockSignals(true);
      globalAlphaVal->blockSignals(true);
      globalAlphaSlider->setValue(config->globalAlphaBlend);
      globalAlphaVal->setValue(config->globalAlphaBlend);
      globalAlphaSlider->blockSignals(false);
      globalAlphaVal->blockSignals(false);
      
      maxAliasedPointSize->blockSignals(true);
      maxAliasedPointSize->setValue(config->maxAliasedPointSize);
      maxAliasedPointSize->blockSignals(false);
          
      iconSizeSpin->blockSignals(true);
      iconSizeSpin->setValue(config->iconSize);
      iconSizeSpin->blockSignals(false);
      cursorSizeSpin->blockSignals(true);
      cursorSizeSpin->setValue(config->cursorSize);
      cursorSizeSpin->blockSignals(false);
      trackGradientSpin->blockSignals(true);
      trackGradientSpin->setValue(config->trackGradientStrength);
      trackGradientSpin->blockSignals(false);
      partGradientSpin->blockSignals(true);
      partGradientSpin->setValue(config->partGradientStrength);
      partGradientSpin->blockSignals(false);

      cascadeStylesheetsCheckBox->setChecked(config->cascadeStylesheets);
      cbShowIconsInMenus->setChecked(config->showIconsInMenus);
      cbUseNativeStandardDialogs->setChecked(config->useNativeStandardDialogs);

      // Grab all the colours.
      updateColorItems();

      updateColor();
}

QString &Appearance::getSetDefaultStyle(const QString *newStyle)
{
   static QString defaultStyle = "Fusion";

   if (newStyle)
      defaultStyle = *newStyle;

   return defaultStyle;
}


//---------------------------------------------------------
//   bgSelectionChanged
//---------------------------------------------------------

void Appearance::bgSelectionChanged(QTreeWidgetItem* item)
      {
      if (item->text(0).length() && lastSelectedBgItem)
            {
            backgroundTree->setCurrentItem(lastSelectedBgItem);
            item = lastSelectedBgItem;
            }

      removeBgButton->setEnabled(false);
  
      QTreeWidgetItem* parent = item->parent();
      if (parent)
            if (parent->text(0) == user_bg->text(0))
	          removeBgButton->setEnabled(true);
  
      lastSelectedBgItem = item;
      MusEGlobal::muse->arranger()->getCanvas()->setBg(QPixmap(item->data(0, Qt::UserRole).toString()));
      }

//---------------------------------------------------------
//   updateFonts
//---------------------------------------------------------

void Appearance::updateFonts()
      {
      fontSize0->setValue(MusEGlobal::config.fonts[0].pointSize());
      fontName0->setText(MusEGlobal::config.fonts[0].family());
      italic0->setChecked(MusEGlobal::config.fonts[0].italic());
      bold0->setChecked(MusEGlobal::config.fonts[0].bold());

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
//   changeTheme
//
//---------------------------------------------------------

bool Appearance::changeTheme()
{
    QString currentTheme = themeComboBox->currentText();

    if (config->theme == currentTheme)
        return false;

    printf("Changing to theme %s\n", qPrintable(currentTheme) );

    MusEGlobal::config.theme = currentTheme;

    if (!isColorsDirty())
        saveCurrentThemeColors();

    QString configColorPath = MusEGlobal::configPath + "/themes/" + currentTheme + ".cfc";
    if (!QFile::exists(configColorPath)) {
        configColorPath = MusEGlobal::museGlobalShare + "/themes/" + currentTheme + ".cfc";
    }

    // We want the simple version, don't set the style or stylesheet yet.
    MusECore::readConfiguration(qPrintable(configColorPath));
//    MusEGlobal::muse->changeConfig(true);

    backgroundTree->reset();
    hide();

    return true;
}

void Appearance::saveCurrentThemeColors() {

    QDir dir(MusEGlobal::configPath + "/themes/");
    if (!dir.exists())
        dir.mkpath(MusEGlobal::configPath + "/themes/");

    QString lastColorPath = MusEGlobal::configPath + "/themes/" + config->theme + ".cfc";

    FILE* f = fopen(qPrintable(lastColorPath), "w");
    if (!f) {
        fprintf(stderr, "Saving configuration colors to <%s> failed: %s\n",
                qPrintable(lastColorPath), strerror(errno));
    } else {
        MusECore::Xml xml(f);
        xml.header();
        xml.nput(0, "<muse version=\"%d.%d\">\n", xml.latestMajorVersion(), xml.latestMinorVersion());
        xml.tag(1, "configuration");
        MusECore::writeConfigurationColors(2, xml, false); // Don't save part colour names.
        xml.etag(1, "configuration");
        xml.tag(0, "/muse");
        fclose(f);
    }
}

//---------------------------------------------------------
//   apply
//---------------------------------------------------------

bool Appearance::apply()
{
      bool restart_required = false;

      if (isColorsDirty())
          saveCurrentThemeColors();

      if (changeTheme()) {
          *config = MusEGlobal::config;
          restart_required = true;
      }

      int showPartEvent = 0;
      int showPartType = 0;

      if (partShownames->isChecked())
                showPartType  |= 1;
      if (partShowevents->isChecked())
                showPartType  |= 2;
      if (partCakeStretch->isChecked())
                showPartType  |= 4;

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

      QTreeWidgetItem* cbgitem = backgroundTree->currentItem();

      if (cbgitem)
            config->canvasBgPixmap = cbgitem->data(0, Qt::UserRole).toString();
      else
            config->canvasBgPixmap = QString();

      config->canvasCustomBgList = QStringList();
      for (int i = 0; i < user_bg->childCount(); ++i)
            config->canvasCustomBgList << user_bg->child(i)->data(0, Qt::UserRole).toString();

//      config->fonts[0].setFamily(fontName0->text());
//      config->fonts[0].setPointSize(fontSize0->value());
//      config->fonts[0].setItalic(italic0->isChecked());
//      config->fonts[0].setBold(bold0->isChecked());

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

//      if(config->style != (qtStyleComboBox->currentIndex() == 0 ? QString() : qtStyleComboBox->currentText()))
//      {
//        restart_required = true;
//        config->style = qtStyleComboBox->currentIndex() == 0 ? QString() : qtStyleComboBox->currentText();
//      }

      config->canvasShowGrid = arrGrid->isChecked();
      config->canvasShowGridHorizontalAlways = gridAlwaysShowH->isChecked();
      config->canvasShowGridBeatsAlways = gridAlwaysShowBeats->isChecked();
      config->useTrackColorForParts = cbUseTrackColorForParts->isChecked();
      config->globalAlphaBlend = globalAlphaVal->value();
      config->maxAliasedPointSize = maxAliasedPointSize->value();
      config->autoAdjustFontSize = cbAutoAdjustFontSize->isChecked();

      if (config->iconSize != iconSizeSpin->value()) {
          restart_required = true;
          config->iconSize = iconSizeSpin->value();
      }

      if (config->cursorSize != cursorSizeSpin->value()) {
          restart_required = true;
          config->cursorSize = cursorSizeSpin->value();
      }

      if (config->trackGradientStrength != trackGradientSpin->value()) {
          config->trackGradientStrength = trackGradientSpin->value();
      }

      if (config->partGradientStrength != partGradientSpin->value()) {
          config->partGradientStrength = partGradientSpin->value();
      }

      if (config->cascadeStylesheets != cascadeStylesheetsCheckBox->isChecked()) {
          restart_required = true;
          config->cascadeStylesheets = cascadeStylesheetsCheckBox->isChecked();
      }

      if (config->showIconsInMenus != cbShowIconsInMenus->isChecked()) {
          restart_required = true;
          config->showIconsInMenus = cbShowIconsInMenus->isChecked();
      }

      if (config->useNativeStandardDialogs != cbUseNativeStandardDialogs->isChecked()) {
          restart_required = true;
          config->useNativeStandardDialogs = cbUseNativeStandardDialogs->isChecked();
      }

      if (radioButtonDrawOutline->isChecked())
        config->waveDrawing = MusEGlobal::WaveOutLine;
      else
        config->waveDrawing = MusEGlobal::WaveRmsPeak;


      MusEGlobal::config = *config;
      *backupConfig = *config;
      updateColorItems();

      // We want the simple version, don't set the style or stylesheet yet.
      MusEGlobal::muse->changeConfig(true);
      raise();

      return restart_required;
}

//---------------------------------------------------------
//   checkClose
//---------------------------------------------------------

bool Appearance::checkClose()
{
  if(QMessageBox::warning(MusEGlobal::muse, QString("Muse"),
    tr("Style was changed.\nThe program must be restarted for changes to take place.\nRestart now?"),
    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
     == QMessageBox::Yes)
  {
    MusEGlobal::muse->setRestartingApp(true);
    if(MusEGlobal::muse->close())
      return true;
  }

  // We want the non-simple version, set the style and stylesheet, and don't save - it's already been done.
  // And force the style.
  MusEGlobal::muse->changeConfig(false);
//   MusEGlobal::muse->updateThemeAndStyle(true);

  MusEGlobal::muse->setRestartingApp(false); // Cancel any restart. Also cleared in muse->closeEvent().
  return false;
}

//---------------------------------------------------------
//   applyClicked
//---------------------------------------------------------

void Appearance::applyClicked()
{
  if(apply())
    // If required, ask user to restart the application for proper results.
    checkClose();
}

//---------------------------------------------------------
//   colorNameEditFinished
//---------------------------------------------------------

void Appearance::colorNameEditFinished()
{
  if(!lastSelectedColorItem)
    return;
    
  IdListViewItem* item = (IdListViewItem*)lastSelectedColorItem;
  int id = item->id();
  if(id == 0) 
    return;
  
  QString etxt = colorNameLineEdit->text();
  QString txt = item->text(0);
  // We only support part color names, for now.
  if(id >= 0x600 && id < (0x600 + NUM_PARTCOLORS))
    config->partColorNames[id & 0xff] = etxt;
  if(etxt != txt)
    item->setText(0, etxt);
}

void Appearance::loadColors()
{
    if (!MusEGlobal::muse->loadConfigurationColors(this))
    {
        DEBUG_APPEARANCE(stderr, "Appearance::loadColors failed\n");
        return;
    }

    //  resetValues();
    *config = MusEGlobal::config;
    updateColorItems();
    setConfigurationColors();
    updateColor();
}

void Appearance::saveColors()
{
  MusEGlobal::muse->saveConfigurationColors(this);
}

bool Appearance::isColorDirty(IdListViewItem* item) const
{
  if(!item)
    return false;
  int id = item->id();
  if(id == 0) 
    return false;

  QColor* p_gc = globalConfigColorFromId(id);
  if(!p_gc)
    return false;
  
  QColor* p_bkc = backupConfigColorFromId(id);
  if(!p_bkc)
    return false;

  const QColor& gc = *p_gc;
  const QColor& bkc = *p_bkc;
  
  return gc != bkc;
}

bool Appearance::isColorsDirty() const
{
  QTreeWidgetItemIterator it(itemList);
  while(*it)
  {
    if(isColorDirty((IdListViewItem*)*it))
      return true;
    ++it;
  }
  return false;
}

void Appearance::setColorItemDirty()
{
  IdListViewItem* item = (IdListViewItem*)itemList->selectedItems()[0];
  if(!item)
    return;
  setColorItemDirty(item);
}

void Appearance::setColorItemDirty(IdListViewItem* item)
{
  if(!item)
    return;
  int id = item->id();
  //if(id == 0 || !color) 
  if(id == 0) 
    return;

  QColor* p_gc = globalConfigColorFromId(id);
  if(!p_gc)
    return;
  
  QColor* p_bkc = backupConfigColorFromId(id);
  if(!p_bkc)
    return;

  const QColor& gc = *p_gc;
  const QColor& bkc = *p_bkc;
  
  QFont fnt = item->font(0);
  //fnt.setBold(bkc != gc);
  fnt.setWeight(bkc != gc ? QFont::Black : QFont::Normal);
  fnt.setItalic(bkc != gc);
  item->setFont(0, fnt);
  item->setData(0, Qt::DecorationRole, gc);
}

void Appearance::updateColorItems()
{
  QTreeWidgetItemIterator it(itemList); 
  while(*it)
  {
    setColorItemDirty((IdListViewItem*)*it);
    ++it;
  }  
}

void Appearance::colorListCustomContextMenuReq(const QPoint& p)
{
  DEBUG_APPEARANCE(stderr, "Appearance::colorListCustomContextMenuReq\n");
  
  IdListViewItem* item = static_cast<IdListViewItem*>(itemList->itemAt(p));
  bool itemDirty = item && isColorDirty(item);
  
  QMenu* pup = new QMenu(this);
  QAction* act = pup->addAction(tr("Revert changes"));
  act->setData(0x100);
  act->setEnabled(itemDirty);
  act = pup->addAction(tr("Revert all..."));
  act->setData(0x101);
  act->setEnabled(isColorsDirty());
  act = pup->exec(itemList->mapToGlobal(p));
  if(!act)
  {
    delete pup;
    return;
  }
  
  const int res = act->data().toInt();
  delete pup;
  
  switch(res)
  {
    case 0x100:
    {
      if(item && isColorDirty(item))
      {
        resetColorItem(item);
        updateColor();
        if(color && _colorDialog)
        {
          _colorDialog->blockSignals(true);
          _colorDialog->setCurrentColor(*color);
          _colorDialog->blockSignals(false);
        }
        // Notify the rest of the app, without the heavy stuff.
        MusEGlobal::muse->changeConfig(false); // No write, and simple mode.
      }
    } 
    break;
    
    case 0x101:
    {
      if(QMessageBox::question(this, QString("Muse"),
          tr("Do you really want to reset all colors?"), 
          QMessageBox::Ok | QMessageBox::Cancel,
          QMessageBox::Ok) != QMessageBox::Ok)
        return;
      resetAllColorItems();
      
      updateColor();
      if(color && _colorDialog)
      {
        _colorDialog->blockSignals(true);
        _colorDialog->setCurrentColor(*color);
        _colorDialog->blockSignals(false);
      }
      //changeGlobalColor();
      // Notify the rest of the app, without the heavy stuff.
      MusEGlobal::muse->changeConfig(false); // No write, and simple mode.
    } 
    break;
  }
}

void Appearance::resetColorItem(IdListViewItem* item)
{
  if(!item)
    return;
  int id = item->id();
  if(id == 0)
    return;
  
  QColor* p_bkc = backupConfigColorFromId(id);
  if(!p_bkc)
    return;

  QColor* p_gc = globalConfigColorFromId(id);
  if(!p_gc)
    return;

  QColor* p_wkc = workingConfigColorFromId(id);
  if(!p_wkc)
    return;

  const QColor& bkc = *p_bkc;
  QColor& gc = *p_gc;
  QColor& wkc = *p_wkc;

  gc = bkc;
  wkc = bkc;
  
  QFont fnt = item->font(0);
  fnt.setWeight(QFont::Normal);
  fnt.setItalic(false);
  item->setFont(0, fnt);
  item->setData(0, Qt::DecorationRole, gc);
}

void Appearance::resetAllColorItems()
{
  QTreeWidgetItemIterator it(itemList); 
  while(*it)
  {
    resetColorItem((IdListViewItem*)*it);
    ++it;
  }  
}

void Appearance::chooseColorClicked()
{
  if(!color)
    return;
  if(!_colorDialog)
  {
    DEBUG_APPEARANCE(stderr, "Appearance::chooseColorClicked creating new dialog\n");
    _colorDialog = new QColorDialog(this);
    _colorDialog->setOption(QColorDialog::NoButtons, true);
    connect(_colorDialog, SIGNAL(currentColorChanged(QColor)), SLOT(colorDialogCurrentChanged(QColor)));
    connect(_colorDialog, SIGNAL(finished(int)), SLOT(colorDialogFinished(int)));
  }
  _colorDialog->setCurrentColor(*color);
  
  IdListViewItem* item = (IdListViewItem*)itemList->selectedItems()[0];
  if(item)
    setColorDialogWindowText(item->text(0));
  else
    setColorDialogWindowText();
    
  QRect r = this->geometry();
  _colorDialog->move(r.x() + 250, r.y() + 170);
  _colorDialog->show();
  _colorDialog->raise();
}

void Appearance::changeGlobalColor()
{
  if(!color)
    return;

  // Memory pointer HACK to get to the global config color.
  unsigned long int itemOffset = ((const char*)color) - ((const char*)config);
  const char* configOffset = ((const char*)&MusEGlobal::config) + itemOffset;
  
  // Change the actual global config item, 'live'.
  QColor& ccol = *((QColor*)configOffset);
  if(ccol != *color)
  {
    ccol = *color;
    // Notify the rest of the app, without the heavy stuff.
    MusEGlobal::muse->changeConfig(false); // No write, and simple mode.
  }

  setColorItemDirty();
}

void Appearance::changeColor(const QColor& c)
{
//   if(!color)
//     return;

  if(color && *color != c)
  {
    *color = c;
    //updateColor();
  }
  
  _configChangedTimer->start(); // Restart
}

void Appearance::colorDialogCurrentChanged(const QColor& c)
{
  changeColor(c);
  //updateColor();
}

void Appearance::colorDialogFinished(int /*result*/)
{
  DEBUG_APPEARANCE(stderr, "Appearance::colorDialogFinished result:%d\n", result);
  if(_configChangedTimer->isActive())
   _configChangedTimer->stop();
  
  if(_colorDialog)
  {
    _colorDialog->deleteLater();
    _colorDialog = 0;
  }
}

void Appearance::configChangeTimeOut()
{
  updateColor();
  if(color && _colorDialog)
  {
    _colorDialog->blockSignals(true);
    _colorDialog->setCurrentColor(*color);
    _colorDialog->blockSignals(false);
  }
  changeGlobalColor();
}

//---------------------------------------------------------
//   doCancel
//---------------------------------------------------------

void Appearance::doCancel()
{
  MusEGlobal::muse->arranger()->getCanvas()->setBg(QPixmap(config->canvasBgPixmap));
  MusEGlobal::config = *backupConfig;
  // Save settings. Use non-simple version - set style and stylesheet.
  MusEGlobal::muse->changeConfig(true); // Restore everything possible.
//   MusEGlobal::muse->updateThemeAndStyle(true);
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void Appearance::closeEvent(QCloseEvent* e)
{
  DEBUG_APPEARANCE(stderr, "Appearance::closeEvent\n");
  doCancel();

  if(_colorDialog)
  {
    _colorDialog->deleteLater();
    _colorDialog = nullptr;
  }
  e->accept();
  QDialog::closeEvent(e);
}

//---------------------------------------------------------
//   okClicked
//---------------------------------------------------------

void Appearance::okClicked()
      {
      DEBUG_APPEARANCE(stderr, "Appearance::ok\n");
      if(_colorDialog)
      {
        _colorDialog->deleteLater();
        _colorDialog = nullptr;
      }
      //close();
      hide();

      if(apply())
        // If required, ask user to restart the application for proper results.
        checkClose();

      }

//---------------------------------------------------------
//   cancel
//---------------------------------------------------------

void Appearance::cancel()
      {
      DEBUG_APPEARANCE(stderr, "Appearance::cancel\n");
      doCancel();
      if(_colorDialog)
      {
        _colorDialog->deleteLater();
        _colorDialog = nullptr;
      }
      //close();
      hide();
      }

//---------------------------------------------------------
//   removeBackground
//---------------------------------------------------------

void Appearance::removeBackground()
      {
      QTreeWidgetItem* item = backgroundTree->currentItem();
      MusEGlobal::muse->arranger()->getCanvas()->setBg(QPixmap());
      user_bg->takeChild(user_bg->indexOfChild(item));
      backgroundTree->setCurrentItem (nullptr);
      removeBgButton->setEnabled(false);
      }

//---------------------------------------------------------
//   addBackground
//---------------------------------------------------------

void Appearance::addBackground()
      {
      QString cur = getenv("HOME");
      QString user_bgfile = getImageFileName(cur, MusEGlobal::image_file_pattern, this,
                                             tr("MusE: load image"));

      bool image_exists = false;
      for (int i = 0; i < global_bg->childCount(); ++i)
            if (global_bg->child(i)->data(0, Qt::UserRole).toString() == user_bgfile)
                  image_exists = true;
      for (int i = 0; i < user_bg->childCount(); ++i)
            if (user_bg->child(i)->data(0, Qt::UserRole).toString() == user_bgfile)
                  image_exists = true;

      if (! image_exists)
            {
            QTreeWidgetItem* item = new QTreeWidgetItem(user_bg, 0);
            item->setData(0, Qt::UserRole, QVariant(user_bgfile));
            BgPreviewWidget* bgw = new BgPreviewWidget(user_bgfile, backgroundTree);
            backgroundTree->setItemWidget(item, 0, bgw);
            }
      }

//---------------------------------------------------------
//   clearBackground
//---------------------------------------------------------

void Appearance::clearBackground()
      {
      MusEGlobal::muse->arranger()->getCanvas()->setBg(QPixmap());
      backgroundTree->setCurrentItem (0);
      removeBgButton->setEnabled(false);
      }

//---------------------------------------------------------
//    setColorDialogWindowText
//---------------------------------------------------------

void Appearance::setColorDialogWindowText(const QString& colorName)
{
  if(!_colorDialog)
    return;
  
  if(colorName.isEmpty())
    _colorDialog->setWindowTitle(tr("No current color item"));
  else
  {
    const QString title = tr("Select Color: %1").arg(colorName);
    _colorDialog->blockSignals(true);
    _colorDialog->setWindowTitle(title);
    _colorDialog->blockSignals(false);
  }
}

//---------------------------------------------------------
//    selectionChanged
//---------------------------------------------------------

void Appearance::colorItemSelectionChanged()
      {
      IdListViewItem* item = (IdListViewItem*)itemList->selectedItems()[0];
      lastSelectedColorItem = 0;
      if(!item)
      {
        colorNameLineEdit->setEnabled(false);
        setColorDialogWindowText();
        updateColor();
        return;
      }
      
      int id = item->id();
      
      color = workingConfigColorFromId(id);
      if(!color)
      {
        lastSelectedColorItem = 0;
        colorNameLineEdit->setEnabled(false);
        setColorDialogWindowText();
        updateColor();
        return;
      }
            
      bool enle = false;
      if(id >= 0x600 && id < (0x600 + NUM_PARTCOLORS))
      {
        lastSelectedColorItem = item;
        enle = true;
      }
      
      colorNameLineEdit->setEnabled(enle);
      QString s;
      if(enle)
        s = config->partColorNames[id & 0xff];
      colorNameLineEdit->setText(s);
      updateColor();
      
      if(_colorDialog)
      {
        _colorDialog->blockSignals(true);
        _colorDialog->setCurrentColor(*color);
        setColorDialogWindowText(item->text(0));
        _colorDialog->blockSignals(false);
      }
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
      colorwidget->setEnabled(color);
      colorwidget->setEnabled(color);
      pickColorButton->setEnabled(color);
      if (color == 0)
            return;
      QPalette pal;
      QColor cfc(*color);
      
      colorwidget->setColor(cfc);

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

void Appearance::asliderChanged(int val)
      {
      globalAlphaVal->blockSignals(true);
      globalAlphaVal->setValue(val);
      globalAlphaVal->blockSignals(false);
      updateColor();
      }

void Appearance::aValChanged(int val)
      {
      globalAlphaSlider->blockSignals(true);
      globalAlphaSlider->setValue(val);
      globalAlphaSlider->blockSignals(false);
      updateColor();
      }

void Appearance::rsliderChanged(int val)
      {
      int r, g, b;
      if (color) {
            color->getRgb(&r, &g, &b);
            color->setRgb(val, g, b);
            }
      updateColor();
      _configChangedTimer->start(); // Restart
      }

void Appearance::gsliderChanged(int val)
      {
      int r, g, b;
      if (color) {
            color->getRgb(&r, &g, &b);
            color->setRgb(r, val, b);
            }
      updateColor();
      _configChangedTimer->start(); // Restart
      }

void Appearance::bsliderChanged(int val)
      {
      int r, g, b;
      if (color) {
            color->getRgb(&r, &g, &b);
            color->setRgb(r, g, val);
            }
      updateColor();
      _configChangedTimer->start(); // Restart
      }

void Appearance::hsliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(val, s, v);
            }
      updateColor();
      _configChangedTimer->start(); // Restart
      }

void Appearance::ssliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(h, val, v);
            }
      updateColor();
      _configChangedTimer->start(); // Restart
      }

void Appearance::vsliderChanged(int val)
      {
      int h, s, v;
      if (color) {
            color->getHsv(&h, &s, &v);
            color->setHsv(h, s, val);
            }
      updateColor();
      _configChangedTimer->start(); // Restart
      }

//---------------------------------------------------------
//   addToPaletteClicked
//---------------------------------------------------------

void Appearance::addToPaletteClicked()
      {
      QColor new_c = color ? *color : colorwidget->color();

      QAbstractButton* button = (QAbstractButton*)aPalette->checkedButton(); // ddskrjo

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
		    aPalette->button(i)->toggle();
                        button = (QAbstractButton*)aPalette->button(i); // ddskrjo
                        break;
                        }
                  }
            }
      if (button) {
            int id = aPalette->id(button);
            config->palette[id] = new_c;
            button->setStyleSheet(QString("background-color: ") + new_c.name());
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
      QAbstractButton* button = (QAbstractButton*)aPalette->button(id); // ddskrjo
      if (button) {
        QColor c = button->palette().color(QPalette::Window);
            int r, g, b;
            c.getRgb(&r, &g, &b);
            if (r == 0xff && g == 0xff && b == 0xff)
                  return;     // interpret palette slot as empty
            *color = c;
            updateColor();
            _configChangedTimer->start(); // Restart
            }
      }

//---------------------------------------------------------
//   browseFont
//---------------------------------------------------------

void Appearance::browseFont1() { browseFont(1); }
void Appearance::browseFont2() { browseFont(2); }
void Appearance::browseFont3() { browseFont(3); }
void Appearance::browseFont4() { browseFont(4); }
void Appearance::browseFont5() { browseFont(5); }
void Appearance::browseFont6() { browseFont(6); }

void Appearance::browseFont(int n)
      {
      bool ok;
      QFont f(config->fonts[n]);
      if (!f.bold() && !f.italic())
          f.setStyleName("Regular");
      QFont font = QFontDialog::getFont(&ok, f, this, "browseFont");
      if (ok) {
            config->fonts[n] = font;
            updateFonts();
            }
      }

void Appearance::on_pbAdjustFontSizes_clicked()
{
    int fs = fontSize0->value();
    fontSize1->setValue(qRound(fs * MusEGlobal::FntFac::F1));
    fontSize2->setValue(qRound(fs * MusEGlobal::FntFac::F2));
    fontSize3->setValue(qRound(fs * MusEGlobal::FntFac::F3));
    fontSize4->setValue(qRound(fs * MusEGlobal::FntFac::F4));
    fontSize5->setValue(qRound(fs * MusEGlobal::FntFac::F5));
    fontSize6->setValue(qRound(fs * MusEGlobal::FntFac::F6));
}

void Appearance::on_pbSetFontFamily_clicked()
{
    fontName1->setText(fontName0->text());
    fontName2->setText(fontName0->text());
    fontName3->setText(fontName0->text());
    fontName4->setText(fontName0->text());
    fontName5->setText(fontName0->text());
    fontName6->setText(fontName0->text());
}


void Appearance::on_helpButton_clicked()
{
    QString s(ORGANIZATION_HELP_URL "uicustom");
    MusEGlobal::muse->launchBrowser(s);
}


} // namespace MusEGui
