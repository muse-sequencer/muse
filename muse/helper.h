//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: helper.h,v 1.1.1.1 2003/10/27 18:52:11 wschweer Exp $
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#ifndef __HELPER_H__
#define __HELPER_H__

#include <set>
#include <QStringList>

class QActionGroup;
class QString;
class QMenu;
class QWidget;
class MidiTrack;

namespace MusECore {
class Part;
QString pitch2string(int v);
Part* partFromSerialNumber(int serial);
bool any_event_selected(const std::set<Part*>&, bool in_range=false);
void record_controller_change_and_maybe_send(unsigned tick, int ctrl_num, int val, MidiTrack* mt);
}

namespace MusEGui {
QMenu* populateAddSynth(QWidget* parent);
QActionGroup* populateAddTrack(QMenu* addTrack, bool populateAll=false);
QStringList localizedStringListFromCharArray(const char** array, const char* context);
QString getFilterExtension(const QString &filter);
QString browseProjectFolder(QWidget* parent = 0);
QString projectTitleFromFilename(QString filename);
QString projectPathFromFilename(QString filename);
QString projectExtensionFromFilename(QString filename);
QString getUniqueUntitledName();
void populateMidiPorts();
} 

#endif

