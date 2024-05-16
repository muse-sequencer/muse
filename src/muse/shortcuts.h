//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/shortcuts.h $
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
// C++ Interface: shortcuts
//
// Description:
// Datastructures and declaration of shortcuts used in the application
//

#ifndef __SHORTCUTS_H__
#define __SHORTCUTS_H__

#include <list>
#include "xml.h"

//
// Shortcut categories
//
#define PROLL_SHRT       1  // Pianoroll shortcut
#define DEDIT_SHRT       2  // Drumedit shortcut
#define LEDIT_SHRT       4  // Listedit shortcut
#define SCORE_SHRT       8  // Score shortcut
#define ARRANG_SHRT     16  // Arranger shortcut
#define TRANSP_SHRT     32  // Transport shortcut
#define WAVE_SHRT       64  // Waveedit shortcut
#define GLOBAL_SHRT    128  // Global shortcuts
#define LMEDIT_SHRT    256  // List masteredit
#define MEDIT_SHRT     512  // Master editor
#define MIXER_SHRT    1024  // Mixer
#define ALL_SHRT      2047  // All shortcuts
#define INVIS_SHRT    2048  // Shortcuts not shown in the config-dialog. Hard-coded. To avoid conflicts

#define SHRT_NUM_OF_CATEGORIES   10 //Number of shortcut categories

namespace MusEGui {

struct shortcut
{
    int key;
    const char* descr;
    const char* xml; //xml-tag for config-file
    int type;
    shortcut() : key(0),  descr(nullptr), xml(nullptr), type(0) { }
};

struct shortcut_cg
{
    int         id_flag;
    const char* name;
};

QString shrtToStr(long int shrt);

typedef struct shortcut ShortCut ;

enum {
    //Transport/Positioning
    SHRT_PLAY_SONG, //Enter
    SHRT_PLAY_TOGGLE, //Space
    SHRT_REC_RESTART, //Ctrl+Space
    SHRT_REC_RESTART_MULTI, //Shift+Space
    SHRT_STOP, //Insert
    SHRT_GOTO_START, // W
    SHRT_GOTO_END,   // Ctrl+W

    SHRT_GOTO_LEFT,  //End-keypad
    SHRT_GOTO_RIGHT, //Cursordown-keypad
    SHRT_POS_INC, // Plus
    SHRT_POS_DEC, // Minus
    SHRT_TOGGLE_LOOP, // Slash
    SHRT_TOGGLE_METRO, // C
    SHRT_START_REC, // *(keypad)
    SHRT_REC_ARM_TRACK, // Shift+A
    SHRT_REC_CLEAR, // backspace
    SHRT_FULLSCREEN,
    SHRT_TABBED_WIN,
    SHRT_TOGGLE_DOCKS,
    SHRT_TOGGLE_REWINDONSTOP,
    SHRT_ARRANGER,

    //Main + Drumeditor
    SHRT_NEW, //Ctrl+N
    SHRT_NEW_FROM_TEMPLATE, //Ctrl+Shift+N
    SHRT_OPEN, //Ctrl+O
    SHRT_SAVE, //Ctrl+S

    //Used throughout the app:
    SHRT_UNDO,  //Ctrl+Z
    SHRT_REDO,  //Ctrl+Y
    SHRT_COPY,  //Ctrl+C
    SHRT_COPY_RANGE, //Ctrl+Shift+C
    SHRT_CUT,   //Ctrl+X
    SHRT_PASTE, //Ctrl+V
    SHRT_PASTE_TO_CUR_PART, //CTRL+Key_B
    SHRT_PASTE_CLONE, //CTRL+Shift+V
    SHRT_PASTE_TO_TRACK, //CTRL+Key_B
    SHRT_PASTE_CLONE_TO_TRACK, //CTRL+SHIFT+Key_B
    SHRT_PASTE_DIALOG, //Ctrl+G
    SHRT_DELETE,//Delete
    SHRT_RETURN,
    SHRT_ESCAPE,


    //Main:
    SHRT_SAVE_AS, //Default: undefined
    SHRT_SAVE_AS_NEW_PROJECT,
    SHRT_SAVE_REVISION,
    SHRT_SAVE_AS_TEMPLATE,
    SHRT_OPEN_RECENT, //Ctrl+1
    SHRT_LOAD_TEMPLATE, //Default: undefined
    SHRT_CONFIG_PRINTER, //Ctrl+P
    SHRT_IMPORT_MIDI, //Default: undefined
    SHRT_EXPORT_MIDI, //Default: undefined
    SHRT_IMPORT_PART, //!< Import midi part to current track & location, Default: undefined
    SHRT_IMPORT_AUDIO, //Default: undefined
    SHRT_QUIT, //Default: Ctrl+Q

    SHRT_DESEL_PARTS, //Ctrl+B
    SHRT_RENAME_PART, // Default: undefined
    SHRT_SELECT_PRTSTRACK, //Default: undefined
    SHRT_OPEN_PIANO, //Ctrl+E
    SHRT_OPEN_SCORE, //Ctrl+R
    SHRT_OPEN_DRUMS, //Ctrl+D
    SHRT_OPEN_LIST, //Ctrl+L
    SHRT_OPEN_WAVE, //Ctrl+W
    SHRT_OPEN_GRAPHIC_MASTER, //Ctrl+M
    SHRT_OPEN_LIST_MASTER, //Ctrl+Shift+M
    SHRT_OPEN_MIDI_TRANSFORM, //Ctrl+T
    SHRT_OPEN_PLUGIN_GUI,

    SHRT_OPEN_PIANO_NEW, //Ctrl+Alt+E
    SHRT_OPEN_DRUMS_NEW, //Ctrl+Alt+D
    SHRT_OPEN_LIST_NEW, //Ctrl+Alt+L
    SHRT_OPEN_WAVE_NEW, //Ctrl+Alt+W

    SHRT_GLOBAL_CUT, //Default: undefined
    SHRT_GLOBAL_INSERT, //Default: undefined
    SHRT_GLOBAL_SPLIT, //Default: undefined
    SHRT_CUT_EVENTS, //Default: undefined

    SHRT_OPEN_TRANSPORT, //F11
    SHRT_OPEN_BIGTIME, //F12
    SHRT_OPEN_MIXER, //Ctrl+*
    SHRT_OPEN_MIXER2, //Ctrl+*
    SHRT_OPEN_MARKER, // F9
    SHRT_HIDE_MIXER_STRIP, // F3
    SHRT_OPEN_CLIPS, //Default: undefined

    SHRT_FOLLOW_JUMP, //Default: undefined
    SHRT_FOLLOW_NO, //Default: undefined
    SHRT_FOLLOW_CONTINUOUS, //Default: undefined

    SHRT_GLOBAL_CONFIG, //Default: undefined
    SHRT_CONFIG_SHORTCUTS, //Default: Ctrl+K
    SHRT_CONFIG_METRONOME, //Default: undefined
    SHRT_CONFIG_MIDISYNC, //Default: undefined
    SHRT_MIDI_FILE_CONFIG, //Default: undefined
    SHRT_APPEARANCE_SETTINGS, //Default: undefined
    SHRT_CONFIG_MIDI_PORTS, //Default: undefined
    SHRT_CONFIG_AUDIO_PORTS, //Default: undefined
    //SHRT_SAVE_GLOBAL_CONFIG, //Default: undefined

    SHRT_MIDI_EDIT_INSTRUMENTS, //Default: undefined
    SHRT_MIDI_INPUT_TRANSFORM, //Default: undefined
    SHRT_MIDI_INPUT_FILTER, //Default: undefined
    SHRT_MIDI_INPUT_TRANSPOSE, //Default: undefined
    SHRT_MIDI_REMOTE_CONTROL, //Default: undefined
    SHRT_RANDOM_RHYTHM_GENERATOR, //Default: undefined
    SHRT_MIDI_RESET, //Default: undefined
    SHRT_MIDI_INIT, //Default: undefined
    SHRT_MIDI_LOCAL_OFF, //Default: undefined

    SHRT_AUDIO_BOUNCE_TO_TRACK, //Default: undefined
    SHRT_AUDIO_BOUNCE_TO_FILE, //Default: undefined
    SHRT_AUDIO_RESTART, //Default: undefined

    // REMOVE Tim. automation.
    // Deprecated. MusEGlobal::automation is now fixed TRUE
    //  for now until we decide what to do with it.
    // Must keep this enum for proper ordering.
    SHRT_MIXER_AUTOMATION, //Default: undefined

    SHRT_MIXER_SNAPSHOT, //Default: undefined
    SHRT_MIXER_AUTOMATION_CLEAR, //Default: undefined

    SHRT_ADD_TRACK_MENU, //Default: Ins

    SHRT_ADD_MIDI_TRACK, //Default: Ctrl+J
    SHRT_ADD_DRUM_TRACK, //Default: Ctrl+k
//    SHRT_ADD_NEW_STYLE_DRUM_TRACK, //Default: undefined
    SHRT_ADD_SYNTH_TRACK,
    SHRT_ADD_WAVE_TRACK, //Default: undefined
    SHRT_ADD_AUDIO_OUTPUT, //Default: undefined
    SHRT_ADD_AUDIO_GROUP, //Default: undefined
    SHRT_ADD_AUDIO_INPUT, //Default: undefined
    SHRT_ADD_AUDIO_AUX, //Default: undefined

    SHRT_INSERT_MIDI_TRACK, //Default: undefined
    SHRT_INSERT_DRUM_TRACK, //Default: undefined
//    SHRT_INSERT_NEW_STYLE_DRUM_TRACK, //Default: undefined
    SHRT_INSERT_SYNTH_TRACK,
    SHRT_INSERT_WAVE_TRACK, //Default: undefined
    SHRT_INSERT_AUDIO_OUTPUT, //Default: undefined
    SHRT_INSERT_AUDIO_GROUP, //Default: undefined
    SHRT_INSERT_AUDIO_INPUT, //Default: undefined
    SHRT_INSERT_AUDIO_AUX, //Default: undefined

    SHRT_RESET_MIDI, //Ctrl+Alt+Z

    SHRT_OPEN_HELP, //F1
    SHRT_START_WHATSTHIS, //Shift-F1

    //Arranger, parts:
    SHRT_EDIT_PART, //Enter
    SHRT_SEL_ABOVE, //Up
    SHRT_SEL_ABOVE_ADD, //move up and add to selection
    SHRT_SEL_BELOW, //Down
    SHRT_SEL_BELOW_ADD, //move down and add to selection

    SHRT_INSERTMEAS, //Ctrl+Shift+M - insert measures

    //Arranger tracks
    SHRT_DUPLICATE_TRACK, // Ctrl+Shift+D
    SHRT_DELETE_TRACK, // Ctrl+DEL
    SHRT_MOVEUP_TRACK,
    SHRT_MOVEDOWN_TRACK,
    SHRT_MOVETOP_TRACK,
    SHRT_MOVEBOTTOM_TRACK,

    SHRT_SEL_TRACK_BELOW,
    SHRT_SEL_TRACK_ABOVE,
    SHRT_EDIT_TRACK_NAME,
    SHRT_MUTE_CURRENT_TRACKS,
    SHRT_SOLO_CURRENT_TRACKS,

    SHRT_TOGGLE_TRACK_HEIGHTS,

    //To be in arranger, pianoroll & drumeditor. p4.0.10 now globally handled, too.
    SHRT_SELECT_ALL, //Ctrl+A
    SHRT_SELECT_NONE, //Ctrl+Shift+A
    SHRT_SELECT_INVERT, //Ctrl+I
    SHRT_SELECT_ILOOP, //Default: Undefined
    SHRT_SELECT_OLOOP, //Default: Undefined
    SHRT_SELECT_PREV_PART, // Ctrl+-
    SHRT_SELECT_NEXT_PART, // Ctrl++
    SHRT_SEL_LEFT, //left
    SHRT_SEL_LEFT_ADD, //move left and add to selection
    SHRT_SEL_RIGHT, //Right
    SHRT_SEL_RIGHT_ADD, //move right and add to selection
    SHRT_INC_PITCH,
    SHRT_DEC_PITCH,
    SHRT_INC_POS,
    SHRT_DEC_POS,

    SHRT_STEP_RECORD,
    SHRT_MIDI_INPUT,
    SHRT_PLAY_EVENTS,
    SHRT_INC_VELOCITY,
    SHRT_DEC_VELOCITY,

    SHRT_POS_INC_NOSNAP,
    SHRT_POS_DEC_NOSNAP,

    /*
      SHRT_POS_INC_BAR,
      SHRT_POS_DEC_BAR,
      SHRT_POS_INC_BAR_NOSNAP,
      SHRT_POS_DEC_BAR_NOSNAP,
      
      SHRT_POS_INC_BEAT,
      SHRT_POS_DEC_BEAT,
      SHRT_POS_INC_BEAT_NOSNAP,
      SHRT_POS_DEC_BEAT_NOSNAP,
      
      SHRT_POS_INC_TICK,
      SHRT_POS_DEC_TICK,
      SHRT_POS_INC_TICK_NOSNAP,
      SHRT_POS_DEC_TICK_NOSNAP,
      
      SHRT_POS_INC_MINUTE,
      SHRT_POS_DEC_MINUTE,
      SHRT_POS_INC_MINUTE_NOSNAP,
      SHRT_POS_DEC_MINUTE_NOSNAP,
      
      SHRT_POS_INC_SECOND,
      SHRT_POS_DEC_SECOND,
      SHRT_POS_INC_SECOND_NOSNAP,
      SHRT_POS_DEC_SECOND_NOSNAP,
      
      SHRT_POS_INC_FRAME,
      SHRT_POS_DEC_FRAME,
      */

    SHRT_LOCATORS_TO_SELECTION, //Alt+P, currently in arranger & pianoroll
    SHRT_LOC_PUSH_EVENTS,
    SHRT_LOC_PULL_EVENTS,
    SHRT_INCREASE_LEN,
    SHRT_DECREASE_LEN,

    //SHRT_TOOL_1,//Shift+1 Pointer
    //SHRT_TOOL_2,//Shift+2 Pen
    //SHRT_TOOL_3,//Shift+3 Rubber
    //SHRT_TOOL_4,//Shift+4
    //SHRT_TOOL_5,//Shift+5
    //SHRT_TOOL_6,//Shift+6
    SHRT_TRANSPOSE, //Default: undefined

    //Shortcuts to be in pianoroll & drumeditor
    SHRT_ZOOM_IN,  // PgUp
    SHRT_ZOOM_OUT, // PgDown
    SHRT_GOTO_CPOS, // c
    SHRT_SCROLL_LEFT, // h
    SHRT_SCROLL_RIGHT, // l
    SHRT_SCROLL_LEFT_FAST, //Default: undefined
    SHRT_SCROLL_RIGHT_FAST, //Default: undefined
    SHRT_SCROLL_UP, // o
    SHRT_SCROLL_DOWN, // k
    SHRT_SCROLL_UP_FAST, // Shift+o
    SHRT_SCROLL_DOWN_FAST, // Shift+k

    SHRT_FIXED_LEN, //Alt+L, currently only drumeditor
    SHRT_QUANTIZE, //q
    SHRT_MODIFY_GATE_TIME, //Default: undefined
    SHRT_MODIFY_VELOCITY,
    SHRT_CRESCENDO,
    SHRT_DELETE_OVERLAPS,

    SHRT_THIN_OUT,
    SHRT_ERASE_EVENT,
    SHRT_NOTE_SHIFT,
    SHRT_MOVE_CLOCK,
    SHRT_COPY_MEASURE,
    SHRT_ERASE_MEASURE,
    SHRT_DELETE_MEASURE,
    SHRT_CREATE_MEASURE,
    SHRT_SET_QUANT_BAR,
    SHRT_SET_QUANT_OFF,
    SHRT_SET_QUANT_1,
    SHRT_SET_QUANT_2,
    SHRT_SET_QUANT_3,
    SHRT_SET_QUANT_4,
    SHRT_SET_QUANT_5,
    SHRT_SET_QUANT_6,
    SHRT_SET_QUANT_7,
    SHRT_TOGGLE_TRIOL,
    SHRT_TOGGLE_PUNCT,
    SHRT_TOGGLE_PUNCT2,

    SHRT_EVENT_COLOR,

    SHRT_MOVE_PLAY_TO_NOTE,

    SHRT_USE_LAST_EDITED_EVENT,

    // Shortcuts for tools
    // global
    SHRT_TOOL_POINTER,  //
    SHRT_TOOL_PENCIL,
    SHRT_TOOL_RUBBER,
    SHRT_TOOL_PAN,
    SHRT_TOOL_ZOOM,

    // pianoroll and drum editor
    SHRT_TOOL_LINEDRAW,

    // drum editor
    SHRT_TOOL_CURSOR,
    SHRT_ADDNOTE_1,
    SHRT_ADDNOTE_2,
    SHRT_ADDNOTE_3,
    SHRT_ADDNOTE_4,
    SHRT_CURSOR_STEP_UP,
    SHRT_CURSOR_STEP_DOWN,
    SHRT_INSTRUMENT_STEP_DOWN,
    SHRT_INSTRUMENT_STEP_UP,

    // wave editor
    SHRT_TOOL_RANGE,

    // arranger
    SHRT_TOOL_SCISSORS,
    SHRT_TOOL_GLUE,
    SHRT_TOOL_MUTE,


    //Listeditor:
    SHRT_LE_INS_NOTES, // N
    SHRT_LE_INS_SYSEX, // S
    SHRT_LE_INS_CTRL, // C
    SHRT_LE_INS_META, // M
    // unused
    //      SHRT_LE_INS_CHAN_AFTERTOUCH,//Ctrl+A
    //      SHRT_LE_INS_POLY_AFTERTOUCH,//Ctrl+P

    //List master editor:
    SHRT_LM_INS_TEMPO, // Ctrl+T
    SHRT_LM_INS_SIG,   // Ctrl+R
    SHRT_LM_EDIT_BEAT, // Ctrl+Shift+E
    SHRT_LM_EDIT_VALUE,// Ctrl+E
    SHRT_LM_INS_KEY,   // Ctrl+K

    // Marker view
    SHRT_NEXT_MARKER,
    SHRT_PREV_MARKER,

    // Arranger
    SHRT_PART_NORMALIZE, // Alt+Shift+N

    SHRT_TOOL_STRETCH,
    SHRT_TOOL_SAMPLERATE,

    // Mixer
    SHRT_MIXER_SELECT_STRIP_LEFT,
    SHRT_MIXER_SELECT_STRIP_RIGHT,
    // TODO Feature marked as todo in AudioMixerApp::selectNextStrip().
    //SHRT_MIXER_MULTI_SELECT_STRIP_LEFT,
    //SHRT_MIXER_MULTI_SELECT_STRIP_RIGHT,
    SHRT_MIXER_STRIP_VOL_DOWN,
    SHRT_MIXER_STRIP_VOL_UP,
    SHRT_MIXER_STRIP_PAN_LEFT,
    SHRT_MIXER_STRIP_PAN_RIGHT,
    SHRT_MIXER_STRIP_VOL_DOWN_PAGE,
    SHRT_MIXER_STRIP_VOL_UP_PAGE,
    SHRT_MIXER_STRIP_PAN_LEFT_PAGE,
    SHRT_MIXER_STRIP_PAN_RIGHT_PAGE,

    //Last item:
    SHRT_NUM_OF_ELEMENTS        // must be last
};

extern ShortCut shortcuts[SHRT_NUM_OF_ELEMENTS]; //size of last entry
extern void initShortCuts();
extern void writeShortCuts(int level, MusECore::Xml& xml);
extern void readShortCuts (MusECore::Xml& xml);
extern const shortcut_cg shortcut_category[SHRT_NUM_OF_CATEGORIES];

} // namespace MusEGui

#endif
