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
#include "muse.h"
#include "gui.h"

//---------------------------------------------------------
//   shortcut_category
//    These need to be members of ShortcutConfig, otherwise 
//    tr() won't play:
//---------------------------------------------------------

const shortcut_cg ShortcutConfig::shortcut_category[] = {
      { GLOBAL_SHRT, QT_TR_NOOP("Global") },
      { ARRANG_SHRT, QT_TR_NOOP("Arranger") },
      { PROLL_SHRT,  QT_TR_NOOP("Pianoroll") },
      { DEDIT_SHRT,  QT_TR_NOOP("Drumeditor") },
      { LEDIT_SHRT,  QT_TR_NOOP("List editor") },
      { LMEDIT_SHRT, QT_TR_NOOP("List Mastertrack") },
      { WAVE_SHRT,   QT_TR_NOOP("Wave editor") },
      { ALL_SHRT  ,  QT_TR_NOOP("All categories") }
      };

QMap<QString, Shortcut*> shortcuts;

Shortcut MuseApplication::sc[] = {
      Shortcut(
            "start",
            QT_TR_NOOP("Transport: Goto Start"),
            GLOBAL_SHRT,
            0,
            QT_TR_NOOP("Goto Start"),
            QT_TR_NOOP("rewind to start position"),
            ":/xpm/start.xpm"
            ),
      Shortcut(
            "toggle_loop",
            QT_TR_NOOP("Transport: Toggle Loop section"),
            GLOBAL_SHRT, 
            0, // QKeySequence(Qt::Key_Slash),
            QT_TR_NOOP("Loop"),
            QT_TR_NOOP("loop between left mark and right mark"),
            ":/xpm/loop.xpm"
            ),
      Shortcut(
            "play",
            QT_TR_NOOP("Transport: Start playback from current location"),
            GLOBAL_SHRT,
            QKeySequence(Qt::Key_Enter),
            QT_TR_NOOP("Play"),
            QT_TR_NOOP("start sequencer play"),
            ":/xpm/play.xpm"
            ),
      Shortcut(
            "play_toggle",
            QT_TR_NOOP("Transport: Play, Stop, Rewind"),
            GLOBAL_SHRT, 
            Qt::Key_Space
            ),
      Shortcut(
            "stop",
            QT_TR_NOOP("Transport: Stop Playback"),
            GLOBAL_SHRT, 
            Qt::Key_Insert,
            QT_TR_NOOP("Stop"),
            QT_TR_NOOP("stop sequencer"),
            ":/xpm/stop.xpm"
            ),
      Shortcut(
            "goto_left",
            QT_TR_NOOP("Transport: Goto left marker"),
            GLOBAL_SHRT, 
            Qt::Key_End  
            ),
      Shortcut(
            "goto_right",
            QT_TR_NOOP("Transport: Goto right marker"),
            GLOBAL_SHRT, 
            Qt::Key_PageDown
            ),
      Shortcut(
            "toggle_metro",
            QT_TR_NOOP("Transport: Toggle metronome"),
            GLOBAL_SHRT,
            QKeySequence(Qt::Key_C)
            ),
      Shortcut(
            "toggle_rec",
            QT_TR_NOOP("Transport: Toggle Record"),
            GLOBAL_SHRT, 
            Qt::Key_Asterisk,
            QT_TR_NOOP("Record"),
            QT_TR_NOOP("to record press record and then play"),
            ":/xpm/recordOn.svg",
            ":/xpm/recordOff.svg"
            ),
      Shortcut(
            "punchin",
            QT_TR_NOOP("Transport: Punch In"),
            GLOBAL_SHRT, 
            0,
            QT_TR_NOOP("Punchin"),
            QT_TR_NOOP("record starts at left mark"),
            ":/xpm/punchin.xpm"
            ),
      Shortcut(
            "punchout",
            QT_TR_NOOP("Transport: Punch Out"),
            GLOBAL_SHRT, 
            0,
            QT_TR_NOOP("Punchout"),
            QT_TR_NOOP("record stops at right mark"),
            ":/xpm/punchout.xpm"
            ),
      Shortcut(
            "rewind",
            QT_TR_NOOP("Transport: Rewind"),
            GLOBAL_SHRT, 
            0,
            QT_TR_NOOP("rewind"),
            QT_TR_NOOP("rewind current position"),
            ":/xpm/frewind.xpm"
            ),
      Shortcut(
            "forward",
            QT_TR_NOOP("Transport: Forward"),
            GLOBAL_SHRT, 
            0,
            QT_TR_NOOP("forward"),
            QT_TR_NOOP("move current position"),
            ":/xpm/fforward.xpm"
            ),
      Shortcut(
            "panic",
            QT_TR_NOOP("Panic"),
            GLOBAL_SHRT, 
            0,
            QT_TR_NOOP("Panic"),
            QT_TR_NOOP("send note off to all midi channels"),
            ":/xpm/panic.xpm"
            ),
      Shortcut(
            "undo",
            QT_TR_NOOP("Edit: Undo"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_Z,
            QT_TR_NOOP("undo"),
            QT_TR_NOOP("undo last change to song"),
            ":/xpm/undo.xpm"
            ),
      Shortcut(
            "redo",
            QT_TR_NOOP("Edit: Redo"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_Y,
            QT_TR_NOOP("redo"),
            QT_TR_NOOP("redo last undo"),
            ":/xpm/redo.xpm"
            ),
      Shortcut(
            "cut",
            QT_TR_NOOP("Edit: Cut" ),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_X,
            QT_TR_NOOP("Cut"),
            QT_TR_NOOP("Cut"),
            ":/xpm/editcut.xpm"
            ),
      Shortcut(
            "copy",
            QT_TR_NOOP("Edit: Copy"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_C,
            QT_TR_NOOP("Copy"),
            QT_TR_NOOP("Copy"),
            ":/xpm/editcopy.xpm"
            ),
      Shortcut(
            "paste",
            QT_TR_NOOP("Edit: Paste,"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_V,
            QT_TR_NOOP("Paste"),
            QT_TR_NOOP("Paste"),
            ":/xpm/editpaste.xpm"
            ),
      Shortcut(
            "delete",
            QT_TR_NOOP("Edit: Delete" ),
            INVIS_SHRT, 
            Qt::Key_Delete,
            QT_TR_NOOP("Delete"),
            QT_TR_NOOP("Delete"),
            ":/xpm/delete.png"
            ),
      Shortcut(
            "open_project",
            QT_TR_NOOP("Open"),
            ARRANG_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_O,
            QT_TR_NOOP("open project"),
            QT_TR_NOOP("Click this button to select a new project\n"
               "You can also select the <b>Open command</b> from the Project menu."),
            ":/xpm/fileopen.png"
            ),
      Shortcut(
            "save_project",
            QT_TR_NOOP("Save"),
            ARRANG_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_S,
            QT_TR_NOOP("save project"),
            QT_TR_NOOP("Click this button to save the project you are editing.\n"
               "You can also select the Save command from the Project menu."),
            ":/xpm/filesave.png"
            ),
      Shortcut(
            "open_recent",
            QT_TR_NOOP("File: Open recent file"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_1
            ),
      Shortcut(
            "import_midi",
            QT_TR_NOOP("File: Import midi file"),
            ARRANG_SHRT,
            0
            ),
      Shortcut(
            "export_midi",
            QT_TR_NOOP("File: Export midi file"),
            ARRANG_SHRT, 
            0
         ),
      Shortcut(
            "import_audio",
            QT_TR_NOOP("File: Import audio file"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "quit",
            QT_TR_NOOP("File: Quit MusE"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_Q
            ),
      Shortcut(
            "select_parts_on_track",
            QT_TR_NOOP("Edit: Select parts on track"),
            ARRANG_SHRT, 
            Qt::CTRL+ Qt::ALT + Qt::Key_P,
            QT_TR_NOOP("All &Parts on Track"),
            QT_TR_NOOP("All &Parts on Track"),
            ":/xpm/select_all_parts_on_track.xpm"
            ),
      Shortcut(
            "open_pianoroll",
            QT_TR_NOOP("Open Pianoroll"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_E,
            QT_TR_NOOP("Pianoroll"),
            QT_TR_NOOP("Start Pianoroll Editor"),
            ":/xpm/piano.xpm"
            ),
      Shortcut(
            "open_miditracker",
            QT_TR_NOOP("Open MidiTracker"),
            ARRANG_SHRT, 
            0,
            QT_TR_NOOP("MidiTracker"),
            QT_TR_NOOP("Start Midi Tracker"),
            ":/xpm/piano.xpm"
            ),
      Shortcut(
            "open_drumedit",
            QT_TR_NOOP("Open drumeditor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_D
            ),
      Shortcut(
            "open_listedit",
            QT_TR_NOOP("Open listeditor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_L
            ),
      Shortcut(
            "open_graph_master",
            QT_TR_NOOP("Open graphical mastertrack editor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_M
            ),
      Shortcut(
            "open_list_master",
            QT_TR_NOOP("Open list mastertrack editor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::SHIFT + Qt::Key_M
            ),
      Shortcut(
            "add_midi_track",
            QT_TR_NOOP("Add midi track"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_J
            ),
      Shortcut(
            "add_drum_track",
            QT_TR_NOOP("Add drum track"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "add_wave_track",
            QT_TR_NOOP("Add wave track"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "add_audio_output",
            QT_TR_NOOP("Add audio output"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "add_audio_group",
            QT_TR_NOOP("Add audio group"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "add_audio_input",
            QT_TR_NOOP("Add audio input"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "global_cut",
            QT_TR_NOOP("Structure: Global cut"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "global_insert",
            QT_TR_NOOP("Structure: Global insert"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "global_split",
            QT_TR_NOOP("Structure: Global split"),
            ARRANG_SHRT,  
            0
            ),
      Shortcut(
            "copy_range",
            QT_TR_NOOP("Structure: Copy range"),
            ARRANG_SHRT,    
            0
            ),
      Shortcut(
            "cut_events",
            QT_TR_NOOP("Structure: Cut events"),
            ARRANG_SHRT,    
            0
            ),
      Shortcut(
            "toggle_mixer1",
            QT_TR_NOOP("View: Open mixer 1 window"),
            ARRANG_SHRT, 
            Qt::Key_F10,
            QT_TR_NOOP("Mixer1"),
            QT_TR_NOOP("Show Mixer 1"),
            ":/xpm/view_mixer.xpm"
            ),
      Shortcut(
            "toggle_mixer2",
            QT_TR_NOOP("View: Open mixer 2 window"),
            ARRANG_SHRT, 
            0,
            QT_TR_NOOP("Mixer2"),
            QT_TR_NOOP("Show Mixer 2"),
            ":/xpm/view_mixer.xpm"
            ),
      Shortcut(
            "toggle_transport",
            QT_TR_NOOP("View: Toggle transport window"),
            ARRANG_SHRT, 
            Qt::Key_F11,
            QT_TR_NOOP("Transport"),
            QT_TR_NOOP("Show Transport Window"),
            ":/xpm/view_transport_window.xpm"
            ),
      Shortcut(
            "toggle_bigtime",
            QT_TR_NOOP("View: Toggle bigtime window"),
            ARRANG_SHRT,   
            Qt::Key_F12,
            QT_TR_NOOP("BigTime"),
            QT_TR_NOOP("Show BigTime Window"),
            ":/xpm/view_bigtime_window.xpm"
            ),
      Shortcut(
            "marker_window",
            QT_TR_NOOP("View: Open marker window"),
            ARRANG_SHRT,
            Qt::Key_F9,
            QT_TR_NOOP("Marker"),
            QT_TR_NOOP("Show Marker List"),
            ":/xpm/view_marker.xpm"
            ),
      Shortcut(
            "follow_jump",
            QT_TR_NOOP("Settings: Follow song by page"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "follow_no",
            QT_TR_NOOP("Settings: Follow song off"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "follow_continuous",
            QT_TR_NOOP("Settings: Follow song continuous"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "configure_shortcuts",
            QT_TR_NOOP("Settings: Configure shortcuts"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "configure_metronome",
            QT_TR_NOOP("Settings: Configure metronome"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "configure_midi_sync",
            QT_TR_NOOP("Settings: Midi sync configuration"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "configure_midi_file",
            QT_TR_NOOP("Settings: Midi file export configuration"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "configure_midi_ports",
            QT_TR_NOOP("Settings: Midi ports / Soft Synth"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "configure_audio_ports",
            QT_TR_NOOP("Settings: Audio subsystem configuration"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "midi_edit_instruments",
            QT_TR_NOOP("Midi: Edit midi instruments"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "midi_remote_control",
            QT_TR_NOOP("Midi: Midi remote control"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "midi_reset",
            QT_TR_NOOP("Midi: Reset midi"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "midi_init",
            QT_TR_NOOP("Midi: Init midi"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "midi_local_off",
            QT_TR_NOOP("Midi: Midi local off"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "audio_bounce_to_track",
            QT_TR_NOOP("Audio: Bounce audio to track"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "audio_bounce_to_file",
            QT_TR_NOOP("Audio: Bounce audio to file"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "audio_restart",
            QT_TR_NOOP("Audio: Restart audio"),
            ARRANG_SHRT, 
            0
            ),
      Shortcut(
            "open_help",
            QT_TR_NOOP("Help: Open Manual"),
            ARRANG_SHRT,        
            Qt::Key_F1
            ),
      Shortcut(
            "toggle_whatsthis",
            QT_TR_NOOP("Help: Toggle whatsthis mode"),
            ARRANG_SHRT, 
            Qt::SHIFT + Qt::Key_F1
            ),
      Shortcut(
            "edit_selected_part",
            QT_TR_NOOP("Edit: Edit selected part"),
            ARRANG_SHRT, 
            Qt::Key_Return
            ),
      Shortcut(
            "sel_part_above",
            QT_TR_NOOP("Edit: Select nearest part on track above"),
            ARRANG_SHRT, 
            Qt::Key_Up
            ),
      Shortcut(
            "sel_part_above_add",
            QT_TR_NOOP("Edit: Add nearest part on track above"),
            ARRANG_SHRT, 
            Qt::SHIFT + Qt::Key_Up
            ),
      Shortcut(
            "sel_part_below",
            QT_TR_NOOP("Edit: Select nearest part on track below"),
            ARRANG_SHRT, 
            Qt::Key_Down
            ),
      Shortcut(
            "sel_part_below_add",
            QT_TR_NOOP("Edit: Add nearest part on track below"),
            ARRANG_SHRT, 
            Qt::SHIFT + Qt::Key_Down
            ),
      Shortcut(
            "midi_transpose",
            QT_TR_NOOP("Midi: Transpose"),
            ARRANG_SHRT + PROLL_SHRT, 
            0,
            QT_TR_NOOP("Transpose")
            ),
      Shortcut(
            "sel_all",
            QT_TR_NOOP("Edit: Select all"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_A,
            QT_TR_NOOP("Select All"),
            QT_TR_NOOP("Select All"),
            ":/xpm/select_all.xpm"
            ),
      Shortcut(
            "sel_none",
            QT_TR_NOOP("Edit: Select none"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::SHIFT + Qt::Key_A,
            QT_TR_NOOP("Deselect All"),
            QT_TR_NOOP("Deselect All"),
            ":/xpm/select_deselect_all.xpm"
            ),
      Shortcut(
            "sel_inv",
            QT_TR_NOOP("Edit: Invert selection"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,  
            Qt::CTRL + Qt::Key_I,
            QT_TR_NOOP("Invert Selection"),
            QT_TR_NOOP("Invert Selection"),
            ":/xpm/select_invert_selection.xpm"
            ),
      Shortcut(
            "sel_ins_loc",
            QT_TR_NOOP("Edit: Select events/parts inside locators"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,  
            0,
            QT_TR_NOOP("Select Inside Loop"),
            QT_TR_NOOP("Select Inside Loop"),
            ":/xpm/select_inside_loop.xpm"
            ),
      Shortcut(
            "sel_out_loc",
            QT_TR_NOOP("Edit: Select events/parts outside locators"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            0,
            QT_TR_NOOP("Select Outside Loop"),
            QT_TR_NOOP("Select Outside Loop"),
            ":/xpm/select_outside_loop.xpm"
            ),
      Shortcut(
            "sel_left",
            QT_TR_NOOP("Edit: Select nearest part/event to the left"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_Left
            ),
      Shortcut(
            "sel_left_add",
            QT_TR_NOOP("Edit: Add nearest part/event to the left to selection"),
            PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_Left + Qt::SHIFT
            ),
      Shortcut(
            "sel_right",
            QT_TR_NOOP("Edit: Select nearest part/event to the left"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,
            Qt::Key_Right
            ),
      Shortcut(
            "sel_right_add",
            QT_TR_NOOP("Edit: Add nearest part/event to the right to selection"),
            PROLL_SHRT + DEDIT_SHRT,
            Qt::Key_Right + Qt::SHIFT
            ),
      Shortcut(
            "loc_to_sel",
            QT_TR_NOOP("Edit: Set locators to selection"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::ALT + Qt::Key_P
            ),
      Shortcut(
            "sel_inc_pitch",
            QT_TR_NOOP("Edit: Increase pitch"),
            PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_Up
            ),
      Shortcut(
            "sel_dec_pitch",
            QT_TR_NOOP("Edit: Decrease pitch"),
            PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_Down
            ),
      Shortcut(
            "midi_fixed_len",
            QT_TR_NOOP("Edit: Set fixed length on midi events"),
            DEDIT_SHRT, 
            Qt::ALT + Qt::Key_L,
            QT_TR_NOOP("Set fixed length")
            ),
      Shortcut(
            "midi_over_quant",
            QT_TR_NOOP("Quantize: Over Quantize"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Over Quantize")
            ),
      Shortcut(
            "midi_quant_noteon",
            QT_TR_NOOP("Quantize: Note On Quantize"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Note On Quantize")
            ),
      Shortcut(
            "midi_quant_noteoff",
            QT_TR_NOOP("Quantize: Note On/Off Quantize"),
            PROLL_SHRT,
            0,
            QT_TR_NOOP("Note On/Off Quantize")
            ),
      Shortcut(
            "midi_quant_iterative",
            QT_TR_NOOP("Quantize: Iterative Quantize"),
            PROLL_SHRT,
            0,
            QT_TR_NOOP("Iterative Quantize")
            ),
      Shortcut(
            "config_quant",
            QT_TR_NOOP("Quantize: Configure quant"),
            PROLL_SHRT, 
            Qt::CTRL + Qt::ALT + Qt::Key_Q
            ),
      Shortcut(
            "midi_mod_gate_time",
            QT_TR_NOOP("Quantize: Modify Gate Time"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Modify Gate Time")
            ),
      Shortcut(
            "midi_mod_velo",
            QT_TR_NOOP("Quantize: Modify Velocity"),
            PROLL_SHRT,  
            0,
            QT_TR_NOOP("Modify Velocity")
            ),
      Shortcut(
            "midi_crescendo",
            QT_TR_NOOP("Edit: Crescendo"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Crescendo")
            ),
      Shortcut(
            "midi_thin_out",
            QT_TR_NOOP("Edit: Thin Out"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Thin Out")
            ),
      Shortcut(
            "midi_erase_event",
            QT_TR_NOOP("Edit: Erase Event"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Erase Event")
            ),
      Shortcut(
            "midi_note_shift",
            QT_TR_NOOP("Edit: Note Shift"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Note Shift")
            ),
      Shortcut(
            "midi_move_clock",
            QT_TR_NOOP("Edit: Move Clock"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Move Clock")
            ),
      Shortcut(
            "midi_copy_measure",
            QT_TR_NOOP("Edit: Copy Measure"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Copy Measure")
            ),
      Shortcut(
            "midi_erase_measure",
            QT_TR_NOOP("Edit: Erase Measure"),
            PROLL_SHRT,
            0,
            QT_TR_NOOP("Erase Measure")
            ),
      Shortcut(
            "midi_delete_measure",
            QT_TR_NOOP("Edit: Delete Measure"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Delete Measure")
            ),
      Shortcut(
            "midi_create_measure",
            QT_TR_NOOP("Edit: Create Measure"),
            PROLL_SHRT, 
            0,
            QT_TR_NOOP("Create Measure")
            ),
      Shortcut(
            "change_event_color",
            QT_TR_NOOP("Edit: Change event color"),
            PROLL_SHRT, 
            Qt::Key_E
            ),
      Shortcut(
            "pointer",
            QT_TR_NOOP("Tool: Pointer"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_A,
            QT_TR_NOOP("Pointer"),
            QT_TR_NOOP("select Pointer Tool:\n"
                  "with the pointer tool you can:\n"
                  "  select parts\n"
                  "  move parts\n"
                  "  copy parts"),
            ":/xpm/pointer.xpm"
            ),
      Shortcut(
            "pencil",
            QT_TR_NOOP("Tool: Pencil"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_D,
            QT_TR_NOOP("Pencil"),
            QT_TR_NOOP("select Pencil Tool:\n"
                  "with the pencil tool you can:\n"
                  "  create new parts\n"
                  "  modify length of parts"),
            ":/xpm/pencil.xpm"
            ),
      Shortcut(
            "eraser",
            QT_TR_NOOP("Tool: Eraser"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_R,
            QT_TR_NOOP("Eraser"),
            QT_TR_NOOP("select Delete Tool:\n"
                  "with the delete tool you can delete parts"),
            ":/xpm/delete.xpm"
            ),
      Shortcut(
            "scissor",
            QT_TR_NOOP("Tool: Scissor"),
            ARRANG_SHRT, 
            Qt::Key_S,
            QT_TR_NOOP("Scissor"),
            QT_TR_NOOP("select Cut Tool:\n"
                  "with the cut tool you can split a part"),
            ":/xpm/cut.xpm"
            ),
      Shortcut(
            "glue",
            QT_TR_NOOP("Tool: Glue"),
            ARRANG_SHRT, 
            Qt::Key_G,
            QT_TR_NOOP("Glue"),
            QT_TR_NOOP("select Glue Tool:\n"
                  "with the glue tool you can glue two parts"),
            ":/xpm/glue.xpm"
            ),
      Shortcut(
            "quantize",
            QT_TR_NOOP("Tool: Quantize"),
            ARRANG_SHRT, 
            Qt::Key_G,
            QT_TR_NOOP("Quantize"),
            QT_TR_NOOP("select Quantize Tool:\n"
                  "insert display quantize event"),
            ":/xpm/quant.xpm"
            ),
      Shortcut(
            "draw",
            QT_TR_NOOP("Tool: Line Draw"),
            PROLL_SHRT + DEDIT_SHRT, 
            0,
            QT_TR_NOOP("Draw"),
            QT_TR_NOOP("select Drawing Tool"),
            ":/xpm/draw.xpm"
            ),
      Shortcut(
            "mute_parts",
            QT_TR_NOOP("Tool: Mute Parts"),
            ARRANG_SHRT, 
            0,
            QT_TR_NOOP("Mute"),
            QT_TR_NOOP("select Muting Tool:\n"
                  "click on part to mute/unmute"),
            ":/xpm/editmute.xpm"
            ),

      Shortcut(
            "curpos_increase",
            QT_TR_NOOP("Transport: Increase current position"),
            GLOBAL_SHRT, 
            Qt::Key_Plus
            ),
      Shortcut(
            "curpos_decrease",
            QT_TR_NOOP("Transport: Decrease current position"),
            GLOBAL_SHRT,  
            Qt::Key_Minus
            ),
      Shortcut(
            "midi_quant_1",
            QT_TR_NOOP("Quantize: Set quantize to 1/1 note"),
            PROLL_SHRT, 
            Qt::Key_1
            ),
      Shortcut(
            "midi_quant_2",
            QT_TR_NOOP("Quantize: Set quantize to 1/2 note"),
            PROLL_SHRT, 
            Qt::Key_2
            ),
      Shortcut(
            "midi_quant_3",
            QT_TR_NOOP("Quantize: Set quantize to 1/4 note"),
            PROLL_SHRT, 
            Qt::Key_3
            ),
      Shortcut(
            "midi_quant_4",
            QT_TR_NOOP("Quantize: Set quantize to 1/8 note"),
            PROLL_SHRT, 
            Qt::Key_4
            ),
      Shortcut(
            "midi_quant_5",
            QT_TR_NOOP("Quantize: Set quantize to 1/16 note"),
            PROLL_SHRT, 
            Qt::Key_5
            ),
      Shortcut(
            "midi_quant_6",
            QT_TR_NOOP("Quantize: Set quantize to 1/32 note"),
            PROLL_SHRT, 
            Qt::Key_6
            ),
      Shortcut(
            "midi_quant_7",
            QT_TR_NOOP("Quantize: Set quantize to 1/64 note"),
            PROLL_SHRT, 
            Qt::Key_7
            ),
      Shortcut(
            "midi_quant_triol",
            QT_TR_NOOP("Quantize: Toggle triol quantization"),
            PROLL_SHRT, 
            Qt::Key_T
            ),
      Shortcut(
            "midi_quant_punct",
            QT_TR_NOOP("Quantize: Toggle punctuation quantization"),
            PROLL_SHRT, 
            Qt::Key_Period
            ),
      Shortcut(
            "midi_quant_punct2",
            QT_TR_NOOP("Quantize: Toggle punctuation quantization (2)"),
            PROLL_SHRT, 
            Qt::Key_Comma
            ),
      Shortcut(
            "midi_insert_at_loc",
            QT_TR_NOOP("Insert"),
            PROLL_SHRT,  
            Qt::SHIFT + Qt::Key_Right
            ),
      Shortcut(
            "lm_ins_tempo",
            QT_TR_NOOP("Insert Tempo"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::Key_T
            ),
      Shortcut(
            "lm_ins_sig",
            QT_TR_NOOP("Insert Signature"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::Key_R
            ),
      Shortcut(
            "lm_edit_beat",
            QT_TR_NOOP("Change Event Position"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::SHIFT + Qt::Key_E
            ),
      Shortcut(
            "lm_edit_val",
            QT_TR_NOOP("Edit Event Value"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::Key_E
            ),
      Shortcut(0, 0, 0, 0 ),
      };

KeyboardMovementIndicator shortcutsKbdMovement; //for keeping track of active part selected by kbd

//---------------------------------------------------------
//   writeShortCuts
//---------------------------------------------------------

void writeShortCuts(Xml& xml)
      {
      xml.stag("shortcuts");
      foreach(Shortcut* s, shortcuts)
            if (s->xml && s->type != INVIS_SHRT) //Avoid nullptr & hardcoded shortcuts
                  xml.tag(s->xml, s->key.toString(QKeySequence::PortableText));
      xml.etag("shortcuts");
      }

//---------------------------------------------------------
//   readShortCuts
//---------------------------------------------------------

void readShortCuts(QDomNode node)
      {
      for (;!node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            Shortcut* s   = shortcuts.value(e.tagName());
            if (s)
                  s->key = QKeySequence::fromString(e.text(), QKeySequence::PortableText);
            else
                  printf("MusE:readShortCuts: unknown tag <%s>\n", e.tagName().toLatin1().data());
            }
      }

//---------------------------------------------------------
//   getAction
//    returns action for shortcut
//---------------------------------------------------------

QAction* getAction(const char* id, QObject* parent)
      {
      Shortcut* s = shortcuts.value(id);
      if (s == 0) {
            printf("interanl error: shortcut <%s> not found\n", id);
            return 0;
            }
      if (s->action == 0 || (s->action->parent() != parent)) {
            s->action = new QAction(s->xml, parent);
            s->action->setData(s->xml);
            s->action->setShortcut(s->key);
            if (s->help) {
                  s->action->setToolTip(s->help);
                  s->action->setWhatsThis(s->help);
                  }
            else {
                  s->action->setToolTip(s->descr);
                  s->action->setWhatsThis(s->descr);
                  }
            if (s->text)
                  s->action->setText(s->text);
            if (s->iconOn) {
                  QIcon icon;
                  icon.addFile(s->iconOn,  ICON_SIZE, QIcon::Normal, QIcon::On);
                  if (s->iconOff)
                        icon.addFile(s->iconOff, ICON_SIZE, QIcon::Normal, QIcon::Off);
                  s->action->setIcon(icon);
                  }
            }
      else
            printf("action <%s> already initialized\n", s->xml);
      return s->action;
      }

