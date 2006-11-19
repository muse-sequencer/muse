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
//!  Holds the basic values for a configurable shortcut
//---------------------------------------------------------

struct Shortcut
      {
      const char* xml;   /*! xml tag name for configuration file          */
      QString descr;     /*! Description of the shortcut, shown in editor */
      int type;          /*! Bitmask category value mapped against PROLL_SHRT, DEDIT_SHRT etc. One shortcut can be a member of many categories */
      QKeySequence key;  /*! shortcut key */
      QAction* action;
      QString help;

      Shortcut() {
            xml  = 0;
            type = 0;
            key  = 0;
            action = 0;
            }
      Shortcut(const char* x, const QString& d, int t, const QKeySequence& k, const QString& h) 
       : xml(x), descr(d), type(t), key(k), help(h) { 
            action = 0; 
            }
      Shortcut(const char* x, const QString& d, int t, const QKeySequence& k) 
       : xml(x), descr(d), type(t), key(k) { 
            action = 0; 
            help   = descr;
            }
      };

//! Describes a shortcut category
struct shortcut_cg
      {
      int         id_flag; /*! The category (one of PROLL_SHRT, DEDIT_SHRT etc) */
      const char* name;    /*! Name (shown in editor) */
      };

//------------------------------------------------------------------------------------------------
//   KeyboardMovementIndicator
//!  Used by Arranger to keep track of which Part is currently active when navigating with keys
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

extern QMap<QString, Shortcut*> shortcuts;

extern KeyboardMovementIndicator shortcutsKbdMovement;
extern void writeShortCuts(Xml& xml);
extern void readShortCuts(QDomNode);
extern QAction* getAction(const char*, QObject* parent);
extern void initShortcuts();
#endif
