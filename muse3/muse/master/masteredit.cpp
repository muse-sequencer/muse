//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: masteredit.cpp,v 1.4.2.5 2009/07/01 22:14:56 spamatica Exp $
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

#include "masteredit.h"
#include "utils.h"
#include "lcombo.h"
#include "doublelabel.h"
#include "globals.h"
#include "app.h"
#include "gconfig.h"
#include "audio.h"
#include "song.h"
#include "icons.h"

#include <limits.h>

#include <QActionGroup>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QList>
#include <QHeaderView>
#include <QListView>

// Forwards from header:
#include <QCloseEvent>
#include <QToolBar>
#include <QToolButton>
#include "master.h"
#include "mtscale.h"
#include "poslabel.h"
#include "scrollscale.h"
#include "sigscale.h"
#include "tempolabel.h"
#include "tscale.h"
#include "raster_widgets.h"
#include "xml.h"
#include "shortcuts.h"

namespace MusEGui {

int MasterEdit::_rasterInit = 0;

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void MasterEdit::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else if (e->key() == shortcuts[SHRT_TOOL_PENCIL].key)
        tools2->set(MusEGui::PencilTool);
    else if (e->key() == shortcuts[SHRT_TOOL_RUBBER].key)
        tools2->set(MusEGui::RubberTool);
    else if (e->key() == shortcuts[SHRT_TOOL_LINEDRAW].key)
        tools2->set(MusEGui::DrawTool);
    else
        e->ignore();
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MasterEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MasterEdit::songChanged(MusECore::SongChangedStruct_t type)
      {
      if(_isDeleting)  // Ignore while while deleting to prevent crash.
        return;
        
      if (type & SC_DIVISION_CHANGED)
      {
        // The division has changed. The raster table and raster model will have been
        //  cleared and re-filled, so any views on the model will no longer have a
        //  current item and our current raster value will be invalid. They WILL NOT
        //  emit an activated signal. So we must manually select a current item and
        //  raster value here. We could do something fancy to try to keep the current
        //  index - for example stay on quarter note - by taking the ratio of the new
        //  division to old division and apply that to the old raster value and try
        //  to select that index, but the division has already changed.
        // So instead, simply try to select the current raster value. The index in the box may change.
        changeRaster(_raster);

        // Now set a reasonable zoom (mag) range.
        setupHZoomRange();
      }
        
      if (type & SC_SIG) {
            sign->redraw();
            }
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void MasterEdit::configChanged()
      {
      gridOnButton->blockSignals(true);
      gridOnButton->setChecked(MusEGlobal::config.canvasShowGrid);
      gridOnButton->blockSignals(false);

      canvas->redraw();
      }

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

MasterEdit::MasterEdit(QWidget* parent, const char* name)
   : MidiEditor(TopWin::MASTER, _rasterInit, 0, parent, name)
      {
      isMdiWin() ? setWindowTitle(tr("Mastertrack")) : setWindowTitle(tr("MusE: Mastertrack"));

      setFocusPolicy(Qt::NoFocus);

      _canvasXOrigin = 0;
//       _minXMag = -100;
      _minXMag = -500;
      _maxXMag = 2;
      _rasterizerModel->setDisplayFormat(RasterizerModel::FractionFormat);
      _rasterizerModel->setMaxRows(7);
      QList<Rasterizer::Column> rast_cols;
      rast_cols <<
        Rasterizer::TripletColumn <<
        Rasterizer::NormalColumn <<
        Rasterizer::DottedColumn;
      _rasterizerModel->setVisibleColumns(rast_cols);
      // Request to set the raster, but be sure to use the one it chooses,
      //  which may be different than the one requested.
      _rasterInit = _rasterizerModel->checkRaster(_rasterInit);
      _raster = _rasterInit;

      //---------Pulldown Menu----------------------------
      QMenu* settingsMenu = menuBar()->addMenu(tr("&Display"));
      settingsMenu->menuAction()->setStatusTip(tr("Display menu: View-specific display options."));
      settingsMenu->addAction(subwinAction);
//      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      // Toolbars ---------------------------------------------------------

      // NOTICE: Please ensure that any tool bar object names here match the names assigned
      //          to identical or similar toolbars in class MusE or other TopWin classes.
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      addToolBarBreak();

      // Already has an object name.
      editTools = MusEGui::PencilTool | MusEGui::RubberTool| MusEGui::DrawTool;
      tools2 = new MusEGui::EditToolBar(this, editTools);
      addToolBar(tools2);

      QToolBar* info = addToolBar(tr("Info"));
      // Make it appear like a Toolbar1 object.
      info->setObjectName("Pos/Snap/Solo-tools");
      QLabel* label  = new QLabel(tr("Cursor"));
      label->setIndent(2);
      info->addWidget(label);

      cursorPos = new MusEGui::PosLabel(nullptr, "PosLabel");
      cursorPos->setFixedHeight(22);
      cursorPos->setToolTip(tr("Time at cursor position"));
      info->addWidget(cursorPos);
      tempo = new MusEGui::TempoLabel(nullptr, "TempoLabel");
      tempo->setFixedHeight(22);
      tempo->setToolTip(tr("Tempo at cursor position"));
      info->addWidget(tempo);

      gridOnButton = new QToolButton();
      gridOnButton->setIcon(*gridOnSVGIcon);
      gridOnButton->setFocusPolicy(Qt::NoFocus);
      gridOnButton->setCheckable(true);
      gridOnButton->setToolTip(tr("Show grid"));
      gridOnButton->setWhatsThis(tr("Show grid"));
      info->addWidget(gridOnButton);
      connect(gridOnButton, &QToolButton::toggled, [this](bool v) { gridOnChanged(v); } );

      rasterLabel = new RasterLabelCombo(RasterLabelCombo::TableView, _rasterizerModel, this, "RasterLabelCombo");
      rasterLabel->setFocusPolicy(Qt::TabFocus);
      info->addWidget(rasterLabel);

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      int xscale = -20;
      int yscale = -500;
      hscroll   = new MusEGui::ScrollScale(
        (_minXMag * MusEGlobal::config.division) / 384,
        _maxXMag,
        xscale,
        MusEGlobal::song->len(),
        Qt::Horizontal,
        mainw);
      vscroll   = new MusEGui::ScrollScale(-1000, -100, yscale, 120000, Qt::Vertical, mainw);
      vscroll->setRange(30000, 250000);
      time1     = new MusEGui::MTScale(_raster, mainw, xscale);
      sign      = new MusEGui::SigScale(&_raster, mainw, xscale);

      canvas    = new Master(this, mainw, xscale, yscale);

      time2     = new MusEGui::MTScale(_raster, mainw, xscale);
      tscale    = new TScale(mainw, yscale);
      time2->setBarLocator(true);

      canvas->setOrigin(_canvasXOrigin, 0);
      time1->setOrigin(_canvasXOrigin, 0);

      changeRaster(_raster);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      mainGrid->setRowStretch(5, 100);
      mainGrid->setColumnStretch(1, 100);

      mainGrid->addWidget(MusECore::hLine(mainw),  0, 1);
      mainGrid->addWidget(time1,         1, 1);
      mainGrid->addWidget(MusECore::hLine(mainw),  2, 1);
      mainGrid->addWidget(sign,          3, 1);
      mainGrid->addWidget(MusECore::hLine(mainw),  4, 1);
      mainGrid->addWidget(canvas,        5, 1);
      mainGrid->addWidget(tscale,        5, 0);
      mainGrid->addWidget(MusECore::hLine(mainw),  6, 1);
      mainGrid->addWidget(time2,         7, 1);
      mainGrid->addWidget(hscroll,       8, 1);
      mainGrid->addWidget(vscroll, 0, 2, 10, 1);

      canvas->setFocus();

      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));
      connect(MusEGlobal::muse, &MusE::configChanged, tools2, &EditToolBar::configChanged);
      connect(vscroll, SIGNAL(scrollChanged(int)),   canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)), canvas, SLOT(setYMag(int)));

      connect(vscroll, SIGNAL(scrollChanged(int)),   tscale, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)), tscale, SLOT(setYMag(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), time1,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), sign,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), time2,  SLOT(setXPos(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)), time1,  SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), sign,   SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), canvas, SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), time2,  SLOT(setXMag(int)));

      connect(time1,  SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
      connect(time2,  SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));

      connect(tscale, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(canvas, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));

      connect(MusEGlobal::muse, &MusE::configChanged, [this]() { configChanged(); } );

      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
      connect(canvas, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));

      connect(rasterLabel, &RasterLabelCombo::rasterChanged, [this](int raster) { _setRaster(raster); } );

      configChanged();  // set configuration values

      initTopwinState();
      finalizeInit();
      }

//---------------------------------------------------------
//   ~MasterEdit
//---------------------------------------------------------

MasterEdit::~MasterEdit()
      {
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MasterEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "ypos")
                              vscroll->setPos(xml.parseInt());
                        else if (tag == "ymag")
                              vscroll->setMag(xml.parseInt());
                        else
                              xml.unknown("MasterEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "master") {
                              // set raster
                              changeRaster(_raster);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MasterEdit::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "master");
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      xml.intTag(level, "ypos", vscroll->pos());
      MidiEditor::writeStatus(level, xml);
      xml.tag(level, "/master");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void MasterEdit::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readConfiguration(MASTER, xml);
                        else
                              xml.unknown("MasterEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "masteredit")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void MasterEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "masteredit");
      xml.intTag(level, "raster", _rasterInit);
      TopWin::writeConfiguration(MASTER, level, xml);
      xml.tag(level, "/masteredit");
      }

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void MasterEdit::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    canvas->setFocus();
    canvas->activateWindow();
  }
}

//---------------------------------------------------------
//   _setRaster
//---------------------------------------------------------

void MasterEdit::_setRaster(int raster)
      {
      MidiEditor::setRaster(raster);
      _rasterInit = _raster;
      time1->setRaster(_raster);
      time2->setRaster(_raster);
      canvas->redrawGrid();
      for (auto it : ctrlEditList)
          it->redrawCanvas();
      focusCanvas();
      }

//---------------------------------------------------------
//   changeRaster
//---------------------------------------------------------

int MasterEdit::changeRaster(int val)
      {
        const RasterizerModel* rast_mdl = rasterLabel->rasterizerModel();
        MidiEditor::setRaster(rast_mdl->checkRaster(val));
        _rasterInit = _raster;
        time1->setRaster(_raster);
        time2->setRaster(_raster);
        const QModelIndex mdl_idx = rast_mdl->modelIndexOfRaster(_raster);
        if(mdl_idx.isValid())
          rasterLabel->setCurrentModelIndex(mdl_idx);
        else
          fprintf(stderr, "MasterEdit::changeRaster: _raster %d not found in box!\n", _raster);

        canvas->redrawGrid();
        for (auto it : ctrlEditList)
            it->redrawCanvas();
        return _raster;
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void MasterEdit::setTime(unsigned tick)
      {
      if (tick == INT_MAX)
            cursorPos->setEnabled(false);
      else {
            cursorPos->setEnabled(true);
            cursorPos->setValue(tick);
            time1->setPos(3, tick, false);
            time2->setPos(3, tick, false);
            }
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void MasterEdit::setTempo(int val)
      {
      if (val == -1)
            tempo->setEnabled(false);
      else {
            tempo->setEnabled(true);
            tempo->setValue(val);
            }
      }
      
//---------------------------------------------------------
//   setupHZoomRange
//---------------------------------------------------------

void MasterEdit::setupHZoomRange()
{
  const int min = (_minXMag * MusEGlobal::config.division) / 384;
  hscroll->setScaleRange(min, _maxXMag);
}

//---------------------------------------------------------
//   setEditTool
//---------------------------------------------------------

void MasterEdit::setEditTool(int tool)
{
    tools2->set(tool);
}

//---------------------------------------------------------
//   gridOnChanged
//---------------------------------------------------------

void MasterEdit::gridOnChanged(bool v)
{
  MusEGlobal::config.canvasShowGrid = v;
  // We want the simple version, don't set the style or stylesheet yet.
  MusEGlobal::muse->changeConfig(true);
}

} // namespace MusEGui
