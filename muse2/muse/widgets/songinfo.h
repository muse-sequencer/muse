//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id: songinfo.h,v 1.0.0.0 2010/11/17 01:01:01 ogetbilo Exp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#include "ui_songinfo.h"

class QDialog;


namespace MusEGui {

//---------------------------------------------------------
//   SongInfoWidget
//   Wrapper around Ui::SongInfo
//---------------------------------------------------------

class SongInfoWidget : public QDialog, public Ui::SongInfo
{
      Q_OBJECT

   public:
      SongInfoWidget(QDialog *parent = 0) : QDialog(parent) { setupUi(this); }
};

} // namespace MusEGui
