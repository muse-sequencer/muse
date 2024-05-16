//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/shortcuts.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
// Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
//
// Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
//
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
//
// C++ Implementation: shortcuts
//
// Description:
// Definition of shortcuts used in the application
//

#include "shortcuts.h"
//#include <QTranslator>
#include <QKeySequence>

namespace MusEGui {

ShortCut shortcuts[SHRT_NUM_OF_ELEMENTS];
void defShrt(int shrt, int key, const char* descr, int type, const char* xml)
{
      shortcuts[shrt].key   = key;
      shortcuts[shrt].descr = descr;
      shortcuts[shrt].type  = type;
      shortcuts[shrt].xml   = xml;
}

void initShortCuts()
      {
      // Reserved: To avoid unwanted side-effects (inline editing etc.)
      defShrt(SHRT_RETURN,        Qt::Key_Return, QT_TRANSLATE_NOOP("shortcuts", "Return"), INVIS_SHRT, "return");
      defShrt(SHRT_ESCAPE,        Qt::Key_Escape, QT_TRANSLATE_NOOP("shortcuts", "Escape"), INVIS_SHRT, "escape");

    //Global:
      defShrt(SHRT_PLAY_SONG,     Qt::Key_Enter,  QT_TRANSLATE_NOOP("shortcuts", "Transport: Start playback from current location"), GLOBAL_SHRT, "play");
      defShrt(SHRT_TOGGLE_METRO,  Qt::Key_C,      QT_TRANSLATE_NOOP("shortcuts", "Transport: Toggle metronome"), GLOBAL_SHRT,"toggle_metro");
      defShrt(SHRT_STOP,          Qt::Key_0,      QT_TRANSLATE_NOOP("shortcuts", "Transport: Stop Playback"), GLOBAL_SHRT, "stop");
      defShrt(SHRT_GOTO_START,    Qt::Key_Home,   QT_TRANSLATE_NOOP("shortcuts", "Transport: Goto Start"), GLOBAL_SHRT, "goto_start");
      defShrt(SHRT_GOTO_END,      Qt::Key_End,    QT_TRANSLATE_NOOP("shortcuts", "Transport: Goto End"), GLOBAL_SHRT, "goto_end");

      defShrt(SHRT_PLAY_TOGGLE,   Qt::Key_Space, QT_TRANSLATE_NOOP("shortcuts", "Transport: Play, Stop, Rewind"), GLOBAL_SHRT, "play_toggle");
      defShrt(SHRT_REC_RESTART,   Qt::CTRL + Qt::Key_Space, QT_TRANSLATE_NOOP("shortcuts", "Transport: Restart recording"), GLOBAL_SHRT, "rec_restart");
      defShrt(SHRT_REC_RESTART_MULTI,   Qt::SHIFT + Qt::Key_Space, QT_TRANSLATE_NOOP("shortcuts", "Transport: Restart recording (multi take)"), GLOBAL_SHRT, "rec_restart_multi");
      defShrt(SHRT_GOTO_LEFT,     Qt::Key_PageUp,   QT_TRANSLATE_NOOP("shortcuts", "Transport: Goto left marker")  , GLOBAL_SHRT, "goto_left");
      defShrt(SHRT_GOTO_RIGHT,    Qt::Key_PageDown,  QT_TRANSLATE_NOOP("shortcuts", "Transport: Goto right marker") , GLOBAL_SHRT, "goto_right");
      defShrt(SHRT_TOGGLE_LOOP,   Qt::Key_Slash, QT_TRANSLATE_NOOP("shortcuts", "Transport: Toggle Loop section"), GLOBAL_SHRT, "toggle_loop");
      defShrt(SHRT_START_REC,     Qt::Key_Asterisk, QT_TRANSLATE_NOOP("shortcuts", "Transport: Toggle Record"),    GLOBAL_SHRT, "toggle_rec");
      defShrt(SHRT_REC_ARM_TRACK, Qt::CTRL + Qt::Key_Return, QT_TRANSLATE_NOOP("shortcuts", "Transport: Arm current track"),    GLOBAL_SHRT, "rec_arm_track");
      defShrt(SHRT_REC_CLEAR,     Qt::Key_Backspace, QT_TRANSLATE_NOOP("shortcuts", "Transport: Clear all rec enabled tracks"),    GLOBAL_SHRT, "rec_clear");
      defShrt(SHRT_FULLSCREEN,    Qt::CTRL + Qt::Key_F, QT_TRANSLATE_NOOP("shortcuts", "Toggle fullscreen"), GLOBAL_SHRT, "fullscreen");
      defShrt(SHRT_TABBED_WIN,    Qt::Key_F4, QT_TRANSLATE_NOOP("shortcuts", "Toggle tabbed/floating editor window"), GLOBAL_SHRT, "tabbed_win");
      defShrt(SHRT_TOGGLE_DOCKS,  Qt::Key_F7, QT_TRANSLATE_NOOP("shortcuts", "Toggle docks display"), GLOBAL_SHRT, "docks_display");
      defShrt(SHRT_TOGGLE_REWINDONSTOP,       Qt::CTRL + Qt::SHIFT + Qt::Key_R, QT_TRANSLATE_NOOP("shortcuts", "Toggle rewind on stop"), GLOBAL_SHRT, "rewindonstop");
      defShrt(SHRT_ARRANGER,      Qt::META + Qt::Key_A, QT_TRANSLATE_NOOP("shortcuts", "Show Arranger window"), GLOBAL_SHRT, "arranger");

      defShrt(SHRT_CONFIG_SHORTCUTS, Qt::CTRL + Qt::SHIFT + Qt::Key_F1, QT_TRANSLATE_NOOP("shortcuts", "Settings: Configure shortcuts"), GLOBAL_SHRT, "configure_key_shortcuts");

      defShrt(SHRT_COPY,          Qt::CTRL + Qt::Key_C, QT_TRANSLATE_NOOP("shortcuts", "Edit: Copy"), INVIS_SHRT, "copy");
      defShrt(SHRT_COPY_RANGE,    Qt::CTRL + Qt::SHIFT + Qt::Key_C, QT_TRANSLATE_NOOP("shortcuts", "Edit: Copy in range"), SCORE_SHRT + PROLL_SHRT + DEDIT_SHRT + ARRANG_SHRT, "copy_range");
      defShrt(SHRT_UNDO,          Qt::CTRL + Qt::Key_Z, QT_TRANSLATE_NOOP("shortcuts", "Edit: Undo"), INVIS_SHRT, "undo");
      defShrt(SHRT_REDO,          Qt::CTRL + Qt::SHIFT + Qt::Key_Z, QT_TRANSLATE_NOOP("shortcuts", "Edit: Redo"), INVIS_SHRT, "redo");
      defShrt(SHRT_CUT,           Qt::CTRL + Qt::Key_X, QT_TRANSLATE_NOOP("shortcuts", "Edit: Cut"), INVIS_SHRT, "cut");
      defShrt(SHRT_PASTE,         Qt::CTRL + Qt::Key_V, QT_TRANSLATE_NOOP("shortcuts", "Edit: Paste"), INVIS_SHRT, "paste");
      defShrt(SHRT_PASTE_TO_CUR_PART, Qt::CTRL + Qt::SHIFT + Qt::Key_V, QT_TRANSLATE_NOOP("shortcuts", "Edit: Paste to current part"), PROLL_SHRT + DEDIT_SHRT, "paste_to_current_part");
      defShrt(SHRT_PASTE_TO_TRACK, Qt::CTRL + Qt::SHIFT + Qt::Key_V, QT_TRANSLATE_NOOP("shortcuts", "Edit: Paste to selected track"), ARRANG_SHRT, "paste_to_track");
      defShrt(SHRT_PASTE_CLONE,   Qt::CTRL + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Edit: Paste clone"), ARRANG_SHRT, "paste_clones");
      defShrt(SHRT_PASTE_CLONE_TO_TRACK,  Qt::CTRL + Qt::SHIFT + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Edit: Paste clone to selected track"), ARRANG_SHRT, "paste_clones_to_track");
      defShrt(SHRT_PASTE_DIALOG,  Qt::CTRL + Qt::Key_G, QT_TRANSLATE_NOOP("shortcuts", "Edit: Paste (with dialog)"), SCORE_SHRT + PROLL_SHRT + DEDIT_SHRT + ARRANG_SHRT, "paste_with_dialog");
      defShrt(SHRT_DELETE,        Qt::Key_Delete, QT_TRANSLATE_NOOP("shortcuts", "Edit: Delete"), INVIS_SHRT, "delete");

      //-----------------------------------------------------------
      // Arranger:
      defShrt(SHRT_NEW,           Qt::CTRL + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "File: New project"), ARRANG_SHRT + DEDIT_SHRT, "new_project");
      defShrt(SHRT_NEW_FROM_TEMPLATE, Qt::CTRL + Qt::SHIFT + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "File: New project from template"), ARRANG_SHRT + DEDIT_SHRT, "new_project_from_template");
      defShrt(SHRT_OPEN,          Qt::CTRL + Qt::Key_O, QT_TRANSLATE_NOOP("shortcuts", "File: Open from disk"), ARRANG_SHRT + DEDIT_SHRT, "open_project");
      defShrt(SHRT_SAVE,          Qt::CTRL + Qt::Key_S, QT_TRANSLATE_NOOP("shortcuts", "File: Save project"), ARRANG_SHRT + DEDIT_SHRT, "save_project");
      //-----------------------------------------------------------

      defShrt(SHRT_OPEN_RECENT,           Qt::CTRL + Qt::Key_1, QT_TRANSLATE_NOOP("shortcuts", "File: Open recent file"), ARRANG_SHRT, "open_recent");
      defShrt(SHRT_SAVE_AS,               0 , QT_TRANSLATE_NOOP("shortcuts", "File: Save as"), ARRANG_SHRT, "save_project_as");
      defShrt(SHRT_SAVE_AS_NEW_PROJECT,   0 , QT_TRANSLATE_NOOP("shortcuts", "File: Save as new project"), ARRANG_SHRT, "save_new_project_as");
      defShrt(SHRT_SAVE_REVISION,         Qt::SHIFT + Qt::Key_R, QT_TRANSLATE_NOOP("shortcuts", "File: Save revision"), ARRANG_SHRT, "save_project_new_revision");
      defShrt(SHRT_SAVE_AS_TEMPLATE,      0 , QT_TRANSLATE_NOOP("shortcuts", "File: Save as template"), ARRANG_SHRT, "save_project_template");

      defShrt(SHRT_LOAD_TEMPLATE,         0 , QT_TRANSLATE_NOOP("shortcuts", "File: Load template"), ARRANG_SHRT, "load_template");
//      defShrt(SHRT_CONFIG_PRINTER,        Qt::CTRL + Qt::Key_P, QT_TRANSLATE_NOOP("shortcuts", "Configure printer"), ARRANG_SHRT, "config_printer");
      defShrt(SHRT_IMPORT_MIDI,           0 , QT_TRANSLATE_NOOP("shortcuts", "File: Import midi file"), ARRANG_SHRT, "import_midi");
      defShrt(SHRT_EXPORT_MIDI,           0 , QT_TRANSLATE_NOOP("shortcuts", "File: Export midi file"), ARRANG_SHRT, "export_midi");
      defShrt(SHRT_IMPORT_PART,           0 , QT_TRANSLATE_NOOP("shortcuts", "File: Import midi part"), ARRANG_SHRT, "import_part");
      defShrt(SHRT_IMPORT_AUDIO,          0 , QT_TRANSLATE_NOOP("shortcuts", "File: Import audio file"), ARRANG_SHRT, "import_audio");
      defShrt(SHRT_QUIT,                  0, QT_TRANSLATE_NOOP("shortcuts", "File: Quit MusE"), ARRANG_SHRT, "quit");
//      defShrt(SHRT_DESEL_PARTS,           Qt::CTRL + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Deselect all parts"), ARRANG_SHRT, "deselect_parts");

      defShrt(SHRT_RENAME_PART,           0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Rename part"), ARRANG_SHRT, "rename_part_on_track");
      defShrt(SHRT_SELECT_PRTSTRACK,      Qt::CTRL+ Qt::ALT + Qt::Key_P, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select parts on track"), ARRANG_SHRT, "select_parts_on_track");
      defShrt(SHRT_OPEN_PIANO,            Qt::CTRL + Qt::Key_E, QT_TRANSLATE_NOOP("shortcuts", "Open pianoroll"), ARRANG_SHRT, "open_pianoroll");
      defShrt(SHRT_OPEN_DRUMS,            Qt::CTRL + Qt::Key_D, QT_TRANSLATE_NOOP("shortcuts", "Open drumeditor"), ARRANG_SHRT, "open_drumedit");
      defShrt(SHRT_OPEN_LIST,             Qt::CTRL + Qt::Key_L, QT_TRANSLATE_NOOP("shortcuts", "Open listeditor"), ARRANG_SHRT, "open_listedit");
      defShrt(SHRT_OPEN_WAVE,             Qt::CTRL + Qt::Key_Y, QT_TRANSLATE_NOOP("shortcuts", "Open waveeditor"), ARRANG_SHRT, "open_waveeditor");
      defShrt(SHRT_OPEN_GRAPHIC_MASTER,   Qt::CTRL + Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Open graphical mastertrack editor"), ARRANG_SHRT, "open_graph_master");
      defShrt(SHRT_OPEN_LIST_MASTER,      Qt::CTRL + Qt::SHIFT + Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Open list mastertrack editor"), ARRANG_SHRT, "open_list_master");
      defShrt(SHRT_OPEN_MIDI_TRANSFORM,   Qt::CTRL + Qt::Key_T, QT_TRANSLATE_NOOP("shortcuts", "Open midi transformer"), ARRANG_SHRT, "open_midi_transform");
      defShrt(SHRT_OPEN_PLUGIN_GUI,       Qt::CTRL + Qt::Key_U, QT_TRANSLATE_NOOP("shortcuts", "Open track Synth GUI"), ARRANG_SHRT, "open_plugin_gui");

      defShrt(SHRT_OPEN_PIANO_NEW,        Qt::CTRL + Qt::SHIFT + Qt::Key_E, QT_TRANSLATE_NOOP("shortcuts", "Open pianoroll in new window"), ARRANG_SHRT, "open_pianoroll_new");
      defShrt(SHRT_OPEN_DRUMS_NEW,        Qt::CTRL + Qt::SHIFT + Qt::ALT + Qt::Key_D, QT_TRANSLATE_NOOP("shortcuts", "Open drumeditor in new window"), ARRANG_SHRT, "open_drumedit_new");
      defShrt(SHRT_OPEN_LIST_NEW,         Qt::CTRL + Qt::SHIFT + Qt::Key_L, QT_TRANSLATE_NOOP("shortcuts", "Open listeditor in new window"), ARRANG_SHRT, "open_listedit_new");
      defShrt(SHRT_OPEN_WAVE_NEW,         Qt::CTRL + Qt::SHIFT + Qt::Key_Y, QT_TRANSLATE_NOOP("shortcuts", "Open waveeditor in new window"), ARRANG_SHRT, "open_waveeditor_new");

      defShrt(SHRT_ADD_TRACK_MENU,        Qt::Key_Insert, QT_TRANSLATE_NOOP("shortcuts", "Open add track menu"), ARRANG_SHRT, "add_track_menu");

      defShrt(SHRT_ADD_MIDI_TRACK,        Qt::SHIFT + Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Add midi track"), ARRANG_SHRT, "add_midi_track");
      defShrt(SHRT_ADD_DRUM_TRACK,        Qt::SHIFT + Qt::Key_D, QT_TRANSLATE_NOOP("shortcuts", "Add drum track"), ARRANG_SHRT, "add_drum_track");
      defShrt(SHRT_ADD_WAVE_TRACK,        Qt::SHIFT + Qt::Key_W, QT_TRANSLATE_NOOP("shortcuts", "Add wave track"), ARRANG_SHRT, "add_wave_track");
      defShrt(SHRT_ADD_AUDIO_OUTPUT,      Qt::SHIFT + Qt::Key_O, QT_TRANSLATE_NOOP("shortcuts", "Add audio output track"), ARRANG_SHRT, "add_audio_output");
      defShrt(SHRT_ADD_AUDIO_GROUP,       Qt::SHIFT + Qt::Key_G, QT_TRANSLATE_NOOP("shortcuts", "Add audio group track"), ARRANG_SHRT, "add_audio_group");
      defShrt(SHRT_ADD_AUDIO_INPUT,       Qt::SHIFT + Qt::Key_I, QT_TRANSLATE_NOOP("shortcuts", "Add audio input track"), ARRANG_SHRT, "add_audio_input");
      defShrt(SHRT_ADD_AUDIO_AUX  ,       Qt::SHIFT + Qt::Key_A, QT_TRANSLATE_NOOP("shortcuts", "Add audio aux track"), ARRANG_SHRT, "add_audio_aux");
      defShrt(SHRT_ADD_SYNTH_TRACK,       Qt::SHIFT + Qt::Key_S, QT_TRANSLATE_NOOP("shortcuts", "Add synth track dialog"), ARRANG_SHRT, "add_synth_track");

      defShrt(SHRT_INSERT_MIDI_TRACK,        0, QT_TRANSLATE_NOOP("shortcuts", "Insert midi track"), ARRANG_SHRT, "insert_midi_track");
      defShrt(SHRT_INSERT_DRUM_TRACK,        0, QT_TRANSLATE_NOOP("shortcuts", "Insert drum track"), ARRANG_SHRT, "insert_drum_track");
      defShrt(SHRT_INSERT_WAVE_TRACK,        0, QT_TRANSLATE_NOOP("shortcuts", "Insert wave track"), ARRANG_SHRT, "insert_wave_track");
      defShrt(SHRT_INSERT_AUDIO_OUTPUT,      0, QT_TRANSLATE_NOOP("shortcuts", "Insert audio output track"), ARRANG_SHRT, "insert_audio_output");
      defShrt(SHRT_INSERT_AUDIO_GROUP,       0, QT_TRANSLATE_NOOP("shortcuts", "Insert audio group track"), ARRANG_SHRT, "insert_audio_group");
      defShrt(SHRT_INSERT_AUDIO_INPUT,       0, QT_TRANSLATE_NOOP("shortcuts", "Insert audio input track"), ARRANG_SHRT, "insert_audio_input");
      defShrt(SHRT_INSERT_AUDIO_AUX  ,       0, QT_TRANSLATE_NOOP("shortcuts", "Insert audio aux track"), ARRANG_SHRT, "insert_audio_aux");
      defShrt(SHRT_INSERT_SYNTH_TRACK,       0, QT_TRANSLATE_NOOP("shortcuts", "Insert synth track dialog"), ARRANG_SHRT, "insert_synth_track");

      defShrt(SHRT_GLOBAL_CUT,            0, QT_TRANSLATE_NOOP("shortcuts", "Structure: Global cut"), ARRANG_SHRT, "global_cut");
      defShrt(SHRT_GLOBAL_INSERT,         0, QT_TRANSLATE_NOOP("shortcuts", "Structure: Global insert"), ARRANG_SHRT, "global_insert");
      defShrt(SHRT_GLOBAL_SPLIT,          0, QT_TRANSLATE_NOOP("shortcuts", "Structure: Global split"), ARRANG_SHRT,  "global_split");
      defShrt(SHRT_CUT_EVENTS,            0, QT_TRANSLATE_NOOP("shortcuts", "Structure: Cut events"), ARRANG_SHRT,    "cut_events");
      defShrt(SHRT_OPEN_MIXER,            Qt::Key_F10, QT_TRANSLATE_NOOP("shortcuts", "View: Open mixer #1 window"), ARRANG_SHRT, "toggle_mixer");
      defShrt(SHRT_OPEN_MIXER2,           Qt::CTRL + Qt::Key_F10, QT_TRANSLATE_NOOP("shortcuts", "View: Open mixer #2 window"), ARRANG_SHRT, "toggle_mixer2");
      defShrt(SHRT_OPEN_TRANSPORT,        Qt::Key_F11, QT_TRANSLATE_NOOP("shortcuts", "View: Toggle transport window"), ARRANG_SHRT, "toggle_transport");
      defShrt(SHRT_OPEN_BIGTIME,          Qt::Key_F12, QT_TRANSLATE_NOOP("shortcuts", "View: Toggle bigtime window"), ARRANG_SHRT,   "toggle_bigtime");
      defShrt(SHRT_OPEN_MARKER,           Qt::Key_F9, QT_TRANSLATE_NOOP("shortcuts", "View: Open marker window"), ARRANG_SHRT,   "marker_window");
      defShrt(SHRT_HIDE_MIXER_STRIP,      Qt::Key_F3, QT_TRANSLATE_NOOP("shortcuts", "View: Toggle mixer strip display"), ARRANG_SHRT, "toggle_mixer_strip");

      defShrt(SHRT_FOLLOW_JUMP,           0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Follow song by page"), ARRANG_SHRT, "follow_jump");
      defShrt(SHRT_FOLLOW_NO,             0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Follow song off"), ARRANG_SHRT, "follow_no");
      defShrt(SHRT_FOLLOW_CONTINUOUS,     0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Follow song continuous"), ARRANG_SHRT, "follow_continuous");

      defShrt(SHRT_GLOBAL_CONFIG,         0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Global configuration"), ARRANG_SHRT, "configure_global");
      defShrt(SHRT_CONFIG_METRONOME,      0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Configure metronome"), ARRANG_SHRT, "configure_metronome");
      defShrt(SHRT_CONFIG_MIDISYNC,       0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Midi sync configuration"), ARRANG_SHRT, "configure_midi_sync");
      defShrt(SHRT_MIDI_FILE_CONFIG,      0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Midi file import/export configuration"), ARRANG_SHRT, "configure_midi_file");
      defShrt(SHRT_APPEARANCE_SETTINGS,   0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Appearance settings"), ARRANG_SHRT, "configure_appearance_settings");
      defShrt(SHRT_CONFIG_MIDI_PORTS,     0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Midi ports / Soft synths"), ARRANG_SHRT, "configure_midi_ports");
      defShrt(SHRT_CONFIG_AUDIO_PORTS,    0, QT_TRANSLATE_NOOP("shortcuts", "Settings: Audio subsystem configuration"), ARRANG_SHRT, "configure_audio_ports");
      //defShrt(SHRT_SAVE_GLOBAL_CONFIG,    0, QT_TRANSLATE_NOOP("shortcuts", "Save global configuration"), ARRANG_SHRT, "configure_save_global");

      defShrt(SHRT_MIDI_EDIT_INSTRUMENTS, 0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Edit midi instruments"), ARRANG_SHRT, "midi_edit_instruments");
      defShrt(SHRT_MIDI_INPUT_TRANSFORM,  0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Open midi input transform"), ARRANG_SHRT, "midi_open_input_transform");
      defShrt(SHRT_MIDI_INPUT_FILTER,     0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Open midi input filter"), ARRANG_SHRT, "midi_open_input_filter");
      defShrt(SHRT_MIDI_INPUT_TRANSPOSE,  0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Midi input transpose"), ARRANG_SHRT, "midi_open_input_transpose");
      defShrt(SHRT_MIDI_REMOTE_CONTROL,   0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Midi remote control"), ARRANG_SHRT, "midi_remote_control");
#ifdef BUILD_EXPERIMENTAL
      defShrt(SHRT_RANDOM_RHYTHM_GENERATOR,0,QT_TRANSLATE_NOOP("shortcuts", "Midi: Random rhythm generator"), ARRANG_SHRT, "midi_random_rhythm_generator");
#endif
      defShrt(SHRT_MIDI_RESET,            0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Reset midi"), ARRANG_SHRT, "midi_reset");
      defShrt(SHRT_MIDI_INIT,             0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Init midi"), ARRANG_SHRT, "midi_init");
      defShrt(SHRT_MIDI_LOCAL_OFF,        0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Midi local off"), ARRANG_SHRT, "midi_local_off");

      defShrt(SHRT_AUDIO_BOUNCE_TO_TRACK, 0, QT_TRANSLATE_NOOP("shortcuts", "Audio: Bounce audio to track"), ARRANG_SHRT, "audio_bounce_to_track");
      defShrt(SHRT_AUDIO_BOUNCE_TO_FILE,  0, QT_TRANSLATE_NOOP("shortcuts", "Audio: Bounce audio to file"), ARRANG_SHRT, "audio_bounce_to_file");
      defShrt(SHRT_AUDIO_RESTART,         0, QT_TRANSLATE_NOOP("shortcuts", "Audio: Restart audio"), ARRANG_SHRT, "audio_restart");

// REMOVE Tim. automation. Removed.
// Deprecated. MusEGlobal::automation is now fixed TRUE
//  for now until we decide what to do with it.
//       defShrt(SHRT_MIXER_AUTOMATION,      0, QT_TRANSLATE_NOOP("shortcuts", "Automation: Mixer automation"), ARRANG_SHRT, "mixer_automation");
      defShrt(SHRT_MIXER_SNAPSHOT,        0, QT_TRANSLATE_NOOP("shortcuts", "Automation: Take mixer snapshot"), ARRANG_SHRT, "mixer_snapshot");
      defShrt(SHRT_MIXER_AUTOMATION_CLEAR,0, QT_TRANSLATE_NOOP("shortcuts", "Automation: Clear mixer automation"), ARRANG_SHRT, "mixer_automation_clear");

//      defShrt(SHRT_OPEN_CLIPS,            0, QT_TRANSLATE_NOOP("shortcuts", "View audio clips"), ARRANG_SHRT,                  "view_audio_clips");
      defShrt(SHRT_OPEN_HELP,             Qt::Key_F1, QT_TRANSLATE_NOOP("shortcuts", "Help: Open Manual"), ARRANG_SHRT,        "open_help");
      defShrt(SHRT_START_WHATSTHIS,       Qt::SHIFT + Qt::Key_F1, QT_TRANSLATE_NOOP("shortcuts", "Help: Toggle whatsthis mode"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "toggle_whatsthis");

      defShrt(SHRT_EDIT_PART,             Qt::Key_E, QT_TRANSLATE_NOOP("shortcuts", "Edit: Edit selected part"), ARRANG_SHRT, "edit_selected_part");
      defShrt(SHRT_SEL_ABOVE,             Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select nearest part on track above"), ARRANG_SHRT, "sel_part_above");
      defShrt(SHRT_SEL_ABOVE_ADD,         Qt::SHIFT + Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Edit: Add nearest part on track above"), ARRANG_SHRT, "sel_part_above_add");
      defShrt(SHRT_SEL_BELOW,             Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select nearest part on track below"), ARRANG_SHRT, "sel_part_below");
      defShrt(SHRT_SEL_BELOW_ADD,         Qt::SHIFT + Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Edit: Add nearest part on track below"), ARRANG_SHRT, "sel_part_below_add");

      defShrt(SHRT_INSERTMEAS,            Qt::CTRL + Qt::SHIFT + Qt::Key_O, QT_TRANSLATE_NOOP("shortcuts", "Edit: Insert empty measure"), ARRANG_SHRT, "insert_measure");
      defShrt(SHRT_DUPLICATE_TRACK,       Qt::CTRL + Qt::SHIFT + Qt::Key_D, QT_TRANSLATE_NOOP("shortcuts", "Edit: Duplicate track(s)"), ARRANG_SHRT, "duplicate_track");
      defShrt(SHRT_DELETE_TRACK,          Qt::CTRL + Qt::Key_Delete, QT_TRANSLATE_NOOP("shortcuts", "Edit: Delete track(s)"), ARRANG_SHRT, "delete_track");
      defShrt(SHRT_MOVEUP_TRACK,          Qt::CTRL + Qt::SHIFT + Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Edit: Move tracks up"), ARRANG_SHRT, "moveup_track");
      defShrt(SHRT_MOVEDOWN_TRACK,        Qt::CTRL + Qt::SHIFT + Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Edit: Move tracks down"), ARRANG_SHRT, "movedown_track");
      defShrt(SHRT_MOVETOP_TRACK,         Qt::CTRL + Qt::SHIFT + Qt::Key_Home, QT_TRANSLATE_NOOP("shortcuts", "Edit: Move tracks to top"), ARRANG_SHRT, "movetop_track");
      defShrt(SHRT_MOVEBOTTOM_TRACK,      Qt::CTRL + Qt::SHIFT + Qt::Key_End, QT_TRANSLATE_NOOP("shortcuts", "Edit: Move tracks to bottom"), ARRANG_SHRT, "movebottom_track");

      defShrt(SHRT_SEL_TRACK_ABOVE,       Qt::CTRL + Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Select track above"), ARRANG_SHRT, "sel_track_above");
      defShrt(SHRT_SEL_TRACK_BELOW,       Qt::CTRL + Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Select track below"), ARRANG_SHRT, "sel_track_below");
      defShrt(SHRT_EDIT_TRACK_NAME,       Qt::Key_F2, QT_TRANSLATE_NOOP("shortcuts", "Edit selected track name"), ARRANG_SHRT, "edit_track_name");

      defShrt(SHRT_MUTE_CURRENT_TRACKS,   Qt::META + Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Mute all currently selected tracks"), ARRANG_SHRT, "mute_curren_tracks");
      defShrt(SHRT_SOLO_CURRENT_TRACKS,   Qt::META + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Solo all currently selected tracks"), ARRANG_SHRT, "solo_curren_tracks");
      defShrt(SHRT_TOGGLE_TRACK_HEIGHTS,   Qt::META + Qt::Key_H, QT_TRANSLATE_NOOP("shortcuts", "Toggle track height in arranger"), ARRANG_SHRT, "toggle_track_height");
      //-----------------------------------------------------------

      defShrt(SHRT_TRANSPOSE,       0, QT_TRANSLATE_NOOP("shortcuts", "Midi: Transpose"), ARRANG_SHRT + PROLL_SHRT + SCORE_SHRT, "midi_transpose");

      //-----------------------------------------------------------

      defShrt(SHRT_SELECT_ALL,       Qt::CTRL + Qt::Key_A, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select all"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT + SCORE_SHRT, "sel_all");
      defShrt(SHRT_SELECT_NONE,      Qt::CTRL + Qt::SHIFT + Qt::Key_A, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select none"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT + SCORE_SHRT, "sel_none");
      defShrt(SHRT_SELECT_INVERT,    Qt::CTRL + Qt::Key_I, QT_TRANSLATE_NOOP("shortcuts", "Edit: Invert Selection"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT,  "sel_inv");
      defShrt(SHRT_SELECT_ILOOP,     0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select events/parts inside locators"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT,  "sel_ins_loc");
      defShrt(SHRT_SELECT_OLOOP,     0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select events/parts outside locators"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT, "sel_out_loc");
      defShrt(SHRT_SELECT_PREV_PART, Qt::ALT + Qt::Key_Left, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select previous part"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "sel_prv_prt");
      defShrt(SHRT_SELECT_NEXT_PART, Qt::ALT + Qt::Key_Right, QT_TRANSLATE_NOOP("shortcuts", "Edit: Select next part"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "sel_nxt_prt");
      defShrt(SHRT_SEL_LEFT,         Qt::Key_Left,  QT_TRANSLATE_NOOP("shortcuts", "Edit: Select nearest part/event to the left or move cursor"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "sel_left");
      defShrt(SHRT_SEL_LEFT_ADD,     Qt::Key_Left + Qt::SHIFT,  QT_TRANSLATE_NOOP("shortcuts", "Edit: Add nearest part/event to the left to selection"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "sel_left_add");
      defShrt(SHRT_SEL_RIGHT,        Qt::Key_Right,  QT_TRANSLATE_NOOP("shortcuts", "Edit: Select nearest part/event to the right or move cursor"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT,"sel_right");
      defShrt(SHRT_SEL_RIGHT_ADD,    Qt::Key_Right + Qt::SHIFT, QT_TRANSLATE_NOOP("shortcuts", "Edit: Add nearest part/event to the right to selection"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT,"sel_right_add");
      defShrt(SHRT_LOCATORS_TO_SELECTION, Qt::ALT + Qt::Key_P, QT_TRANSLATE_NOOP("shortcuts", "Edit: Set range markers to selection"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "loc_to_sel");
      defShrt(SHRT_INC_PITCH,        Qt::CTRL + Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Edit: Increase pitch"), PROLL_SHRT + DEDIT_SHRT, "sel_inc_pitch");
      defShrt(SHRT_DEC_PITCH,        Qt::CTRL + Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Edit: Decrease pitch"), PROLL_SHRT + DEDIT_SHRT, "sel_dec_pitch");
      defShrt(SHRT_INC_POS,          Qt::CTRL + Qt::Key_Right,  QT_TRANSLATE_NOOP("shortcuts", "Edit: Increase event position"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "sel_inc_pos");
      defShrt(SHRT_DEC_POS,          Qt::CTRL + Qt::Key_Left,   QT_TRANSLATE_NOOP("shortcuts", "Edit: Decrease event position"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "sel_dec_pos");
      defShrt(SHRT_ZOOM_IN,          Qt::CTRL + Qt::Key_PageUp,   QT_TRANSLATE_NOOP("shortcuts", "View: Zoom in"),  PROLL_SHRT + DEDIT_SHRT + ARRANG_SHRT + WAVE_SHRT, "zoom_in");
      defShrt(SHRT_ZOOM_OUT,         Qt::CTRL + Qt::Key_PageDown, QT_TRANSLATE_NOOP("shortcuts", "View: Zoom out"), PROLL_SHRT + DEDIT_SHRT + ARRANG_SHRT + WAVE_SHRT, "zoom_out");
      defShrt(SHRT_GOTO_CPOS,        Qt::Key_I, QT_TRANSLATE_NOOP("shortcuts", "View: Goto Current Position"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "goto_cpos");
      defShrt(SHRT_SCROLL_LEFT,      Qt::Key_H, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll left"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "scroll_left");
      defShrt(SHRT_SCROLL_RIGHT,     Qt::Key_L, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll right"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "scroll_right");
      defShrt(SHRT_SCROLL_LEFT_FAST,  0, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll left fast"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "scroll_left_fast");
      defShrt(SHRT_SCROLL_RIGHT_FAST, 0, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll right fast"), PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "scroll_right_fast");
      defShrt(SHRT_SCROLL_UP,        Qt::Key_O, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll up"), PROLL_SHRT + DEDIT_SHRT, "scroll_up");
      defShrt(SHRT_SCROLL_DOWN,      Qt::Key_K, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll down"), PROLL_SHRT + DEDIT_SHRT, "scroll_down");
      defShrt(SHRT_SCROLL_UP_FAST,   Qt::SHIFT + Qt::Key_O, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll up fast"), PROLL_SHRT + DEDIT_SHRT, "scroll_up_fast");
      defShrt(SHRT_SCROLL_DOWN_FAST, Qt::SHIFT + Qt::Key_K, QT_TRANSLATE_NOOP("shortcuts", "View: Scroll down fast"), PROLL_SHRT + DEDIT_SHRT, "scroll_down_fast");

      defShrt(SHRT_STEP_RECORD,      Qt::CTRL + Qt::Key_R, QT_TRANSLATE_NOOP("shortcuts", "Transport: Step record"), PROLL_SHRT + DEDIT_SHRT, "step_record");
      defShrt(SHRT_MIDI_INPUT,       Qt::CTRL + Qt::Key_U, QT_TRANSLATE_NOOP("shortcuts", "Transport: Midi input"), PROLL_SHRT + DEDIT_SHRT, "midi_input");
      defShrt(SHRT_PLAY_EVENTS,      Qt::CTRL + Qt::Key_L, QT_TRANSLATE_NOOP("shortcuts", "Transport: Play events"), PROLL_SHRT + DEDIT_SHRT, "play_events");
      defShrt(SHRT_INC_VELOCITY,     Qt::ALT + Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Edit: Increase velocity"), PROLL_SHRT + DEDIT_SHRT, "increase_velocity");
      defShrt(SHRT_DEC_VELOCITY,     Qt::ALT + Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Edit: Decrease velocity"), PROLL_SHRT + DEDIT_SHRT, "decrease_velocity");

      //-----------------------------------------------------------
      //Drum:
      //-----------------------------------------------------------

      defShrt(SHRT_FIXED_LEN,   Qt::ALT + Qt::Key_L, QT_TRANSLATE_NOOP("shortcuts", "Edit: Set Fixed Length on Midi Events"), PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT, "midi_fixed_len");

      //-----------------------------------------------------------
      //Pianoroll:
      //-----------------------------------------------------------

      defShrt(SHRT_QUANTIZE,    0, QT_TRANSLATE_NOOP("shortcuts", "Quantize"), PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT, "midi_quant");
      defShrt(SHRT_MODIFY_GATE_TIME, 0, QT_TRANSLATE_NOOP("shortcuts", "Modify Note Length"), PROLL_SHRT + SCORE_SHRT, "midi_mod_gate_time");
      defShrt(SHRT_MODIFY_VELOCITY,  0, QT_TRANSLATE_NOOP("shortcuts", "Modify Velocity"), PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT,  "midi_mod_velo");
      defShrt(SHRT_CRESCENDO,        0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Crescendo"), PROLL_SHRT + DEDIT_SHRT, "midi_crescendo");
      defShrt(SHRT_THIN_OUT,         0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Thin Out"), PROLL_SHRT + DEDIT_SHRT, "midi_thin_out");
      defShrt(SHRT_ERASE_EVENT,      0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Erase Event"), PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT, "midi_erase_event");
      defShrt(SHRT_DELETE_OVERLAPS,  0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Delete Overlaps"), PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT, "midi_delete_overlaps");
      defShrt(SHRT_NOTE_SHIFT,       0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Note Shift"), PROLL_SHRT + DEDIT_SHRT + SCORE_SHRT, "midi_note_shift");
      defShrt(SHRT_MOVE_CLOCK,       0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Move Clock"), PROLL_SHRT + DEDIT_SHRT, "midi_move_clock");
      defShrt(SHRT_COPY_MEASURE,     0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Copy Measure"), PROLL_SHRT + DEDIT_SHRT, "midi_copy_measure");
      defShrt(SHRT_ERASE_MEASURE,    0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Erase Measure"), PROLL_SHRT + DEDIT_SHRT,"midi_erase_measure");
      defShrt(SHRT_DELETE_MEASURE,   0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Delete Measure"), PROLL_SHRT + DEDIT_SHRT, "midi_delete_measure");
      defShrt(SHRT_CREATE_MEASURE,   0, QT_TRANSLATE_NOOP("shortcuts", "Edit: Create Measure"), PROLL_SHRT + DEDIT_SHRT, "midi_create_measure");
      defShrt(SHRT_EVENT_COLOR,      Qt::Key_E, QT_TRANSLATE_NOOP("shortcuts", "Edit: Change Event Color"), PROLL_SHRT + SCORE_SHRT, "change_event_color");
      defShrt(SHRT_MOVE_PLAY_TO_NOTE,    Qt::CTRL + Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Move: Move to selected note"), PROLL_SHRT + DEDIT_SHRT, "move_to_note");

      defShrt(SHRT_USE_LAST_EDITED_EVENT, Qt::SHIFT + Qt::Key_L, QT_TRANSLATE_NOOP("shortcuts", "Edit: Use Last Edited event"), PROLL_SHRT, "use_last_edited_event");

      // Shortcuts for tools
      // global
      defShrt(SHRT_TOOL_POINTER,     Qt::Key_A, QT_TRANSLATE_NOOP("shortcuts", "Tool: Pointer"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT + SCORE_SHRT, "pointer_tool");
      defShrt(SHRT_TOOL_PENCIL,      Qt::Key_D, QT_TRANSLATE_NOOP("shortcuts", "Tool: Pencil"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT + SCORE_SHRT, "pencil_tool");
      defShrt(SHRT_TOOL_RUBBER,      Qt::Key_R, QT_TRANSLATE_NOOP("shortcuts", "Tool: Eraser"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT + SCORE_SHRT, "eraser_tool");
      defShrt(SHRT_TOOL_PAN,         Qt::Key_P, QT_TRANSLATE_NOOP("shortcuts", "Tool: Pan"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "pan_tool");
      defShrt(SHRT_TOOL_ZOOM,        Qt::Key_Z, QT_TRANSLATE_NOOP("shortcuts", "Tool: Zoom"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "zoom_tool");
      // piano roll & drum editor
      defShrt(SHRT_TOOL_LINEDRAW,    Qt::Key_F, QT_TRANSLATE_NOOP("shortcuts", "Tool: Line Draw"), ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "line_draw_tool");

      // drum editor
      defShrt(SHRT_TOOL_CURSOR,      Qt::Key_U, QT_TRANSLATE_NOOP("shortcuts", "Tool: Cursor"), DEDIT_SHRT, "cursor_tool");
      defShrt(SHRT_ADDNOTE_1,    Qt::Key_V, QT_TRANSLATE_NOOP("shortcuts", "Add note velocity 1"), DEDIT_SHRT, "add_note_velocity_1");
      defShrt(SHRT_ADDNOTE_2,    Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Add note velocity 2"), DEDIT_SHRT, "add_note_velocity_2");
      defShrt(SHRT_ADDNOTE_3,    Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Add note velocity 3"), DEDIT_SHRT, "add_note_velocity_3");
      defShrt(SHRT_ADDNOTE_4,    Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Add note velocity 4"), DEDIT_SHRT, "add_note_velocity_4");

      defShrt(SHRT_CURSOR_STEP_UP,    Qt::Key_0, QT_TRANSLATE_NOOP("shortcuts", "Cursor step size: larger"), DEDIT_SHRT, "cursor_step_up");
      defShrt(SHRT_CURSOR_STEP_DOWN,    Qt::Key_9, QT_TRANSLATE_NOOP("shortcuts", "Cursor step size: smaller"), DEDIT_SHRT, "cursor_step_down");
      defShrt(SHRT_INSTRUMENT_STEP_UP,    Qt::Key_Up, QT_TRANSLATE_NOOP("shortcuts", "Instrument/Cursor up"), DEDIT_SHRT, "instrument_up");
      defShrt(SHRT_INSTRUMENT_STEP_DOWN,    Qt::Key_Down, QT_TRANSLATE_NOOP("shortcuts", "Instrument/Cursor down"), DEDIT_SHRT, "instrument_down");

      // wave editor
      defShrt(SHRT_TOOL_RANGE,      Qt::Key_Y, QT_TRANSLATE_NOOP("shortcuts", "Tool: Range"), WAVE_SHRT, "range_tool");
      defShrt(SHRT_TOOL_STRETCH,    Qt::Key_T, QT_TRANSLATE_NOOP("shortcuts", "Tool: Stretch"), WAVE_SHRT, "stretch_tool");
      defShrt(SHRT_TOOL_SAMPLERATE, Qt::Key_Q, QT_TRANSLATE_NOOP("shortcuts", "Tool: Sample rate"), WAVE_SHRT, "samplerate_tool");
      
      // arranger
      defShrt(SHRT_TOOL_SCISSORS,    Qt::Key_S, QT_TRANSLATE_NOOP("shortcuts", "Tool: Scissor"), ARRANG_SHRT, "scissor_tool");
      defShrt(SHRT_TOOL_GLUE,        Qt::Key_G, QT_TRANSLATE_NOOP("shortcuts", "Tool: Glue"), ARRANG_SHRT, "glue_tool");
      defShrt(SHRT_TOOL_MUTE,        Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Tool: Mute"), ARRANG_SHRT, "mute_tool");

      //Increase/decrease current position, is going to be in arranger & drumeditor as well
      // p4.0.10 Editors and arranger handle these by themselves, otherwise global handler will now use them, too.
      defShrt(SHRT_POS_INC,          Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase");
      defShrt(SHRT_POS_DEC,          Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT, "curpos_decrease");
      
      defShrt(SHRT_POS_INC_NOSNAP,   Qt::SHIFT + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position, no snap"), GLOBAL_SHRT, "curpos_increase_nosnap");
      defShrt(SHRT_POS_DEC_NOSNAP,   Qt::SHIFT + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position, no snap"), GLOBAL_SHRT,  "curpos_decrease_nosnap");
      
      /*
      defShrt(SHRT_POS_INC_BAR,      Qt::CTRL + Qt::ALT + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_bar");
      defShrt(SHRT_POS_DEC_BAR,      Qt::CTRL + Qt::ALT + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_bar");
      defShrt(SHRT_POS_INC_BAR_NOSNAP,   Qt::SHIFT + Qt::CTRL + Qt::ALT + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_bar_nosnap");
      defShrt(SHRT_POS_DEC_BAR_NOSNAP,   Qt::SHIFT + Qt::CTRL + Qt::ALT + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_bar_nosnap");
      
      defShrt(SHRT_POS_INC_BEAT,      Qt::ALT + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_beat");
      defShrt(SHRT_POS_DEC_BEAT,      Qt::ALT + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_beat");
      defShrt(SHRT_POS_INC_BEAT_NOSNAP,   Qt::SHIFT + Qt::ALT + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_beat_nosnap");
      defShrt(SHRT_POS_DEC_BEAT_NOSNAP,   Qt::SHIFT + Qt::ALT + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_beat_nosnap");
      
      defShrt(SHRT_POS_INC_TICK,      Qt::CTRL + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_tick");
      defShrt(SHRT_POS_DEC_TICK,      Qt::CTRL + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_tick");
      defShrt(SHRT_POS_INC_TICK_NOSNAP,      Qt::SHIFT + Qt::CTRL + Qt::Key_Plus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_tick");
      defShrt(SHRT_POS_DEC_TICK_NOSNAP,      Qt::SHIFT + Qt::CTRL + Qt::Key_Minus, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_tick");
      
      defShrt(SHRT_POS_INC_FRAME,      Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_frame");
      defShrt(SHRT_POS_DEC_FRAME,      Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_frame");
      
      defShrt(SHRT_POS_INC_SECOND,      Qt::CTRL + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_second");
      defShrt(SHRT_POS_DEC_SECOND,      Qt::CTRL + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_second");
      defShrt(SHRT_POS_INC_SECOND_NOSNAP,   Qt::SHIFT + Qt::CTRL + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_second_nosnap");
      defShrt(SHRT_POS_DEC_SECOND_NOSNAP,   Qt::SHIFT + Qt::CTRL + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_second_nosnap");
      
      defShrt(SHRT_POS_INC_MINUTE,      Qt::ALT + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_minute");
      defShrt(SHRT_POS_DEC_MINUTE,      Qt::ALT + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_minute");
      defShrt(SHRT_POS_INC_MINUTE_NOSNAP,   Qt::SHIFT + Qt::ALT + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Transport: Increase current position"), GLOBAL_SHRT, "curpos_increase_minute_nosnap");
      defShrt(SHRT_POS_DEC_MINUTE_NOSNAP,   Qt::SHIFT + Qt::ALT + Qt::Key_B, QT_TRANSLATE_NOOP("shortcuts", "Transport: Decrease current position"), GLOBAL_SHRT,  "curpos_decrease_minute_nosnap");
      */
      
      
      defShrt(SHRT_SET_QUANT_BAR,    0,         QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to bar"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_bar");
      defShrt(SHRT_SET_QUANT_OFF,    Qt::Key_8, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to off"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_off");
      defShrt(SHRT_SET_QUANT_1,      Qt::Key_1, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/1 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_1");
      defShrt(SHRT_SET_QUANT_2,      Qt::Key_2, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/2 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_2");
      defShrt(SHRT_SET_QUANT_3,      Qt::Key_3, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/4 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_3");
      defShrt(SHRT_SET_QUANT_4,      Qt::Key_4, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/8 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_4");
      defShrt(SHRT_SET_QUANT_5,      Qt::Key_5, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/16 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_5");
      defShrt(SHRT_SET_QUANT_6,      Qt::Key_6, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/32 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_6");
      defShrt(SHRT_SET_QUANT_7,      Qt::Key_7, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Set quantize to 1/64 note"),
              ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_7");

      defShrt(SHRT_TOGGLE_TRIOL,       Qt::Key_T, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Toggle triol quantization"),
              PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_triol");
      defShrt(SHRT_TOGGLE_PUNCT,       Qt::Key_Period, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Toggle punctuation quantization"),
              PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_punct");
      defShrt(SHRT_TOGGLE_PUNCT2,      Qt::Key_Comma, QT_TRANSLATE_NOOP("shortcuts", "Quantize: Toggle punctuation quantization (2)"),
              PROLL_SHRT + DEDIT_SHRT + WAVE_SHRT, "midi_quant_punct2");

      defShrt(SHRT_LOC_PUSH_EVENTS, Qt::Key_Insert, QT_TRANSLATE_NOOP("shortcuts", "Edit: Push events from locator"), PROLL_SHRT + WAVE_SHRT, "loc_push_events");
      defShrt(SHRT_LOC_PULL_EVENTS, Qt::Key_Backspace, QT_TRANSLATE_NOOP("shortcuts", "Edit: Pull events from locator"), PROLL_SHRT + WAVE_SHRT, "loc_pull_events");

      defShrt(SHRT_INCREASE_LEN, Qt::CTRL + Qt::SHIFT + Qt::Key_Right, QT_TRANSLATE_NOOP("shortcuts", "Edit: Increase length"), PROLL_SHRT, "increase_len");
      defShrt(SHRT_DECREASE_LEN, Qt::CTRL + Qt::SHIFT + Qt::Key_Left, QT_TRANSLATE_NOOP("shortcuts", "Edit: Decrease length"), PROLL_SHRT, "decrease_len");

      //-----------------------------------------------------------
      // List edit:
      //-----------------------------------------------------------

      defShrt(SHRT_LE_INS_NOTES, Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Insert Note"), LEDIT_SHRT,  "le_ins_note");
      defShrt(SHRT_LE_INS_SYSEX, Qt::Key_S, QT_TRANSLATE_NOOP("shortcuts", "Insert SysEx"), LEDIT_SHRT, "le_ins_sysex");
      defShrt(SHRT_LE_INS_CTRL,  Qt::Key_C, QT_TRANSLATE_NOOP("shortcuts", "Insert Ctrl"), LEDIT_SHRT,  "le_ins_ctrl");
      defShrt(SHRT_LE_INS_META,  Qt::Key_M, QT_TRANSLATE_NOOP("shortcuts", "Insert Meta"), LEDIT_SHRT, "le_ins_meta");
//      defShrt(SHRT_LE_INS_CHAN_AFTERTOUCH, Qt::CTRL + Qt::Key_A, QT_TRANSLATE_NOOP("shortcuts", "Insert Channel Aftertouch"), LEDIT_SHRT, "le_ins_afttouch");
//      defShrt(SHRT_LE_INS_POLY_AFTERTOUCH, Qt::CTRL + Qt::Key_P, QT_TRANSLATE_NOOP("shortcuts", "Insert Key Aftertouch"), LEDIT_SHRT, "le_ins_poly");

      //-----------------------------------------------------------
      // List masteredit:
      //-----------------------------------------------------------
      defShrt(SHRT_LM_INS_TEMPO,  Qt::Key_T,    QT_TRANSLATE_NOOP("shortcuts", "Insert Tempo"),     LMEDIT_SHRT,  "lm_ins_tempo");
      defShrt(SHRT_LM_INS_SIG  ,  Qt::Key_S,    QT_TRANSLATE_NOOP("shortcuts", "Insert Signature"), LMEDIT_SHRT,  "lm_ins_sig");
      defShrt(SHRT_LM_INS_KEY,    Qt::Key_K,    QT_TRANSLATE_NOOP("shortcuts", "Insert Key"),       LMEDIT_SHRT,  "lm_ins_key");
      defShrt(SHRT_LM_EDIT_BEAT,  Qt::Key_P,    QT_TRANSLATE_NOOP("shortcuts", "Change Event Position"),  LMEDIT_SHRT,  "lm_edit_beat");
      defShrt(SHRT_LM_EDIT_VALUE, Qt::Key_V,    QT_TRANSLATE_NOOP("shortcuts", "Edit Event Value"),       LMEDIT_SHRT,  "lm_edit_val");

      // Since the marker window is always created at start, these are actually global
      //  because they are handled there, and affect the whole app.
      defShrt(SHRT_NEXT_MARKER, Qt::Key_F6, QT_TRANSLATE_NOOP("shortcuts", "Goto Next Marker"), GLOBAL_SHRT,  "me_sel_next");
      defShrt(SHRT_PREV_MARKER, Qt::Key_F5, QT_TRANSLATE_NOOP("shortcuts", "Goto Prev Marker"), GLOBAL_SHRT,  "me_sel_prev");

      defShrt(SHRT_PART_NORMALIZE, Qt::ALT + Qt::SHIFT + Qt::Key_N, QT_TRANSLATE_NOOP("shortcuts", "Normalize"), ARRANG_SHRT, "wave_part_normalize");

      //-----------------------------------------------------------
      // Mixer:
      //-----------------------------------------------------------
      defShrt(SHRT_MIXER_SELECT_STRIP_LEFT, Qt::CTRL + Qt::Key_Home,
        QT_TRANSLATE_NOOP("shortcuts", "Select strip to the left"), MIXER_SHRT, "mixer_select_left");
      defShrt(SHRT_MIXER_SELECT_STRIP_RIGHT, Qt::CTRL + Qt::Key_End,
        QT_TRANSLATE_NOOP("shortcuts", "Select strip to the right"), MIXER_SHRT, "mixer_select_right");
      // TODO Feature marked as todo in AudioMixerApp::selectNextStrip().
      //defShrt(SHRT_MIXER_MULTI_SELECT_STRIP_LEFT, Qt::CTRL + Qt::SHIFT + Qt::Key_Home,
      //  QT_TRANSLATE_NOOP("shortcuts", "Select multiple strips to the left"), MIXER_SHRT, "mixer_multi_select_left");
      //defShrt(SHRT_MIXER_MULTI_SELECT_STRIP_RIGHT, Qt::CTRL + Qt::SHIFT + Qt::Key_End,
      //  QT_TRANSLATE_NOOP("shortcuts", "Select multiple strips to the right"), MIXER_SHRT, "mixer_multi_select_right");

      defShrt(SHRT_MIXER_STRIP_VOL_DOWN, Qt::ALT + Qt::Key_Down,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip volume down"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_vol_dn");
      defShrt(SHRT_MIXER_STRIP_VOL_UP, Qt::ALT + Qt::Key_Up,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip volume up"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_vol_up");
      defShrt(SHRT_MIXER_STRIP_PAN_LEFT, Qt::ALT + Qt::Key_Left,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip pan left"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_pan_left");
      defShrt(SHRT_MIXER_STRIP_PAN_RIGHT, Qt::ALT + Qt::Key_Right,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip pan right"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_pan_right");
      defShrt(SHRT_MIXER_STRIP_VOL_DOWN_PAGE, Qt::ALT + Qt::SHIFT + Qt::Key_Down,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip volume down page"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_vol_dn_pg");
      defShrt(SHRT_MIXER_STRIP_VOL_UP_PAGE, Qt::ALT + Qt::SHIFT + Qt::Key_Up,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip volume up page"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_vol_up_pg");
      defShrt(SHRT_MIXER_STRIP_PAN_LEFT_PAGE, Qt::ALT + Qt::SHIFT + Qt::Key_Left,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip pan left page"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_pan_left_pg");
      defShrt(SHRT_MIXER_STRIP_PAN_RIGHT_PAGE, Qt::ALT + Qt::SHIFT + Qt::Key_Right,
        QT_TRANSLATE_NOOP("shortcuts", "Mixer strip pan right page"), ARRANG_SHRT + MIXER_SHRT, "mixer_strip_pan_right_pg");
      }

 const shortcut_cg shortcut_category[SHRT_NUM_OF_CATEGORIES] = {
      { GLOBAL_SHRT, "Global" },
      { ARRANG_SHRT, "Arranger" },
      { PROLL_SHRT,  "Pianoroll" },
      { DEDIT_SHRT,  "Drumeditor" },
      { LEDIT_SHRT,  "List editor" },
      { LMEDIT_SHRT, "List Mastertrack" },
      { SCORE_SHRT,  "Score editor" },
      { WAVE_SHRT,   "Wave editor" },
      { MIXER_SHRT,  "Mixer" },
      { ALL_SHRT  ,  "All categories" }
 };

int getShrtByTag(const char* xml)
      {
      for (int i=0; i<SHRT_NUM_OF_ELEMENTS; i++) {
            if (shortcuts[i].xml) {
                  if (strcmp(shortcuts[i].xml, xml) == 0)
                        return i;
            }
      }
      return -1;
      }

void writeShortCuts(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "shortcuts");
      for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
            if (shortcuts[i].xml != nullptr && shortcuts[i].type != INVIS_SHRT) //Avoid nullptr & hardcoded shortcuts
                  xml.intTag(level, shortcuts[i].xml, shortcuts[i].key);
            }
      xml.etag(level, "shortcuts");
      }

void readShortCuts(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;

            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::TagStart: {
                        if (tag.length()) {
                              int index = getShrtByTag(tag.toLatin1().constData());
                              if (index == -1) //No such tag found
                                    printf("Config file might be corrupted. Unknown shortcut: %s\n",tag.toLatin1().constData());
                              else {
                                    //printf("Index: %d\n",index);
                                    shortcuts[index].key = xml.parseInt();
                                    //printf("shortcuts[%d].key = %d, %s\n",index, shortcuts[index].key, shortcuts[index].descr);
                                    }
                              }
                        }
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "shortcuts")
                              return;
                  default:
                        break;
                  }
            }
      }

QString shrtToStr(long int shrt)
{
 return QKeySequence(shortcuts[shrt].key).toString();
}

} // namespace MusEGui
