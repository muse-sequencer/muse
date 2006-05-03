//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: listedit.h,v 1.20 2005/11/04 12:03:47 wschweer Exp $
//  (C) Copyright 1999-2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __LIST_EDIT_H__
#define __LIST_EDIT_H__

#include "widgets/noteinfo.h"
#include "al/pos.h"
#include "event.h"
#include "cobject.h"
#include "ctrl.h"

namespace Awl {
      class SigEdit;
      };

namespace AL {
      class SigEvent;
      class TEvent;
      };

class Event;
class MidiTrack;
class PartList;
class MidiPart;
class MidiPart;
class Track;
class EditorSettings;
class Ctrl;
class Part;

#if 0
//---------------------------------------------------------
//   LLItem
//---------------------------------------------------------

class LLItem : public QObject, public Q3CheckListItem {
      Q_OBJECT

      int _rtti;
      Track* _track;
      Part* _part;
      int _ctrl;

      virtual void stateChange(bool) {
            emit clicked(this);
            }
      int width(const QFontMetrics&, const QListWidget*, int) const {
            return 155;
            }

   signals:
      void clicked(LLItem*);

   public:
      LLItem(QListWidgetItem* parent, const QString& text, int id)
         : QCheckListItem(parent, text, Q3CheckListItem::CheckBoxController)
            {
            _rtti = id;
            }

      LLItem(Q3ListView* parent, const QString& text, int id)
         : Q3CheckListItem(parent, text, Q3CheckListItem::CheckBoxController)
            {
            _rtti = id;
            }
      virtual int rtti() const { return _rtti;  }
      void setTrack(Track* t)  { _track = t;    }
      Track* track() const     { return _track; }
      void setPart(Part* p)    { _part = p;     }
      Part* part() const       { return _part;  }
      void setCtrl(int id)     { _ctrl = id;    }
      int ctrl() const         { return _ctrl;  }
      };

//---------------------------------------------------------
//   LItem
//---------------------------------------------------------

class LItem : public Q3ListViewItem {
      bool zebra;
      int _rtti;
      Pos _pos;
      bool* showHex;

      AL::TEvent* _tempo;
      Event _event;
      Part* _part;
      AL::SigEvent* _sig;
      CVal _val;
      LLItem* _lli;

      virtual void paintCell(QPainter*, const QColorGroup&, int, int, int);

   public:
      LItem(Q3ListView* parent, int id, bool* shp)
         : Q3ListViewItem(parent) {
            showHex = shp;
            _rtti = id;
            zebra = false;
            }
      virtual int rtti() const { return _rtti; }
      virtual QString text(int col) const;
      void setPos(Pos p)        { _pos = p; }
      unsigned tick() const     { return _pos.tick(); }
      void setTempo(AL::TEvent* t)  { _tempo = t; }
      AL::TEvent* tempo() const     { return _tempo; }
      virtual int compare(Q3ListViewItem* i, int, bool) const {
            return _pos.tick() - ((LItem*)i)->_pos.tick();
            }
      void setEvent(Event ev)       { _event = ev;   }
      Event event() const           { return _event; }
      void setVal(CVal v)           { _val = v;      }
      CVal val() const              { return _val;   }
      void setZebra(bool v)         { zebra = v;     }
      void setSigEvent(AL::SigEvent* e) { _sig = e;      }
      AL::SigEvent* sigEvent() const    { return _sig;   }
      void setPart(Part* p)         { _part = p;     }
      Part* part() const            { return _part;  }
      void setLLi(LLItem* i)        { _lli = i;      }
      LLItem* lli() const           { return _lli;   }
      };

//---------------------------------------------------------
//   ListEdit
//---------------------------------------------------------

class ListEdit : public TopWin {
      Q_OBJECT
      Q3ListView* lists;
      Q3ListView* list;
      LItem* editItem;
      int editCol;
      QLineEdit* textEditor;
      Awl::SigEdit* sigEditor;
      Awl::PitchEdit* pitchEditor;
      Awl::PosEdit* posEditor;
      Q3PopupMenu* menuEdit;
      Q3PopupMenu* menuView;
      QLabel* tb_p;     // current part
      QLabel* tb_t;     // current track

      MidiPart* curPart;
      QSplitter* splitter;

      QToolBar* listTools;
      QWidget* curEditor;
      bool showHex;               // show midi values in hex

      enum { CMD_DELETE, CMD_INSERT_SIG, CMD_INSERT_TEMPO, CMD_INSERT_NOTE, CMD_INSERT_SYSEX,
             CMD_INSERT_PAFTER, CMD_INSERT_CAFTER, CMD_INSERT_META,
             CMD_INSERT_CTRL, CMD_SET_DEC, CMD_SET_HEX
            };

      void setEditorGeometry();
      void genListsTrack(Track*, Q3ListViewItem*);
      void curPartChanged(MidiPart* part);

   private slots:
      void updateLists();
      void editColumn(Q3ListViewItem*, const QPoint&, int col);
      void returnPressed();
      void escapePressed();
      virtual void colResized();
      void cmd(int cmd);
      void listSelectionChanged();
      void updateList();
      void songChanged(int);
      void trackAdded(Track*, int idx);
      void trackRemoved(Track*);

   public:
      ListEdit(QWidget* parent, PartList*);
      ~ListEdit();
      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      };

class ListCtrl;

//---------------------------------------------------------
//   EditCtrlDialog
//---------------------------------------------------------

// class EditCtrlDialog : public EditCtrlBase  {
class EditCtrlDialog : public QDialog  {
      Q_OBJECT

      ListCtrl* ctrl;
      Q3PopupMenu* pop;

      void updatePatch();

   private slots:
      void ctrlListClicked(Q3ListBoxItem*);
      void newController();
      void programChanged();
      void instrPopup();

   protected:
      QGridLayout* layout;


   public:
      EditCtrlDialog(ListCtrl*, QWidget* parent=0);
      };

#endif
#endif

