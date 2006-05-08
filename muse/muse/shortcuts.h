//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
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

#ifndef __SHORTCUTS_H__
#define __SHORTCUTS_H__

class Part;
class Track;
namespace AL {
      class Xml;
      };
using AL::Xml;

//
// Shortcut categories
//
#define PROLL_SHRT       1  // Pianoroll shortcut
#define DEDIT_SHRT       2  // Drumedit shortcut
#define LEDIT_SHRT       4  // Listedit shortcut
#define SCORE_SHRT       8  // Score shortcut
#define ARRANG_SHRT     16  // Arrenger shortcut
#define TRANSP_SHRT     32  // Transport shortcut
#define WAVE_SHRT       64  // Waveedit shortcut
#define GLOBAL_SHRT    128  // Global shortcuts
#define LMEDIT_SHRT    256  // List masteredit
#define MEDIT_SHRT     512  // Master editor
#define ALL_SHRT      1023  // All shortcuts
#define INVIS_SHRT    1024  // Shortcuts not shown in the config-dialog. Hard-coded. To avoid conflicts

#define SHRT_NUM_OF_CATEGORIES   7 //Number of shortcut categories

//---------------------------------------------------------
//   shortcut
//---------------------------------------------------------
//! Holds the basic values for a configurable shortcut
struct shortcut
      {
      int key; /*! Integer storing the keyboard shortcut.  */
      QString descr; /*! Description of the shortcut, shown in editor. Mapped against ls[] in shortcuts.cpp */
      const char* xml; /*! xml tag name for configuration file*/
      int type; /*! Bitmask category value mapped against PROLL_SHRT, DEDIT_SHRT etc. One shortcut can be a member of many categories */
      };

//! Describes a shortcut category
struct shortcut_cg
      {
      int         id_flag; /*! The category (one of PROLL_SHRT, DEDIT_SHRT etc) */
      const char* name; /*! Name (shown in editor) */
      };

//------------------------------------------------------------------------------------------------
//   KeyboardMovementIndicator
//! Used by Arranger to keep track of which Part is currently active when navigating with keys
//------------------------------------------------------------------------------------------------
class KeyboardMovementIndicator {
      //! Left position of the active part, in ticks
      unsigned lpos;
      //! Right position of the active part, in ticks
      unsigned rpos;
      //! Last selected part (the active part)
      Part* lastSelectedPart;
      //! Track the last selected part belongs to
      Track* lastSelectedTrack;

   public:
      KeyboardMovementIndicator()
         { reset(); }

      void setPos(int l, int r) { lpos = l; rpos = r; }
      void setPart(Part* p)     { lastSelectedPart = p; }
      void setTrack(Track* t)   { lastSelectedTrack = t; }
      unsigned getLpos()        { return lpos; }
      unsigned getRpos()        { return rpos; }
      Part* part()         { return lastSelectedPart; }
      Track* track()       { return lastSelectedTrack; }
      //! Resets the values (equals to no active part)
      void reset()         { lpos = 0; rpos = 0; lastSelectedPart = 0; lastSelectedTrack = 0; }
      //! Checks if there is any active part
      bool isValid()       { return (lastSelectedPart && lastSelectedTrack); }
      };

//! Enumeration of the configurable shortcuts
enum {
      //Transport/Positioning
      SHRT_PLAY_SONG, // enter
      SHRT_PLAY_TOGGLE, // space
      SHRT_STOP, //Insert
      SHRT_GOTO_LEFT, //End-keypad
      SHRT_GOTO_RIGHT, //Cursordown-keypad
      SHRT_POS_INC, // Plus
      SHRT_POS_DEC, // Minus
      SHRT_TOGGLE_LOOP, // Slash
      SHRT_TOGGLE_METRO, // C
      SHRT_START_REC, // *(keypad)

      //Main + Drumeditor
      SHRT_OPEN, //Ctrl+O
      SHRT_SAVE, //Ctrl+S

      //Used throughout the app:
      SHRT_UNDO,  //Ctrl+Z
      SHRT_REDO,  //Ctrl+Y
      SHRT_COPY,  //Ctrl+C
      SHRT_CUT,   //Ctrl+X
      SHRT_PASTE, //Ctrl+V
      SHRT_DELETE,//Delete


      //Main:
      SHRT_OPEN_RECENT, //Ctrl+1
      SHRT_LOAD_TEMPLATE, //Default: undefined
      SHRT_CONFIG_PRINTER, //Ctrl+P
      SHRT_IMPORT_MIDI, //Default: undefined
      SHRT_EXPORT_MIDI, //Default: undefined
      SHRT_IMPORT_AUDIO, //Default: undefined
      SHRT_QUIT, //Default: Ctrl+Q

      SHRT_DESEL_PARTS, //Ctrl+B
      SHRT_SELECT_PRTSTRACK, //Default: undefined
      SHRT_OPEN_PIANO, //Ctrl+E
      SHRT_OPEN_SCORE, //Ctrl+R
      SHRT_OPEN_DRUMS, //Ctrl+D
      SHRT_OPEN_LIST, //Ctrl+L
      SHRT_OPEN_GRAPHIC_MASTER, //Ctrl+M
      SHRT_OPEN_LIST_MASTER, //Ctrl+Shift+M
      SHRT_OPEN_MIDI_TRANSFORM, //Ctrl+T

      SHRT_GLOBAL_CUT, //Default: undefined
      SHRT_GLOBAL_INSERT, //Default: undefined
      SHRT_GLOBAL_SPLIT, //Default: undefined
      SHRT_COPY_RANGE, //Default: undefined
      SHRT_CUT_EVENTS, //Default: undefined

      SHRT_OPEN_TRANSPORT, //F11
      SHRT_OPEN_BIGTIME, //F12
      SHRT_OPEN_MIXER, //Ctrl+*
      SHRT_OPEN_MARKER, // F9
      SHRT_OPEN_CLIPS, //Default: undefined

      SHRT_FOLLOW_JUMP, //Default: undefined
      SHRT_FOLLOW_NO, //Default: undefined
      SHRT_FOLLOW_CONTINUOUS, //Default: undefined

      SHRT_GLOBAL_CONFIG, //Default: undefined
      SHRT_CONFIG_SHORTCUTS, //Default: undefined
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

//      SHRT_MIXER_AUTOMATION, //Default: undefined
      SHRT_MIXER_SNAPSHOT, //Default: undefined
      SHRT_MIXER_AUTOMATION_CLEAR, //Default: undefined

      SHRT_ADD_MIDI_TRACK, //Default: Ctrl+J
      SHRT_ADD_DRUM_TRACK, //Default: undefined
      SHRT_ADD_WAVE_TRACK, //Default: undefined
      SHRT_ADD_AUDIO_OUTPUT, //Default: undefined
      SHRT_ADD_AUDIO_GROUP, //Default: undefined
      SHRT_ADD_AUDIO_INPUT, //Default: undefined
      SHRT_ADD_AUDIO_AUX, //Default: undefined
      SHRT_RESET_MIDI, //Ctrl+Alt+Z

      SHRT_OPEN_HELP, //F1
      SHRT_START_WHATSTHIS, //Shift-F1

      //Arranger, parts:
      SHRT_EDIT_PART, //Enter
      SHRT_SEL_ABOVE, //Up
      SHRT_SEL_ABOVE_ADD, //move up and add to selection
      SHRT_SEL_BELOW, //Down
      SHRT_SEL_BELOW_ADD, //move down and add to selection

      //To be in arranger, pianoroll & drumeditor
      SHRT_SELECT_ALL, //Ctrl+A
      SHRT_SELECT_NONE, //Ctrl+Shift+A
      SHRT_SELECT_INVERT, //Ctrl+I
      SHRT_SELECT_ILOOP, //Default: Undefined
      SHRT_SELECT_OLOOP, //Default: Undefined
      SHRT_SEL_LEFT, //left
      SHRT_SEL_LEFT_ADD, //move left and add to selection
      SHRT_SEL_RIGHT, //Right
      SHRT_SEL_RIGHT_ADD, //move right and add to selection
      SHRT_INC_PITCH,
      SHRT_DEC_PITCH,
      SHRT_INC_POS,
      SHRT_DEC_POS,
      SHRT_LOCATORS_TO_SELECTION, //Alt+P, currently in arranger & pianoroll
      SHRT_ZOOM_IN, //H - pianoroll
      SHRT_ZOOM_OUT, //G - pianoroll
      SHRT_INSERT_AT_LOCATION, //Shift+CrsrRight

      SHRT_TOOL_1,//Shift+1 Pointer
      SHRT_TOOL_2,//Shift+2 Pen
      SHRT_TOOL_3,//Shift+3 Rubber
      SHRT_TOOL_4,//Shift+4
      SHRT_TOOL_5,//Shift+5
      SHRT_TOOL_6,//Shift+6
      SHRT_TRANSPOSE, //Default: undefined

      //Shortcuts to be in pianoroll & drumeditor
      SHRT_FIXED_LEN, //Alt+L, currently only drumeditor
      SHRT_QUANTIZE, //q
      SHRT_OVER_QUANTIZE, //Default: undefined
      SHRT_ON_QUANTIZE, //Default: undefined
      SHRT_ONOFF_QUANTIZE, //Default: undefined
      SHRT_ITERATIVE_QUANTIZE, //Default: undefined
      SHRT_CONFIG_QUANT, //Default: Ctrl+Alt+Q
      SHRT_MODIFY_GATE_TIME, //Default: undefined
      SHRT_MODIFY_VELOCITY,
      SHRT_CRESCENDO,

      SHRT_THIN_OUT,
      SHRT_ERASE_EVENT,
      SHRT_NOTE_SHIFT,
      SHRT_MOVE_CLOCK,
      SHRT_COPY_MEASURE,
      SHRT_ERASE_MEASURE,
      SHRT_DELETE_MEASURE,
      SHRT_CREATE_MEASURE,
      SHRT_SET_QUANT_1, //1 - pianoroll
      SHRT_SET_QUANT_2, //2 - pianoroll
      SHRT_SET_QUANT_3, //3 - pianoroll
      SHRT_SET_QUANT_4, //4 - pianoroll
      SHRT_SET_QUANT_5, //5 - pianoroll
      SHRT_SET_QUANT_6, //6 - pianoroll
      SHRT_SET_QUANT_7, //7 - pianoroll
      SHRT_TOGGLE_TRIOL, //t
      SHRT_TOGGLE_PUNCT, //.-keypad
      SHRT_TOGGLE_PUNCT2, // ,

      SHRT_EVENT_COLOR, //e

      // Shortcuts for tools
      // global
      SHRT_TOOL_POINTER,  //
      SHRT_TOOL_PENCIL,
      SHRT_TOOL_RUBBER,

      // pianoroll and drum editor
      SHRT_TOOL_LINEDRAW,

      // arranger
      SHRT_TOOL_SCISSORS,
      SHRT_TOOL_GLUE,
      SHRT_TOOL_MUTE,


      //Listeditor:
      SHRT_LE_INS_NOTES, //Ctrl+N
      SHRT_LE_INS_SYSEX, //Ctrl+S
      SHRT_LE_INS_CTRL, //Ctrl+T
      SHRT_LE_INS_META, //Default: undefined
      SHRT_LE_INS_CHAN_AFTERTOUCH,//Ctrl+A
      SHRT_LE_INS_POLY_AFTERTOUCH,//Ctrl+P

      //List master editor:
      SHRT_LM_INS_TEMPO, // Ctrl+T
      SHRT_LM_INS_SIG,   // Ctrl+R
      SHRT_LM_EDIT_BEAT, // Ctrl+Shift+E
      SHRT_LM_EDIT_VALUE,// Ctrl+E

      SHRT_NUM_OF_ELEMENTS        // must be last
      };

//#define SHRT_NUM_OF_ELEMENTS           SHRT_LE_INS_POLY_AFTERTOUCH + 1
extern shortcut shortcuts[SHRT_NUM_OF_ELEMENTS]; //size of last entry
extern KeyboardMovementIndicator shortcutsKbdMovement;
extern void initShortCuts();
extern void writeShortCuts(Xml& xml);
extern void readShortCuts(QDomNode);
#endif
