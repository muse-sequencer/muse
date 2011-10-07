//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.h,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __MRCONFIG_H__
#define __MRCONFIG_H__

#include "ui_mrconfigbase.h"

class QCloseEvent;
class QWidget;

namespace MusEGui {

//---------------------------------------------------------
//   MRConfig
//---------------------------------------------------------

class MRConfig : public QWidget, public Ui::MRConfigBase {
      Q_OBJECT

      virtual void closeEvent(QCloseEvent*);

   signals:
      void hideWindow();

   private slots:
      void setRcEnable(bool);
      void setRcStopNote(int);
      void setRcRecordNote(int);
      void setRcGotoLeftMarkNote(int);
      void setRcPlayNote(int);
      void setRcSteprecNote(int);

   public:
      MRConfig(QWidget* parent=0, Qt::WFlags fl = 0);
      };

} // namespace MusEGui

#endif

