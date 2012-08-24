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
#include <QSet>
#include <QStringList>

#include "drummap.h"

class QActionGroup;
class QString;
class QMenu;
class QWidget;


namespace MusECore {
class Part;
class Track;


QString pitch2string(int v);
Part* partFromSerialNumber(int serial);
bool any_event_selected(const std::set<Part*>&, bool in_range=false);

bool drummaps_almost_equal(const DrumMap* one, const DrumMap* two, int drummap_size=128);

// drummap_hidden may be NULL.
void write_new_style_drummap(int level, Xml& xml, const char* tagname,
                             DrumMap* drummap, bool* drummap_hidden=NULL, bool full=false);
void read_new_style_drummap(Xml& xml, const char* tagname,
                            DrumMap* drummap, bool* drummap_hidden=NULL);
                            

QSet<Part*> parts_at_tick(unsigned tick);
QSet<Part*> parts_at_tick(unsigned tick, const Track* track);
QSet<Part*> parts_at_tick(unsigned tick, const QSet<Track*>& tracks);

bool parse_range(const QString& str, int* from, int* to); // returns true if successful, false on error

void record_controller_change_and_maybe_send(unsigned tick, int ctrl_num, int val, MidiTrack* mt);
}

namespace MusEGui {
QMenu* populateAddSynth(QWidget* parent);
QActionGroup* populateAddTrack(QMenu* addTrack, bool populateAll=false, bool evenIgnoreDrumPreference=false);
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

