//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.h,v 1.2 2004/01/31 17:31:49 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#ifndef __ROUTEDIALOG_H__
#define __ROUTEDIALOG_H__

#include "ui_routedialogbase.h"

class QCloseEvent;
class QDialog;

namespace MusEGui {

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

class RouteDialog : public QDialog, public Ui::RouteDialogBase {
      Q_OBJECT

      virtual void closeEvent(QCloseEvent*);
      void routingChanged();

   private slots:
      void routeSelectionChanged();
      void removeRoute();
      void addRoute();
      void srcSelectionChanged();
      void dstSelectionChanged();
      void songChanged(int);

   signals:
      void closed();

   public:
      RouteDialog(QWidget* parent=0);
      };


} // namespace MusEGui

#endif

