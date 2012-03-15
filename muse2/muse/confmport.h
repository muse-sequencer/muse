//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: confmport.h,v 1.3 2004/01/25 11:20:31 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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
      QMenu* instrPopup;
      MusEGui::PopupMenu* defpup;
      int _showAliases; // -1: None. 0: First aliases. 1: Second aliases etc.
      void setWhatsThis(QTableWidgetItem *item, int col);
      void setToolTip(QTableWidgetItem *item, int col);
      void addItem(int row, int col, QTableWidgetItem *item, QTableWidget *table);

      

   private slots:
      void rbClicked(QTableWidgetItem*);
      void mdevViewItemRenamed(QTableWidgetItem*);
      void songChanged(int);
      void selectionChanged();
      void addInstanceClicked();
      void removeInstanceClicked();
      void changeDefInputRoutes(QAction* act);
      void changeDefOutputRoutes(QAction* act);
   public slots:
      void closeEvent(QCloseEvent*e);

   public:
      MPConfig(QWidget* parent=0);
      ~MPConfig();
      };

} // namespace MusEGui

#endif
