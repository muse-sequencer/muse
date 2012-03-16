//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: icons.h,v 1.11.2.8 2009/11/14 03:37:48 terminator356 Exp $
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

#ifndef ICONS_H
#define ICONS_H

class QPixmap;
class QIcon;

namespace MusEGui {

extern QPixmap* track_commentIcon;
extern QPixmap* pointerIcon;
extern QPixmap* pencilIcon;
extern QPixmap* deleteIcon;
extern QPixmap* punchinIcon;
extern QPixmap* punchoutIcon;
extern QPixmap* punchin1Icon;
extern QPixmap* punchout1Icon;
extern QPixmap* loopIcon;
extern QPixmap* loop1Icon;
extern QPixmap* playIcon;
extern QPixmap* recordIcon;
extern QPixmap* stopIcon;
extern QPixmap* startIcon;
extern QPixmap* fforwardIcon;
extern QPixmap* frewindIcon;
extern QPixmap* dotIcon;
extern QPixmap* dothIcon;
extern QPixmap* dot1Icon;
extern QPixmap* noteIcon;
extern QPixmap* note1Icon;
extern QPixmap* stickIcon;
extern QPixmap* waveIcon;
extern QPixmap* synthIcon;
extern QPixmap* markIcon[3];

extern QPixmap* recEchoIconOn;
extern QPixmap* recEchoIconOff;
extern QPixmap* muteIconOn;
extern QPixmap* muteIconOff;
extern QPixmap* soloIconOn;
extern QPixmap* soloIconOff;

extern QPixmap* cutIcon;
extern QPixmap* steprecIcon;
extern QPixmap* glueIcon;
extern QPixmap* drawIcon;
extern QPixmap* cursorIcon;


extern QPixmap* quantIcon;
extern QPixmap* printIcon;
extern QPixmap* printIconS;
extern QPixmap* openIcon;
extern QPixmap* saveIcon;
extern QPixmap* saveasIcon;
extern QPixmap* openIconS;
extern QPixmap* saveIconS;
extern QPixmap* saveasIconS;
extern QPixmap* archiveIcon;
extern QPixmap* findIcon;
extern QPixmap* masterIcon;
extern QPixmap* filenewIcon;
extern QPixmap* filenewIconS;
extern QPixmap* homeIcon;
extern QPixmap* backIcon;
extern QPixmap* forwardIcon;
extern QPixmap* muteIcon;
extern QPixmap* upIcon;
extern QPixmap* downIcon;
extern QPixmap* boldIcon;
extern QPixmap* italicIcon;
extern QPixmap* underlinedIcon;
extern QPixmap* gvIcon;
extern QPixmap* midiinIcon;
extern QPixmap* sysexIcon;
extern QPixmap* ctrlIcon;
extern QPixmap* metaIcon;
extern QPixmap* pitchIcon;
extern QPixmap* cafterIcon;
extern QPixmap* pafterIcon;
extern QPixmap* flagIcon;
extern QPixmap* flagIconS;
extern QPixmap* lockIcon;
extern QPixmap* tocIcon;
extern QPixmap* exitIconS;

extern QPixmap* undoIcon;
extern QPixmap* redoIcon;
extern QPixmap* undoIconS;
extern QPixmap* redoIconS;

extern QPixmap* speakerIcon;
extern QPixmap* buttondownIcon;
extern QPixmap* configureIcon;

extern QPixmap* editmuteIcon;
extern QPixmap* editmuteSIcon;
extern QPixmap* panicIcon;

extern QIcon* pianoIconSet;
extern QIcon* scoreIconSet;
extern QIcon* editcutIconSet;
extern QIcon* editmuteIconSet;
extern QIcon* editcopyIconSet;
extern QIcon* editpasteIconSet;
extern QIcon* editpaste2TrackIconSet;
extern QIcon* editpasteCloneIconSet;
extern QIcon* editpasteClone2TrackIconSet;

/* Not used
extern QIcon* pianoIcon;
extern QIcon* editcutIcon;
extern QIcon* editcopyIcon;
extern QIcon* editpasteIcon;
extern QIcon* editpasteCloneIcon;
extern QIcon* editpaste2TrackIcon;
extern QIcon* editpasteClone2TrackIcon;
*/

extern QPixmap* exitIcon;
extern QPixmap* exit1Icon;
extern QPixmap* record1_Icon;
extern QPixmap* record_on_Icon;
extern QPixmap* record_off_Icon;
extern QPixmap* newmuteIcon;
extern QPixmap* soloIcon;

extern QPixmap* routesInIcon;
extern QPixmap* routesOutIcon;
extern QPixmap* routesMidiInIcon;
extern QPixmap* routesMidiOutIcon;
extern QPixmap* muteIconOn;
extern QPixmap* muteIconOff;
extern QPixmap* soloIconOn; 
extern QPixmap* soloIconOff;
extern QPixmap* soloblksqIconOn; 
extern QPixmap* soloblksqIconOff;
//extern QIcon* soloIconSet1;
//extern QIcon* soloIconSet2;

extern QPixmap* toggle_small_Icon;
extern QPixmap* redLedIcon;
extern QPixmap* darkRedLedIcon;
extern QPixmap* greendotIcon;
//extern QPixmap* darkgreendotIcon;
extern QPixmap* graydotIcon;
extern QPixmap* bluedotIcon;
extern QPixmap* offIcon;
extern QPixmap* blacksquareIcon;
extern QPixmap* blacksqcheckIcon;

extern QPixmap* mastertrackSIcon;
extern QPixmap* localoffSIcon;
extern QPixmap* miditransformSIcon;
extern QPixmap* midi_plugSIcon;
extern QPixmap* miditransposeSIcon;
extern QPixmap* midiThruOnIcon;
extern QPixmap* midiThruOffIcon;
extern QPixmap* mixerSIcon;
extern QPixmap* mustangSIcon;
extern QPixmap* resetSIcon;
extern QPixmap* track_addIcon;
extern QPixmap* track_deleteIcon;
extern QPixmap* listSIcon;
extern QPixmap* inputpluginSIcon;
extern QPixmap* cliplistSIcon;
extern QPixmap* mixerAudioSIcon;
extern QPixmap* initSIcon;
extern QPixmap* deltaOnIcon;
extern QPixmap* deltaOffIcon;

extern QPixmap* addtrack_addmiditrackIcon;
extern QPixmap* addtrack_audiogroupIcon;
extern QPixmap* addtrack_audioinputIcon;
extern QPixmap* addtrack_audiooutputIcon;
extern QPixmap* addtrack_auxsendIcon;
extern QPixmap* addtrack_drumtrackIcon;
extern QPixmap* addtrack_wavetrackIcon;
extern QPixmap* edit_drummsIcon;
extern QPixmap* edit_listIcon;
extern QPixmap* edit_waveIcon;
extern QPixmap* edit_mastertrackIcon;
extern QPixmap* edit_pianorollIcon;
extern QPixmap* edit_scoreIcon;
extern QPixmap* edit_track_addIcon;
extern QPixmap* edit_track_delIcon;
extern QPixmap* mastertrack_graphicIcon;
extern QPixmap* mastertrack_listIcon;
extern QPixmap* midi_transformIcon;
extern QPixmap* midi_transposeIcon;
extern QPixmap* selectIcon;
extern QPixmap* select_allIcon;
extern QPixmap* select_all_parts_on_trackIcon;
extern QPixmap* select_deselect_allIcon;
extern QPixmap* select_inside_loopIcon;
extern QPixmap* select_invert_selectionIcon;
extern QPixmap* select_outside_loopIcon;

extern QPixmap* audio_bounce_to_fileIcon;
extern QPixmap* audio_bounce_to_trackIcon;
extern QPixmap* audio_restartaudioIcon;
extern QPixmap* automation_clear_dataIcon;
extern QPixmap* automation_mixerIcon;
extern QPixmap* automation_take_snapshotIcon;
extern QPixmap* edit_midiIcon;
extern QPixmap* midi_edit_instrumentIcon;
extern QPixmap* midi_init_instrIcon;
extern QPixmap* midi_inputpluginsIcon;
extern QPixmap* midi_inputplugins_midi_input_filterIcon;
extern QPixmap* midi_inputplugins_midi_input_transformIcon;
extern QPixmap* midi_inputplugins_random_rhythm_generatorIcon;
extern QPixmap* midi_inputplugins_remote_controlIcon;
extern QPixmap* midi_inputplugins_transposeIcon;
extern QPixmap* midi_local_offIcon;
extern QPixmap* midi_reset_instrIcon;
extern QPixmap* settings_appearance_settingsIcon;
extern QPixmap* settings_configureshortcutsIcon;
extern QPixmap* settings_follow_songIcon;
extern QPixmap* settings_globalsettingsIcon;
extern QPixmap* settings_metronomeIcon;
extern QPixmap* settings_midifileexportIcon;
extern QPixmap* settings_midiport_softsynthsIcon;
extern QPixmap* settings_midisyncIcon;
extern QPixmap* view_bigtime_windowIcon;
extern QPixmap* view_cliplistIcon;
extern QPixmap* view_markerIcon;
extern QPixmap* view_mixerIcon;
extern QPixmap* view_transport_windowIcon;

extern QPixmap* monoIcon;
extern QPixmap* stereoIcon;

extern QPixmap* museIcon;
extern QPixmap* aboutMuseImage;
extern QPixmap* museLeftSideLogo;

extern QIcon* globalIcon;
extern QIcon* projectIcon;
extern QIcon* userIcon;

extern QPixmap* sineIcon;
extern QPixmap* sawIcon;

} // namespace MusEGui

#endif

