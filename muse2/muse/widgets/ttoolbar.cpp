//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ttoolbar.cpp,v 1.1.1.1 2003/10/27 18:54:46 wschweer Exp $
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

#include <QAction>
#include "globals.h"

//---------------------------------------------------------
//   syncChanged
//---------------------------------------------------------

void syncChanged(bool flag)
      {
      MusEGlobal::startAction->setEnabled(!flag);
      MusEGlobal::forwardAction->setEnabled(!flag);
      MusEGlobal::rewindAction->setEnabled(!flag);
      MusEGlobal::stopAction->setEnabled(!flag);
      MusEGlobal::playAction->setEnabled(!flag);
      }


