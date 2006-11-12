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

Shortcut shortcuts[SHRT_NUM_OF_ELEMENTS] = {
      {  
            "play",
            QT_TR_NOOP("Transport: Start playback from current location"),
            GLOBAL_SHRT,
            Qt::Key_Enter, 
            },
      {  
            "toggle_metro",
            QT_TR_NOOP("Transport: Toggle metronome"),
            GLOBAL_SHRT,
            Qt::Key_C,     
            },
      {  
            "stop",
            QT_TR_NOOP("Transport: Stop Playback"),
            GLOBAL_SHRT, 
            Qt::Key_Insert, 
            },
      {  
            "play_toggle",
            QT_TR_NOOP("Transport: Play, Stop, Rewind"),
            GLOBAL_SHRT, 
            Qt::Key_Space, 
            },
      {  
            "goto_left",
            QT_TR_NOOP("Transport: Goto left marker"),
            GLOBAL_SHRT, 
            Qt::Key_End,   
            },
      {  
            "goto_right",
            QT_TR_NOOP("Transport: Goto right marker"),
            GLOBAL_SHRT, 
            Qt::Key_PageDown,  
            },
      {  
            "toggle_loop",
            QT_TR_NOOP("Transport: Toggle Loop section"),
            GLOBAL_SHRT, 
            Qt::Key_Slash, 
            },
      {  
            "toggle_rec",
            QT_TR_NOOP("Transport: Toggle Record"),
            GLOBAL_SHRT, 
            Qt::Key_Asterisk, 
            },
      {  
            "copy",
            QT_TR_NOOP("Edit: Copy"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_C, 
            },
      {  
            "undo",
            QT_TR_NOOP("Edit: Undo"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_Z,
            },
      {  
            "redo",
            QT_TR_NOOP("Edit: Redo"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_Y, 
            },
      {  
            "cut",
            QT_TR_NOOP("Edit: Cut" ),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_X, 
            },
      {  
            "paste",
            QT_TR_NOOP("Edit: Paste,"),
            INVIS_SHRT, 
            Qt::CTRL + Qt::Key_V, 
            },
      {  
            "delete",
            QT_TR_NOOP("Edit: Delete" ),
            INVIS_SHRT, 
            Qt::Key_Delete, 
            },
      {  
            "open_project",
            QT_TR_NOOP("File: Open from disk"),
            ARRANG_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_O, 
            },
      {  
            "save_project",
            QT_TR_NOOP("File: Save project"),
            ARRANG_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_S, 
            },
      {  
            "open_recent",
            QT_TR_NOOP("File: Open recent file"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_1, 
            },
      {  
            "import_midi",
            QT_TR_NOOP("File: Import midi file"),
            ARRANG_SHRT, 
            0 , 
            },
      {  
            "export_midi",
            QT_TR_NOOP("File: Export midi file"),
            ARRANG_SHRT, 
            0 , 
         },
      {  
            "import_audio",
            QT_TR_NOOP("File: Import audio file"),
            ARRANG_SHRT, 
            0 , 
            },
      {  
            "quit",
            QT_TR_NOOP("File: Quit MusE"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_Q, 
            },
      {  
            "select_parts_on_track",
            QT_TR_NOOP("Edit: Select parts on track"),
            ARRANG_SHRT, 
            Qt::CTRL+ Qt::ALT + Qt::Key_P, 
            },
      {  
            "open_pianoroll",
            QT_TR_NOOP("Open pianoroll"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_E, 
            },
      {  
            "open_drumedit",
            QT_TR_NOOP("Open drumeditor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_D, 
            },
      {  
            "open_listedit",
            QT_TR_NOOP("Open listeditor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_L, 
            },
      {  
            "open_graph_master",
            QT_TR_NOOP("Open graphical mastertrack editor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_M, 
            },
      {  
            "open_list_master",
            QT_TR_NOOP("Open list mastertrack editor"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::SHIFT + Qt::Key_M, 
            },
      {  
            "add_midi_track",
            QT_TR_NOOP("Add midi track"),
            ARRANG_SHRT, 
            Qt::CTRL + Qt::Key_J, 
            },
      {  
            "add_drum_track",
            QT_TR_NOOP("Add drum track"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "add_wave_track",
            QT_TR_NOOP("Add wave track"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "add_audio_output",
            QT_TR_NOOP("Add audio output"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "add_audio_group",
            QT_TR_NOOP("Add audio group"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "add_audio_input",
            QT_TR_NOOP("Add audio input"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "global_cut",
            QT_TR_NOOP("Structure: Global cut"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "global_insert",
            QT_TR_NOOP("Structure: Global insert"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "global_split",
            QT_TR_NOOP("Structure: Global split"),
            ARRANG_SHRT,  
            0, 
            },
      {  
            "copy_range",
            QT_TR_NOOP("Structure: Copy range"),
            ARRANG_SHRT,    
            0, 
            },
      {  
            "cut_events",
            QT_TR_NOOP("Structure: Cut events"),
            ARRANG_SHRT,    
            0, 
            },
      {  
            "toggle_mixer",
            QT_TR_NOOP("View: Open mixer window"),
            ARRANG_SHRT, 
            Qt::Key_F10, 
            },
      {  
            "toggle_transport",
            QT_TR_NOOP("View: Toggle transport window"),
            ARRANG_SHRT, 
            Qt::Key_F11, 
         },
      {  
            "toggle_bigtime",
            QT_TR_NOOP("View: Toggle bigtime window"),
            ARRANG_SHRT,   
            Qt::Key_F12, 
            },
      {  
            "marker_window",
            QT_TR_NOOP("View: Open marker window"),
            ARRANG_SHRT,
            Qt::Key_F9, 
            },
      {  
            "follow_jump",
            QT_TR_NOOP("Settings: Follow song by page"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "follow_no",
            QT_TR_NOOP("Settings: Follow song off"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "follow_continuous",
            QT_TR_NOOP("Settings: Follow song continuous"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "configure_shortcuts",
            QT_TR_NOOP("Settings: Configure shortcuts"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "configure_metronome",
            QT_TR_NOOP("Settings: Configure metronome"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "configure_midi_sync",
            QT_TR_NOOP("Settings: Midi sync configuration"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "configure_midi_file",
            QT_TR_NOOP("Settings: Midi file export configuration"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "configure_midi_ports",
            QT_TR_NOOP("Settings: Midi ports / Soft Synth"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "configure_audio_ports",
            QT_TR_NOOP("Settings: Audio subsystem configuration"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "midi_edit_instruments",
            QT_TR_NOOP("Midi: Edit midi instruments"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "midi_remote_control",
            QT_TR_NOOP("Midi: Midi remote control"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "midi_reset",
            QT_TR_NOOP("Midi: Reset midi"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "midi_init",
            QT_TR_NOOP("Midi: Init midi"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "midi_local_off",
            QT_TR_NOOP("Midi: Midi local off"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "audio_bounce_to_track",
            QT_TR_NOOP("Audio: Bounce audio to track"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "audio_bounce_to_file",
            QT_TR_NOOP("Audio: Bounce audio to file"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "audio_restart",
            QT_TR_NOOP("Audio: Restart audio"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "open_help",
            QT_TR_NOOP("Help: Open Manual"),
            ARRANG_SHRT,        
            Qt::Key_F1, 
            },
      {  
            "toggle_whatsthis",
            QT_TR_NOOP("Help: Toggle whatsthis mode"),
            ARRANG_SHRT, 
            Qt::SHIFT + Qt::Key_F1, 
            },
      {  
            "edit_selected_part",
            QT_TR_NOOP("Edit: Edit selected part"),
            ARRANG_SHRT, 
            Qt::Key_Return, 
            },
      {  
            "sel_part_above",
            QT_TR_NOOP("Edit: Select nearest part on track above"),
            ARRANG_SHRT, 
            Qt::Key_Up, 
            },
      {  
            "sel_part_above_add",
            QT_TR_NOOP("Edit: Add nearest part on track above"),
            ARRANG_SHRT, 
            Qt::SHIFT + Qt::Key_Up, 
            },
      {  
            "sel_part_below",
            QT_TR_NOOP("Edit: Select nearest part on track below"),
            ARRANG_SHRT, 
            Qt::Key_Down, 
            },
      {  
            "sel_part_below_add",
            QT_TR_NOOP("Edit: Add nearest part on track below"),
            ARRANG_SHRT, 
            Qt::SHIFT + Qt::Key_Down, 
            },
      {  
            "midi_transpose",
            QT_TR_NOOP("Midi: Transpose"),
            ARRANG_SHRT + PROLL_SHRT, 
            0, 
            },
      {  
            "sel_all",
            QT_TR_NOOP("Edit: Select all"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_A, 
            },
      {  
            "sel_none",
            QT_TR_NOOP("Edit: Select none"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::SHIFT + Qt::Key_A, 
            },
      {  
            "sel_inv",
            QT_TR_NOOP("Edit: Invert selection"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,  
            Qt::CTRL + Qt::Key_I, 
            },
      {  
            "sel_ins_loc",
            QT_TR_NOOP("Edit: Select events/parts inside locators"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,  
            0, 
            },
      {  
            "sel_out_loc",
            QT_TR_NOOP("Edit: Select events/parts outside locators"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            0, 
            },
      {  
            "sel_left",
            QT_TR_NOOP("Edit: Select nearest part/event to the left"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_Left,  
            },
      {  
            "sel_left_add",
            QT_TR_NOOP("Edit: Add nearest part/event to the left to selection"),
            PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_Left + Qt::SHIFT,  
            },
      {  
            "sel_right",
            QT_TR_NOOP("Edit: Select nearest part/event to the left"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT,
            Qt::Key_Right,  
            },
      {  
            "sel_right_add",
            QT_TR_NOOP("Edit: Add nearest part/event to the right to selection"),
            PROLL_SHRT + DEDIT_SHRT,
            Qt::Key_Right + Qt::SHIFT, 
            },
      {  
            "loc_to_sel",
            QT_TR_NOOP("Edit: Set locators to selection"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::ALT + Qt::Key_P, 
            },
      {  
            "sel_inc_pitch",
            QT_TR_NOOP("Edit: Increase pitch"),
            PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_Up, 
            },
      {  
            "sel_dec_pitch",
            QT_TR_NOOP("Edit: Decrease pitch"),
            PROLL_SHRT + DEDIT_SHRT, 
            Qt::CTRL + Qt::Key_Down, 
            },
      {  
            "midi_fixed_len",
            QT_TR_NOOP("Edit: Set fixed length on midi events"),
            DEDIT_SHRT, 
            Qt::ALT + Qt::Key_L, 
            },
      {  
            "midi_over_quant",
            QT_TR_NOOP("Quantize: Over Quantize"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_quant_noteon",
            QT_TR_NOOP("Quantize: Note On Quantize"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_quant_noteoff",
            QT_TR_NOOP("Quantize: Note On/Off Quantize"),
            PROLL_SHRT,
            0, 
            },
      {  
            "midi_quant_iterative",
            QT_TR_NOOP("Quantize: Iterative Quantize"),
            PROLL_SHRT,
            0,
            },
      {  
            "config_quant",
            QT_TR_NOOP("Quantize: Configure quant"),
            PROLL_SHRT, 
            Qt::CTRL + Qt::ALT + Qt::Key_Q, 
            },
      {  
            "midi_mod_gate_time",
            QT_TR_NOOP("Quantize: Modify Gate Time"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_mod_velo",
            QT_TR_NOOP("Quantize: Modify Velocity"),
            PROLL_SHRT,  
            0, 
            },
      {  
            "midi_crescendo",
            QT_TR_NOOP("Edit: Crescendo"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_thin_out",
            QT_TR_NOOP("Edit: Thin Out"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_erase_event",
            QT_TR_NOOP("Edit: Erase Event"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_note_shift",
            QT_TR_NOOP("Edit: Note Shift"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_move_clock",
            QT_TR_NOOP("Edit: Move Clock"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_copy_measure",
            QT_TR_NOOP("Edit: Copy Measure"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_erase_measure",
            QT_TR_NOOP("Edit: Erase Measure"),
            PROLL_SHRT,
            0, 
            },
      {  
            "midi_delete_measure",
            QT_TR_NOOP("Edit: Delete Measure"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "midi_create_measure",
            QT_TR_NOOP("Edit: Create Measure"),
            PROLL_SHRT, 
            0, 
            },
      {  
            "change_event_color",
            QT_TR_NOOP("Edit: Change event color"),
            PROLL_SHRT, 
            Qt::Key_E, 
            },
      {  
            "pointer_tool",
            QT_TR_NOOP("Tool: Pointer"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_A, 
            },
      {  
            "pencil_tool",
            QT_TR_NOOP("Tool: Pencil"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_D, 
            },
      {  
            "eraser_tool",
            QT_TR_NOOP("Tool: Eraser"),
            ARRANG_SHRT + PROLL_SHRT + DEDIT_SHRT, 
            Qt::Key_R, 
            },
      {  
            "line_draw_tool",
            QT_TR_NOOP("Tool: Line Draw"),
            PROLL_SHRT + DEDIT_SHRT, 
            0, 
            },
      {  
            "scissor_tool",
            QT_TR_NOOP("Tool: Scissor"),
            ARRANG_SHRT, 
            Qt::Key_S, 
            },
      {  
            "glue_tool",
            QT_TR_NOOP("Tool: Glue"),
            ARRANG_SHRT, 
            Qt::Key_G, 
            },
      {  
            "mute_tool",
            QT_TR_NOOP("Tool: Mute"),
            ARRANG_SHRT, 
            0, 
            },
      {  
            "curpos_increase",
            QT_TR_NOOP("Transport: Increase current position"),
            GLOBAL_SHRT, 
            Qt::Key_Plus,  
            },
      {  
            "curpos_decrease",
            QT_TR_NOOP("Transport: Decrease current position"),
            GLOBAL_SHRT,  
            Qt::Key_Minus, 
            },
      {  
            "midi_quant_1",
            QT_TR_NOOP("Quantize: Set quantize to 1/1 note"),
            PROLL_SHRT, 
            Qt::Key_1, 
            },
      {  
            "midi_quant_2",
            QT_TR_NOOP("Quantize: Set quantize to 1/2 note"),
            PROLL_SHRT, 
            Qt::Key_2, 
            },
      {  
            "midi_quant_3",
            QT_TR_NOOP("Quantize: Set quantize to 1/4 note"),
            PROLL_SHRT, 
            Qt::Key_3, 
            },
      {  
            "midi_quant_4",
            QT_TR_NOOP("Quantize: Set quantize to 1/8 note"),
            PROLL_SHRT, 
            Qt::Key_4, 
            },
      {  
            "midi_quant_5",
            QT_TR_NOOP("Quantize: Set quantize to 1/16 note"),
            PROLL_SHRT, 
            Qt::Key_5,
            },
      {  
            "midi_quant_6",
            QT_TR_NOOP("Quantize: Set quantize to 1/32 note"),
            PROLL_SHRT, 
            Qt::Key_6, 
            },
      {  
            "midi_quant_7",
            QT_TR_NOOP("Quantize: Set quantize to 1/64 note"),
            PROLL_SHRT, 
            Qt::Key_7, 
            },
      {  
            "midi_quant_triol",
            QT_TR_NOOP("Quantize: Toggle triol quantization"),
            PROLL_SHRT, 
            Qt::Key_T, 
            },
      {  
            "midi_quant_punct",
            QT_TR_NOOP("Quantize: Toggle punctuation quantization"),
            PROLL_SHRT, 
            Qt::Key_Period, 
            },
      {  
            "midi_quant_punct2",
            QT_TR_NOOP("Quantize: Toggle punctuation quantization (2)"),
            PROLL_SHRT, 
            Qt::Key_Comma, 
            },
      {  
            "lm_ins_tempo",
            QT_TR_NOOP("Insert Tempo"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::Key_T,
            },
      {  
            "lm_ins_sig",
            QT_TR_NOOP("Insert Signature"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::Key_R,            
            },
      {  
            "lm_edit_beat",
            QT_TR_NOOP("Change Event Position"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::SHIFT + Qt::Key_E, 
            },
      {  
            "lm_edit_val",
            QT_TR_NOOP("Edit Event Value"),
            LMEDIT_SHRT,  
            Qt::CTRL + Qt::Key_E,
         },
      };

KeyboardMovementIndicator shortcutsKbdMovement; //for keeping track of active part selected by kbd

//---------------------------------------------------------
//   getShrtByTag
//---------------------------------------------------------

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

//---------------------------------------------------------
//   writeShortCuts
//---------------------------------------------------------

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

