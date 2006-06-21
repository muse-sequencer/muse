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

#ifndef __MASTER_EDIT_H__
#define __MASTER_EDIT_H__

#include "editor.h"
#include "widgets/noteinfo.h"
#include "cobject.h"

class MasterCanvas;
class EditToolBar;

namespace AL {
      class Xml;
      };
using AL::Xml;

namespace Awl {
      class TempoEdit;
      class SigEdit;
      class PosLabel;
      class TempoLabel;
      };
using Awl::TempoEdit;
using Awl::SigEdit;
using Awl::PosLabel;
using Awl::TempoLabel;

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

class MasterEdit : public Editor {
      Q_OBJECT
      Q_PROPERTY(int raster       READ raster     WRITE setRaster)
      Q_PROPERTY(QPoint canvasPos READ canvasPos  WRITE setCanvasPos)
      Q_PROPERTY(int tool         READ tool       WRITE setTool)
      Q_PROPERTY(double xmag      READ xmag       WRITE setXmag)
      Q_PROPERTY(double ymag      READ ymag       WRITE setYmag)

      EditToolBar* tools2;
      QAction* enableMasterAction;
      TempoEdit* curTempo;
      SigEdit* curSig;
      QComboBox* rasterLabel;
      QToolBar* tools;
      PosLabel* cursorPos;
      TempoLabel* tempo;

      int _raster;

      MasterCanvas* canvas() { return (MasterCanvas*)tcanvas; }

   private slots:
      void _setRaster(int);
      void posChanged(int idx, const AL::Pos& pos, bool);
      void setTempo(int);
      void songChanged(int);

   signals:
      void deleted(void*);

   public:
      MasterEdit();
      ~MasterEdit();
      void read(QDomNode);
      void write(Xml&) const;

      int raster() const { return _raster; }
      void setRaster(int val);
      QPoint canvasPos() const;
      void setCanvasPos(const QPoint&);
      double xmag() const;
      void setXmag(double val);
      double ymag() const;
      void setYmag(double val);
      int tool() const;
      void setTool(int);

      static int initRaster, initWidth, initHeight;
      static double initXmag, initYmag;

      static const int INIT_RASTER = 384;
      static const int INIT_WIDTH  = 650;
      static const int INIT_HEIGHT = 450;
      };

#endif

