//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mixdowndialog.h,v 1.1.1.1 2003/10/27 18:54:28 wschweer Exp $
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

#ifndef __MIXDOWNDIALOG_H__
#define __MIXDOWNDIALOG_H__

#include "ui_mixdowndialogbase.h"

class QWidget;

namespace MusECore {
class SndFile;
extern SndFile* getSndFile(const SndFile* sf, QWidget* parent);
}

namespace MusEGui {

//---------------------------------------------------------
//   MixdownFileDialog
//---------------------------------------------------------

class MixdownFileDialog : public QDialog, public Ui::MixdownFileDialogBase {
    Q_OBJECT
      MusECore::SndFile* sf;

   private slots:
      void fdialog();
      virtual void accept();

   public:
      MixdownFileDialog(const MusECore::SndFile* f, QWidget* parent = 0,
         Qt::WFlags fl = 0);
      MusECore::SndFile* sndFile() { return sf; }
      };

}

#endif

