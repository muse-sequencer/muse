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
#include <QString>
#include <QRect>
#include <QLine>
#include <QValidator>
#include <QUuid>

#include "type_defs.h"

class QActionGroup;
class QMenu;
class QWidget;

namespace MusEGui {
class PopupMenu;
}

namespace MusECore {
class MEvent;
class Track;
class MidiTrack;
class AudioTrack;
class Part;
class PartList;
struct DrumMap;
class Xml;
class CtrlList;

void enumerateJackMidiDevices();
void populateMidiPorts();

QString pitch2string(int v);
int string2pitch(const QString &s);
QValidator::State validatePitch(const QString &s);
void dumpMPEvent(const MEvent* ev);
Part* partFromSerialNumber(const QUuid& serial);
bool any_event_selected(const std::set<const Part*>&, bool in_range=false,
                        RelevantSelectedEvents_t relevant = NotesRelevant);

bool drummaps_almost_equal(const DrumMap* one, const DrumMap* two, int drummap_size=128);

// drummap_hidden may be NULL.
void write_new_style_drummap(int level, Xml& xml, const char* tagname,
                             DrumMap* drummap, bool full=false);
void read_new_style_drummap(Xml& xml, const char* tagname,
                            DrumMap* drummap, bool compatibility=false);
int readDrummapsEntryPatchCollection(Xml& xml);


QSet<Part*> parts_at_tick(unsigned tick);
QSet<Part*> parts_at_tick(unsigned tick, const Track* track);
QSet<Part*> parts_at_tick(unsigned tick, const QSet<Track*>& tracks);

bool parse_range(const QString& str, int* from, int* to); // returns true if successful, false on error

void record_controller_change_and_maybe_send(unsigned tick, int ctrl_num, int val, MidiTrack* mt);
}

namespace MusEGui {

// Lists all used midi ports + devices, plus all empty ports.
QMenu* midiPortsPopup(QWidget* parent = nullptr, int checkPort = -1, bool includeDefaultEntry = false);
// Includes all listed in midiPortsPopup, plus unused devices.
void midiPortsPopupMenu(MusECore::Track* t, int x, int y, bool allClassPorts, 
                        const QWidget* widget = nullptr, bool includeDefaultEntry = false);
//QMenu* populateAddSynth(QWidget* parent);
QActionGroup* populateAddTrack(QMenu* addTrack, bool populateAll=false, bool insert=false, bool addkey=true);
void openSynthGui(MusECore::Track* t);
QStringList localizedStringListFromCharArray(const char** array, const char* context);
QString getFilterExtension(const QString &filter);
QString browseProjectFolder(QWidget* parent = nullptr);
QString projectTitleFromFilename(QString filename);
QString projectPathFromFilename(QString filename);
QString projectExtensionFromFilename(QString filename);
QString getUniqueUntitledName();
int populateMidiCtrlMenu(PopupMenu* menu, MusECore::PartList* part_list, MusECore::Part* cur_part, int curDrumPitch);
QLine clipQLine(int x1, int y1, int x2, int y2, const QRect& rect);
// We need normalizeQRect() because Qt's own QRect normalize makes the rectangle larger,
//  that is NOT what we want! For example Qt normalizes (50, 50, -40, -40) to (9, 9, 42, 42),
//  but we want (10, 10, 40, 40).
QRect normalizeQRect(const QRect& rect);
//void loadQtStyle(const QString&);
void loadTheme(const QString&);
void loadThemeColors(const QString&);
// Call when the theme or stylesheet part of the configuration has changed, to actually switch them.
//void updateThemeAndStyle();
int countSelectedParts();

// A replacement for obsolete Qt5's QDesktopWidget geometry.
QRect getDesktopGeometry();
} 

#endif

