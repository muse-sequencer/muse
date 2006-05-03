//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Description:
//  Definition of shortcuts used in the application
//
//  Copyright (C) 2003 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "shortcuts.h"
#include "widgets/shortcutconfig.h"
#include "al/xml.h"

// These need to be members of ShortcutConfig, otherwise tr() won't play:
const shortcut_cg ShortcutConfig::shortcut_category[] = {
      { GLOBAL_SHRT, QT_TR_NOOP("Global") },
      { ARRANG_SHRT, QT_TR_NOOP("Arranger") },
      { PROLL_SHRT,  QT_TR_NOOP("Pianoroll") },
      { DEDIT_SHRT,  QT_TR_NOOP("Drumeditor") },
      { LEDIT_SHRT,  QT_TR_NOOP("List editor") },
      { LMEDIT_SHRT, QT_TR_NOOP("List Mastertrack") },
//      { SCORE_SHRT,  "Score editor" },
//      { WAVE_SHRT,   "Wave editor" },
      { ALL_SHRT  ,  QT_TR_NOOP("All categories") }
 };

// These need to be members of ShortcutConfig, otherwise tr() won't play:
const char* ShortcutConfig::ls[] = {
      QT_TR_NOOP("Transport: Start playback from current location"), //0
      QT_TR_NOOP("Transport: Toggle metronome"),
      QT_TR_NOOP("Transport: Stop Playback"),
      QT_TR_NOOP("Transport: Play, Stop, Rewind"),
      QT_TR_NOOP("Transport: Goto left marker"),
      QT_TR_NOOP("Transport: Goto right marker"),
      QT_TR_NOOP("Transport: Toggle Loop section"),
      QT_TR_NOOP("Transport: Toggle Record"),
      QT_TR_NOOP("Edit: Copy"),
      QT_TR_NOOP("Edit: Undo"),

      QT_TR_NOOP("Edit: Redo"), //10
      QT_TR_NOOP("Edit: Cut" ),
      QT_TR_NOOP("Edit: Paste,"),
      QT_TR_NOOP("Edit: Delete" ),
      QT_TR_NOOP("File: New project"),
      QT_TR_NOOP("File: Open from disk"),
      QT_TR_NOOP("File: Save project"),
      QT_TR_NOOP("File: Open recent file"),
      QT_TR_NOOP("File: Save as"),
      QT_TR_NOOP("File: Load template"),

      QT_TR_NOOP("File: Import midi file"),//20
      QT_TR_NOOP("File: Export midi file"),
      QT_TR_NOOP("File: Import audio file"),
      QT_TR_NOOP("File: Quit MusE"),
      QT_TR_NOOP("Edit: Select parts on track"),
      QT_TR_NOOP("Open pianoroll"),
      QT_TR_NOOP("Open drumeditor"),
      QT_TR_NOOP("Open listeditor"),
      QT_TR_NOOP("Open graphical mastertrack editor"),
      QT_TR_NOOP("Open list mastertrack editor"),


      QT_TR_NOOP("Open midi transformer"),//30
      QT_TR_NOOP("Add midi track"),
      QT_TR_NOOP("Add drum track"),
      QT_TR_NOOP("Add wave track"),
      QT_TR_NOOP("Add audio output"),
      QT_TR_NOOP("Add audio group"),
      QT_TR_NOOP("Add audio input"),
      QT_TR_NOOP("Add audio aux"),
      QT_TR_NOOP("Structure: Global cut"),
      QT_TR_NOOP("Structure: Global insert"),

      QT_TR_NOOP("Structure: Global split"),//40
      QT_TR_NOOP("Structure: Copy range"),
      QT_TR_NOOP("Structure: Cut events"),
      QT_TR_NOOP("View: Open mixer window"),
      QT_TR_NOOP("View: Toggle transport window"),
      QT_TR_NOOP("View: Toggle bigtime window"),
      QT_TR_NOOP("View: Open marker window"),
      QT_TR_NOOP("Settings: Follow song by page"),
      QT_TR_NOOP("Settings: Follow song off"),
      QT_TR_NOOP("Settings: Follow song continuous"),

      QT_TR_NOOP("Settings: Global configuration"), //50
      QT_TR_NOOP("Settings: Configure shortcuts"),
      QT_TR_NOOP("Settings: Configure metronome"),
      QT_TR_NOOP("Settings: Midi sync configuration"),
      QT_TR_NOOP("Settings: Midi file export configuration"),
      QT_TR_NOOP("Settings: Appearance settings"),
      QT_TR_NOOP("Settings: Midi ports / Soft Synth"),
      QT_TR_NOOP("Settings: Audio subsystem configuration"),
      QT_TR_NOOP("Midi: Edit midi instruments"),
      QT_TR_NOOP("Midi: Open midi input transform"),

      QT_TR_NOOP("Midi: Open midi input filter"), //60
      QT_TR_NOOP("Midi: Midi input transpose"),
      QT_TR_NOOP("Midi: Midi remote control"),
      QT_TR_NOOP("Midi: Random rhythm generator"),
      QT_TR_NOOP("Midi: Reset midi"),
      QT_TR_NOOP("Midi: Init midi"),
      QT_TR_NOOP("Midi: Midi local off"),
      QT_TR_NOOP("Audio: Bounce audio to track"),
      QT_TR_NOOP("Audio: Bounce audio to file"),
      QT_TR_NOOP("Audio: Restart audio"),

      QT_TR_NOOP("Automation: Mixer automation"), //70
      QT_TR_NOOP("Automation: Take mixer snapshot"),
      QT_TR_NOOP("Automation: Clear mixer automation"),
      QT_TR_NOOP("Help: Open Manual"),
      QT_TR_NOOP("Help: Toggle whatsthis mode"),
      QT_TR_NOOP("Edit: Edit selected part"),
      QT_TR_NOOP("Edit: Select nearest part on track above"),
      QT_TR_NOOP("Edit: Add nearest part on track above"),
      QT_TR_NOOP("Edit: Select nearest part on track below"),
      QT_TR_NOOP("Edit: Add nearest part on track below"),

      QT_TR_NOOP("Midi: Transpose"), //80
      QT_TR_NOOP("Edit: Select all"),
      QT_TR_NOOP("Edit: Select none"),
      QT_TR_NOOP("Edit: Invert selection"),
      QT_TR_NOOP("Edit: Select events/parts inside locators"),
      QT_TR_NOOP("Edit: Select events/parts outside locators"),
      QT_TR_NOOP("Edit: Select nearest part/event to the left"),
      QT_TR_NOOP("Edit: Add nearest part/event to the left to selection"),
      QT_TR_NOOP("Edit: Select nearest part/event to the left"),
      QT_TR_NOOP("Edit: Add nearest part/event to the right to selection"),

      QT_TR_NOOP("Edit: Set locators to selection"), //90
      QT_TR_NOOP("Edit: Increase pitch"),
      QT_TR_NOOP("Edit: Decrease pitch"),
      QT_TR_NOOP("Edit: Set fixed length on midi events"),
      QT_TR_NOOP("Quantize: Over Quantize"),
      QT_TR_NOOP("Quantize: Note On Quantize"),
      QT_TR_NOOP("Quantize: Note On/Off Quantize"),
      QT_TR_NOOP("Quantize: Iterative Quantize"),
      QT_TR_NOOP("Quantize: Configure quant"),
      QT_TR_NOOP("Quantize: Modify Gate Time"),

      QT_TR_NOOP("Quantize: Modify Velocity"), //100
      QT_TR_NOOP("Edit: Crescendo"),
      QT_TR_NOOP("Edit: Thin Out"),
      QT_TR_NOOP("Edit: Erase Event"),
      QT_TR_NOOP("Edit: Note Shift"),
      QT_TR_NOOP("Edit: Move Clock"),
      QT_TR_NOOP("Edit: Copy Measure"),
      QT_TR_NOOP("Edit: Erase Measure"),
      QT_TR_NOOP("Edit: Delete Measure"),
      QT_TR_NOOP("Edit: Create Measure"),

      QT_TR_NOOP("Edit: Change event color"), //110
      QT_TR_NOOP("Tool: Pointer"),
      QT_TR_NOOP("Tool: Pencil"),
      QT_TR_NOOP("Tool: Eraser"),
      QT_TR_NOOP("Tool: Line Draw"),
      QT_TR_NOOP("Tool: Scissor"),
      QT_TR_NOOP("Tool: Glue"),
      QT_TR_NOOP("Tool: Mute"),
      QT_TR_NOOP("Transport: Increase current position"),
      QT_TR_NOOP("Transport: Decrease current position"),

      QT_TR_NOOP("Quantize: Set quantize to 1/1 note"), //120
      QT_TR_NOOP("Quantize: Set quantize to 1/2 note"),
      QT_TR_NOOP("Quantize: Set quantize to 1/4 note"),
      QT_TR_NOOP("Quantize: Set quantize to 1/8 note"),
      QT_TR_NOOP("Quantize: Set quantize to 1/16 note"),
      QT_TR_NOOP("Quantize: Set quantize to 1/32 note"),
      QT_TR_NOOP("Quantize: Set quantize to 1/64 note"),
      QT_TR_NOOP("Quantize: Toggle triol quantization"),
      QT_TR_NOOP("Quantize: Toggle punctuation quantization"),
      QT_TR_NOOP("Quantize: Toggle punctuation quantization (2)"),

      QT_TR_NOOP("Edit: Insert at location"), //130
      QT_TR_NOOP("Insert Note"),
      QT_TR_NOOP("Insert SysEx"),
      QT_TR_NOOP("Insert Ctrl"),
      QT_TR_NOOP("Insert Meta"),
      QT_TR_NOOP("Insert Channel Aftertouch"),
      QT_TR_NOOP("Insert Key Aftertouch"),
      QT_TR_NOOP("Edit: Increase event position"),
      QT_TR_NOOP("Edit: Decrease event position"),
      QT_TR_NOOP("Insert Tempo"),

      QT_TR_NOOP("Insert Signature"), //140
      QT_TR_NOOP("Change Event Position"),
      QT_TR_NOOP("Edit Event Value")
      };


shortcut shortcuts[SHRT_NUM_OF_ELEMENTS];
KeyboardMovementIndicator shortcutsKbdMovement; //for keeping track of active part selected by kbd

void defShrt(int shrt, int key, int locstr_id, int type, const char* xml)
{
      shortcuts[shrt].key   = key;
      QString test = ShortcutConfig::Translate(ShortcutConfig::ls[locstr_id]);
      //printf("shrt=%d key=%d locstr_id=%d str=%s\n", shrt, key, locstr_id, test.toLatin1().data());
      shortcuts[shrt].descr = ShortcutConfig::Translate(ShortcutConfig::ls[locstr_id]); //shortcutDescr(shrt); //(QT_TR_NOOP(descr);
      shortcuts[shrt].type  = type;
      shortcuts[shrt].xml   = xml;
}

void initShortCuts()
      {
      //Global:
      defShrt(SHRT_PLAY_SONG,     Qt::Key_Enter, 0, GLOBAL_SHRT, "play");
      defShrt(SHRT_TOGGLE_METRO,  Qt::Key_C,     1, GLOBAL_SHRT,"toggle_metro");
      defShrt(SHRT_STOP,          Qt::Key_Insert,2, GLOBAL_SHRT, "stop");
      defShrt(SHRT_PLAY_TOGGLE,   Qt::Key_Space, 3, GLOBAL_SHRT, "play_toggle");
      defShrt(SHRT_GOTO_LEFT,     Qt::Key_End,   4, GLOBAL_SHRT, "goto_left");
      defShrt(SHRT_GOTO_RIGHT,    Qt::Key_PageDown,  5, GLOBAL_SHRT, "goto_right");
      defShrt(SHRT_TOGGLE_LOOP,   Qt::Key_Slash, 6, GLOBAL_SHRT, "toggle_loop");
      defShrt(SHRT_START_REC,     Qt::Key_Asterisk, 7,    GLOBAL_SHRT, "toggle_rec");
      defShrt(SHRT_COPY,          Qt::CTRL + Qt::Key_C, 8, INVIS_SHRT, "copy");
      defShrt(SHRT_UNDO,          Qt::CTRL + Qt::Key_Z, 9, INVIS_SHRT, "undo");

      defShrt(SHRT_REDO,          Qt::CTRL + Qt::Key_Y, 10, INVIS_SHRT, "redo");
      defShrt(SHRT_CUT,           Qt::CTRL + Qt::Key_X, 11, INVIS_SHRT, "cut");
      defShrt(SHRT_PASTE,         Qt::CTRL + Qt::Key_V, 12, INVIS_SHRT, "paste");
      defShrt(SHRT_DELETE,        Qt::Key_Delete, 13, INVIS_SHRT, "delete");

      //-----------------------------------------------------------
      // Arranger:
      defShrt(SHRT_NEW,           Qt::CTRL + Qt::Key_N, 14, ARRANG_SHRT + DEDIT_SHRT, "new_project");
      defShrt(SHRT_OPEN,          Qt::CTRL + Qt::Key_O, 15, ARRANG_SHRT + DEDIT_SHRT, "open_project");
      defShrt(SHRT_SAVE,          Qt::CTRL + Qt::Key_S, 16, ARRANG_SHRT + DEDIT_SHRT, "save_project");
      //-----------------------------------------------------------

      defShrt(SHRT_OPEN_RECENT,           Qt::CTRL + Qt::Key_1, 17, ARRANG_SHRT, "open_recent");
      defShrt(SHRT_SAVE_AS,               0 , 18, ARRANG_SHRT, "save_project_as");
      defShrt(SHRT_LOAD_TEMPLATE,         0 , 19, ARRANG_SHRT, "load_template");
//      defShrt(SHRT_CONFIG_PRINTER,        Qt::CTRL + Qt::Key_P, "Configure printer", ARRANG_SHRT, "config_printer");

      defShrt(SHRT_IMPORT_MIDI,           0 , 20, ARRANG_SHRT, "import_midi");
      defShrt(SHRT_EXPORT_MIDI,           0 , 21, ARRANG_SHRT, "export_midi");
      defShrt(SHRT_IMPORT_AUDIO,          0 , 22, ARRANG_SHRT, "import_audio");
      defShrt(SHRT_QUIT,                  Qt::CTRL + Qt::Key_Q, 23, ARRANG_SHRT, "quit");
//      defShrt(SHRT_DESEL_PARTS,           Qt::CTRL + Qt::Key_B, "Deselect all parts", ARRANG_SHRT, "deselect_parts");
      defShrt(SHRT_SELECT_PRTSTRACK,      Qt::CTRL+ Qt::ALT + Qt::Key_P, 24, ARRANG_SHRT, "select_parts_on_track");
      defShrt(SHRT_OPEN_PIANO,            Qt::CTRL + Qt::Key_E, 25, ARRANG_SHRT, "open_pianoroll");
      defShrt(SHRT_OPEN_DRUMS,            Qt::CTRL + Qt::Key_D, 26, ARRANG_SHRT, "open_drumedit");
      defShrt(SHRT_OPEN_LIST,             Qt::CTRL + Qt::Key_L, 27, ARRANG_SHRT, "open_listedit");
      defShrt(SHRT_OPEN_GRAPHIC_MASTER,   Qt::CTRL + Qt::Key_M, 28, ARRANG_SHRT, "open_graph_master");
      defShrt(SHRT_OPEN_LIST_MASTER,      Qt::CTRL + Qt::SHIFT + Qt::Key_M, 29, ARRANG_SHRT, "open_list_master");

      defShrt(SHRT_OPEN_MIDI_TRANSFORM,   Qt::CTRL + Qt::Key_T, 30, ARRANG_SHRT, "open_midi_transform");
      defShrt(SHRT_ADD_MIDI_TRACK,        Qt::CTRL + Qt::Key_J, 31, ARRANG_SHRT, "add_midi_track");
      defShrt(SHRT_ADD_DRUM_TRACK,        0, 32, ARRANG_SHRT, "add_drum_track");
      defShrt(SHRT_ADD_WAVE_TRACK,        0, 33, ARRANG_SHRT, "add_wave_track");
      defShrt(SHRT_ADD_AUDIO_OUTPUT,      0, 34, ARRANG_SHRT, "add_audio_output");
      defShrt(SHRT_ADD_AUDIO_GROUP,       0, 35, ARRANG_SHRT, "add_audio_group");
      defShrt(SHRT_ADD_AUDIO_INPUT,       0, 36, ARRANG_SHRT, "add_audio_input");
      defShrt(SHRT_ADD_AUDIO_AUX  ,       0, 37, ARRANG_SHRT, "add_audio_aux");
      defShrt(SHRT_GLOBAL_CUT,            0, 38, ARRANG_SHRT, "global_cut");
      defShrt(SHRT_GLOBAL_INSERT,         0, 39, ARRANG_SHRT, "global_insert");

      defShrt(SHRT_GLOBAL_SPLIT,          0, 40, ARRANG_SHRT,  "global_split");
      defShrt(SHRT_COPY_RANGE,            0, 41, ARRANG_SHRT,    "copy_range");
      defShrt(SHRT_CUT_EVENTS,            0, 42, ARRANG_SHRT,    "cut_events");
      defShrt(SHRT_OPEN_MIXER,            Qt::Key_F10, 43, ARRANG_SHRT, "toggle_mixer");
      defShrt(SHRT_OPEN_TRANSPORT,        Qt::Key_F11, 44, ARRANG_SHRT, "toggle_transport");
      defShrt(SHRT_OPEN_BIGTIME,          Qt::Key_F12, 45, ARRANG_SHRT,   "toggle_bigtime");
      defShrt(SHRT_OPEN_MARKER,           Qt::Key_F9, 46, ARRANG_SHRT,   "marker_window");
      defShrt(SHRT_FOLLOW_JUMP,           0, 47, ARRANG_SHRT, "follow_jump");
      defShrt(SHRT_FOLLOW_NO,             0, 48, ARRANG_SHRT, "follow_no");
      defShrt(SHRT_FOLLOW_CONTINUOUS,     0, 49, ARRANG_SHRT, "follow_continuous");

      defShrt(SHRT_GLOBAL_CONFIG,         0, 50, ARRANG_SHRT, "configure_global");
      defShrt(SHRT_CONFIG_SHORTCUTS,      0, 51, ARRANG_SHRT, "configure_shortcuts");
      defShrt(SHRT_CONFIG_METRONOME,      0, 52, ARRANG_SHRT, "configure_metronome");
      defShrt(SHRT_CONFIG_MIDISYNC,       0, 53, ARRANG_SHRT, "configure_midi_sync");
      defShrt(SHRT_MIDI_FILE_CONFIG,      0, 54, ARRANG_SHRT, "configure_midi_file");
      defShrt(SHRT_APPEARANCE_SETTINGS,   0, 55, ARRANG_SHRT, "configure_appearance_settings");
      defShrt(SHRT_CONFIG_MIDI_PORTS,     0, 56, ARRANG_SHRT, "configure_midi_ports");
      defShrt(SHRT_CONFIG_AUDIO_PORTS,    0, 57, ARRANG_SHRT, "configure_audio_ports");
      defShrt(SHRT_MIDI_EDIT_INSTRUMENTS, 0, 58, ARRANG_SHRT, "midi_edit_instruments");
      defShrt(SHRT_MIDI_INPUT_TRANSFORM,  0, 59, ARRANG_SHRT, "midi_open_input_transform");

      defShrt(SHRT_MIDI_INPUT_FILTER,     0, 60, ARRANG_SHRT, "midi_open_input_filter");
      defShrt(SHRT_MIDI_INPUT_TRANSPOSE,  0, 61, ARRANG_SHRT, "midi_open_input_transpose");
      defShrt(SHRT_MIDI_REMOTE_CONTROL,   0, 62, ARRANG_SHRT, "midi_remote_control");
      defShrt(SHRT_RANDOM_RHYTHM_GENERATOR,0,63, ARRANG_SHRT, "midi_random_rhythm_generator");
      defShrt(SHRT_MIDI_RESET,            0, 64, ARRANG_SHRT, "midi_reset");
      defShrt(SHRT_MIDI_INIT,             0, 65, ARRANG_SHRT, "midi_init");
      defShrt(SHRT_MIDI_LOCAL_OFF,        0, 66, ARRANG_SHRT, "midi_local_off");
      defShrt(SHRT_AUDIO_BOUNCE_TO_TRACK, 0, 67, ARRANG_SHRT, "audio_bounce_to_track");
      defShrt(SHRT_AUDIO_BOUNCE_TO_FILE,  0, 68, ARRANG_SHRT, "audio_bounce_to_file");
      defShrt(SHRT_AUDIO_RESTART,         0, 69, ARRANG_SHRT, "audio_restart");

//      defShrt(SHRT_MIXER_AUTOMATION,      0, "Automation: Mixer automation", ARRANG_SHRT, "mixer_automation");
      defShrt(SHRT_MIXER_SNAPSHOT,        0, 71, ARRANG_SHRT, "mixer_snapshot");
      defShrt(SHRT_MIXER_AUTOMATION_CLEAR,0, 72, ARRANG_SHRT, "mixer_automation_clear");

//      defShrt(SHRT_OPEN_CLIPS,            0, "View audio clips", ARRANG_SHRT,                  "view_audio_clips");
      defShrt(SHRT_OPEN_HELP,             Qt::Key_F1, 73, ARRANG_SHRT,        "open_help");
      defShrt(SHRT_START_WHATSTHIS,       Qt::SHIFT + Qt::Key_F1, 74, ARRANG_SHRT, "toggle_whatsthis");

      defShrt(SHRT_EDIT_PART,             Qt::Key_Return, 75, ARRANG_SHRT, "edit_selected_part");
      defShrt(SHRT_SEL_ABOVE,             Qt::Key_Up, 76, ARRANG_SHRT, "sel_part_above");
      defShrt(SHRT_SEL_ABOVE_ADD,         Qt::SHIFT + Qt::Key_Up, 77, ARRANG_SHRT, "sel_part_above_add");
      defShrt(SHRT_SEL_BELOW,             Qt::Key_Down, 78, ARRANG_SHRT, "sel_part_below");
      defShrt(SHRT_SEL_BELOW_ADD,         Qt::SHIFT + Qt::Key_Down, 79, ARRANG_SHRT, "sel_part_below_add");

      //-----------------------------------------------------------

      defShrt(SHRT_TRANSPOSE,       0, 80, ARRANG_SHRT + PROLL_SHRT, "midi_transpose");

      //-----------------------------------------------------------

      defShrt(SHRT_SELECT_ALL,       Qt::CTRL + Qt::Key_A, 81, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "sel_all");
      defShrt(SHRT_SELECT_NONE,      Qt::CTRL + Qt::SHIFT + Qt::Key_A, 82, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "sel_none");
      defShrt(SHRT_SELECT_INVERT,    Qt::CTRL + Qt::Key_I, 83, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,  "sel_inv");
      defShrt(SHRT_SELECT_ILOOP,     0, 84, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,  "sel_ins_loc");
      defShrt(SHRT_SELECT_OLOOP,     0, 85, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "sel_out_loc");
      defShrt(SHRT_SEL_LEFT,         Qt::Key_Left,  86, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "sel_left");
      defShrt(SHRT_SEL_LEFT_ADD,     Qt::Key_Left + Qt::SHIFT,  87, PROLL_SHRT + DEDIT_SHRT, "sel_left_add");
      defShrt(SHRT_SEL_RIGHT,        Qt::Key_Right,  88, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,"sel_right");
      defShrt(SHRT_SEL_RIGHT_ADD,    Qt::Key_Right + Qt::SHIFT, 89, PROLL_SHRT + DEDIT_SHRT,"sel_right_add");

      defShrt(SHRT_LOCATORS_TO_SELECTION, Qt::ALT + Qt::Key_P, 90, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "loc_to_sel");
      defShrt(SHRT_INC_PITCH,        Qt::CTRL + Qt::Key_Up, 91, PROLL_SHRT + DEDIT_SHRT, "sel_inc_pitch");
      defShrt(SHRT_DEC_PITCH,        Qt::CTRL + Qt::Key_Down, 92, PROLL_SHRT + DEDIT_SHRT, "sel_dec_pitch");

      //-----------------------------------------------------------
      //Drum:
      //-----------------------------------------------------------

      defShrt(SHRT_FIXED_LEN,   Qt::ALT + Qt::Key_L, 93, DEDIT_SHRT, "midi_fixed_len");

      //-----------------------------------------------------------
      //Pianoroll:
      //-----------------------------------------------------------

      defShrt(SHRT_OVER_QUANTIZE,    0, 94, PROLL_SHRT, "midi_over_quant");
      defShrt(SHRT_ON_QUANTIZE,      0, 95, PROLL_SHRT, "midi_quant_noteon");
      defShrt(SHRT_ONOFF_QUANTIZE,   0, 96, PROLL_SHRT,"midi_quant_noteoff");
      defShrt(SHRT_ITERATIVE_QUANTIZE,0,97, PROLL_SHRT,"midi_quant_iterative");
      defShrt(SHRT_CONFIG_QUANT,     Qt::CTRL + Qt::ALT + Qt::Key_Q, 98, PROLL_SHRT, "config_quant");
      defShrt(SHRT_MODIFY_GATE_TIME, 0, 99, PROLL_SHRT, "midi_mod_gate_time");

      defShrt(SHRT_MODIFY_VELOCITY,  0, 100, PROLL_SHRT,  "midi_mod_velo");
      defShrt(SHRT_CRESCENDO,        0, 101, PROLL_SHRT, "midi_crescendo");
      defShrt(SHRT_THIN_OUT,         0, 102, PROLL_SHRT, "midi_thin_out");
      defShrt(SHRT_ERASE_EVENT,      0, 103, PROLL_SHRT, "midi_erase_event");
      defShrt(SHRT_NOTE_SHIFT,       0, 104, PROLL_SHRT, "midi_note_shift");
      defShrt(SHRT_MOVE_CLOCK,       0, 105, PROLL_SHRT, "midi_move_clock");
      defShrt(SHRT_COPY_MEASURE,     0, 106, PROLL_SHRT, "midi_copy_measure");
      defShrt(SHRT_ERASE_MEASURE,    0, 107, PROLL_SHRT,"midi_erase_measure");
      defShrt(SHRT_DELETE_MEASURE,   0, 108, PROLL_SHRT, "midi_delete_measure");
      defShrt(SHRT_CREATE_MEASURE,   0, 109, PROLL_SHRT, "midi_create_measure");

      defShrt(SHRT_EVENT_COLOR,      Qt::Key_E, 110, PROLL_SHRT, "change_event_color");

      // Shortcuts for tools
      // global
      defShrt(SHRT_TOOL_POINTER,     Qt::Key_A, 111, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "pointer_tool");
      defShrt(SHRT_TOOL_PENCIL,      Qt::Key_D, 112, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "pencil_tool");
      defShrt(SHRT_TOOL_RUBBER,      Qt::Key_R, 113, ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, "eraser_tool");
      // piano roll & drum editor
      defShrt(SHRT_TOOL_LINEDRAW,    0, 114, PROLL_SHRT + DEDIT_SHRT, "line_draw_tool");
      // arranger
      defShrt(SHRT_TOOL_SCISSORS,    Qt::Key_S, 115, ARRANG_SHRT, "scissor_tool");
      defShrt(SHRT_TOOL_GLUE,        Qt::Key_G, 116, ARRANG_SHRT, "glue_tool");
      defShrt(SHRT_TOOL_MUTE,        0, 117, ARRANG_SHRT, "mute_tool");

      //Increase/decrease current position, is going to be in arranger & drumeditor as well
      defShrt(SHRT_POS_INC,          Qt::Key_Plus,  118, GLOBAL_SHRT, "curpos_increase");
      defShrt(SHRT_POS_DEC,          Qt::Key_Minus, 119, GLOBAL_SHRT,  "curpos_decrease");

      defShrt(SHRT_SET_QUANT_1,      Qt::Key_1, 120,  PROLL_SHRT, "midi_quant_1");
      defShrt(SHRT_SET_QUANT_2,      Qt::Key_2, 121,  PROLL_SHRT, "midi_quant_2");
      defShrt(SHRT_SET_QUANT_3,      Qt::Key_3, 122,  PROLL_SHRT, "midi_quant_3");
      defShrt(SHRT_SET_QUANT_4,      Qt::Key_4, 123,  PROLL_SHRT, "midi_quant_4");
      defShrt(SHRT_SET_QUANT_5,      Qt::Key_5, 124, PROLL_SHRT, "midi_quant_5");
      defShrt(SHRT_SET_QUANT_6,      Qt::Key_6, 125, PROLL_SHRT, "midi_quant_6");
      defShrt(SHRT_SET_QUANT_7,      Qt::Key_7, 126, PROLL_SHRT, "midi_quant_7");

      defShrt(SHRT_TOGGLE_TRIOL,       Qt::Key_T, 127, PROLL_SHRT, "midi_quant_triol");
      defShrt(SHRT_TOGGLE_PUNCT,       Qt::Key_Period, 128, PROLL_SHRT, "midi_quant_punct");
      defShrt(SHRT_TOGGLE_PUNCT2,      Qt::Key_Comma, 129, PROLL_SHRT, "midi_quant_punct2");

      defShrt(SHRT_INSERT_AT_LOCATION, Qt::SHIFT + Qt::Key_Right, 130, PROLL_SHRT, "midi_insert_at_loc");

      //-----------------------------------------------------------
      // List edit:
      //-----------------------------------------------------------

      defShrt(SHRT_LE_INS_NOTES, Qt::CTRL + Qt::Key_N, 131, LEDIT_SHRT,  "le_ins_note");
      defShrt(SHRT_LE_INS_SYSEX, Qt::CTRL + Qt::Key_S, 132, LEDIT_SHRT, "le_ins_sysex");
      defShrt(SHRT_LE_INS_CTRL,  Qt::CTRL + Qt::Key_T, 133, LEDIT_SHRT,  "le_ins_ctrl");
      defShrt(SHRT_LE_INS_META,                     0, 134, LEDIT_SHRT, "le_ins_meta");
      defShrt(SHRT_LE_INS_CHAN_AFTERTOUCH, Qt::CTRL + Qt::Key_A, 135, LEDIT_SHRT, "le_ins_afttouch");
      defShrt(SHRT_LE_INS_POLY_AFTERTOUCH, Qt::CTRL + Qt::Key_P, 136, LEDIT_SHRT, "le_ins_poly");

      defShrt(SHRT_INC_POS, Qt::CTRL + Qt::Key_Right,  137, PROLL_SHRT, "sel_inc_pos");
      defShrt(SHRT_DEC_POS, Qt::CTRL + Qt::Key_Left,   138, PROLL_SHRT, "sel_dec_pos");

      //-----------------------------------------------------------
      // List masteredit:
      //-----------------------------------------------------------
      defShrt(SHRT_LM_INS_TEMPO,  Qt::CTRL + Qt::Key_T,            139,     LMEDIT_SHRT,  "lm_ins_tempo");
      defShrt(SHRT_LM_INS_SIG  ,  Qt::CTRL + Qt::Key_R,            140, LMEDIT_SHRT,  "lm_ins_sig");
      defShrt(SHRT_LM_EDIT_BEAT,  Qt::CTRL + Qt::SHIFT+ Qt::Key_E, 141,  LMEDIT_SHRT,  "lm_edit_beat");
      defShrt(SHRT_LM_EDIT_VALUE, Qt::CTRL + Qt::Key_E,            142,       LMEDIT_SHRT,  "lm_edit_val");
      }


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

void writeShortCuts(Xml& xml)
      {
      xml.tag("shortcuts");
      for (int i=0; i < SHRT_NUM_OF_ELEMENTS; i++) {
            if (shortcuts[i].xml != NULL && shortcuts[i].type != INVIS_SHRT) //Avoid nullptr & hardcoded shortcuts
                  xml.intTag(shortcuts[i].xml, shortcuts[i].key);
            }
      xml.etag("shortcuts");
      }

//---------------------------------------------------------
//   readShortCuts
//---------------------------------------------------------

void readShortCuts(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            int index = getShrtByTag(e.tagName().toLatin1().data());
            if (index != -1) {
                  shortcuts[index].key = e.text().toInt();
                  //printf("shortcuts[%d].key = %d, %s\n",index, shortcuts[index].key, shortcuts[index].descr);
                  }
            else
                  printf("MusE:readShortCuts: unknown tag %s\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      }

