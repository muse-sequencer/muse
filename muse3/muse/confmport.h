//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: confmport.h,v 1.3 2004/01/25 11:20:31 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __CONFMPORT_H__
#define __CONFMPORT_H__

#include <QWidget>
#include <QToolTip>

#include "ui_synthconfigbase.h"
#include "type_defs.h"

// Temporary for testing migration of some port list columns over to the new device list. 
// Make permanent later.
#define _USE_EXTRA_INSTANCE_COLUMNS_

class QTreeWidget;
class QTableWidget;
class QPoint;
class QMenu;
class QAction;

namespace MusECore {
class Xml;
}

namespace MusEGui {
class PopupMenu;

//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

class MPConfig : public QDialog, Ui::SynthConfigBase {
      Q_OBJECT
      
      enum InstanceRoles { DeviceRole = Qt::UserRole, DeviceTypeRole = Qt::UserRole + 1};
      #ifdef _USE_EXTRA_INSTANCE_COLUMNS_
      enum DeviceColumns { DEVCOL_NO = 0, DEVCOL_NAME, DEVCOL_INSTR, DEVCOL_DEF_IN_CHANS, DEVCOL_DEF_OUT_CHANS };
      #else
      enum DeviceColumns { DEVCOL_NO = 0, DEVCOL_GUI, DEVCOL_REC, DEVCOL_PLAY, DEVCOL_INSTR, DEVCOL_NAME,
                           DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_DEF_IN_CHANS, DEVCOL_DEF_OUT_CHANS, DEVCOL_STATE };
      #endif     
      #ifdef _USE_EXTRA_INSTANCE_COLUMNS_
      enum InstanceColumns { INSTCOL_NAME = 0, INSTCOL_TYPE, INSTCOL_REC, INSTCOL_PLAY, INSTCOL_GUI, INSTCOL_INROUTES, INSTCOL_OUTROUTES, INSTCOL_STATE };
      #else
      enum InstanceColumns { INSTCOL_NAME = 0, INSTCOL_TYPE, INSTCOL_STATE };
      #endif     
      
      PopupMenu* defpup;
      int _showAliases; // -1: None. 0: First aliases. 1: Second aliases etc.
      void setWhatsThis(QTableWidgetItem *item, int col);
      void setToolTip(QTableWidgetItem *item, int col);
      void setInstWhatsThis(QTableWidgetItem *item, int col);
      void setInstToolTip(QTableWidgetItem *item, int col);
      void addItem(int row, int col, QTableWidgetItem *item, QTableWidget *table);
      void addInstItem(int row, int col, QTableWidgetItem *item, QTableWidget *table);

   private slots:
      void rbClicked(QTableWidgetItem*);
      void DeviceItemRenamed(QTableWidgetItem*);
      void songChanged(MusECore::SongChangedStruct_t);
      void selectionChanged();
      void deviceSelectionChanged();
      void addJackDeviceClicked();
#ifndef _WIN32
#ifdef ALSA_SUPPORT
      void addAlsaDeviceClicked(bool);
#endif
#endif
      void addInstanceClicked();
      void renameInstanceClicked();
      void removeInstanceClicked();
      //void deviceItemClicked(QTreeWidgetItem*, int);
      void deviceItemClicked(QTableWidgetItem* item);
      void changeDefInputRoutes(QAction* act);
      void changeDefOutputRoutes(QAction* act);
      void apply();
      void okClicked();
      void beforeDeviceContextShow(PopupMenu* menu, QAction* menuAction, QMenu* ctxMenu);
      void deviceContextTriggered(QAction*);
      
   public slots:
      void closeEvent(QCloseEvent*e);

   public:
      MPConfig(QWidget* parent=0);
      ~MPConfig();
      };

} // namespace MusEGui

#endif
