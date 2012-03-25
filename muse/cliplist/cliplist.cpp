//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cliplist.cpp,v 1.6.2.3 2008/08/18 00:15:24 terminator356 Exp $
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

#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>

#include "cliplist.h"
#include "song.h"
#include "globals.h"
#include "wave.h"
#include "xml.h"
#include "ui_cliplisteditorbase.h"
#include "app.h"


namespace MusEGui {

extern int mtcType;
enum { COL_NAME=0, COL_REFS, COL_SAMPLERATE, COL_LEN };

//---------------------------------------------------------
//   ClipItem
//---------------------------------------------------------

class ClipItem : public QTreeWidgetItem {
      MusECore::SndFileR _wf;

      //virtual QString text(int) const;

   public:
      ClipItem(QTreeWidget*, const MusECore::SndFileR&);
      MusECore::SndFileR* wf() { return &_wf; }
      };

ClipItem::ClipItem(QTreeWidget* parent, const MusECore::SndFileR& w)
   : QTreeWidgetItem(parent), _wf(w)
      {
        setText(COL_NAME,       _wf.name());
        setText(COL_REFS,       QString().setNum(_wf.getRefCount()));
        setText(COL_SAMPLERATE, QString().setNum(_wf.samplerate()));
        setText(COL_LEN,        QString().setNum(_wf.samples()));
      }

//---------------------------------------------------------
//   samples2smpte
//---------------------------------------------------------

#if 0 // DELETETHIS ?
static QString samples2smpte(int samples)
      {
      double time = double(samples) / double(sampleRate);
      int min  = int(time) / 60;
      int sec  = int(time) % 60;
      double rest = time - (min * 60 + sec);
      switch(mtcType) {
            case 0:     // 24 frames sec
                  rest *= 24;
                  break;
            case 1:     // 25
                  rest *= 25;
                  break;
            case 2:     // 30 drop frame
                  rest *= 30;
                  break;
            case 3:     // 30 non drop frame
                  rest *= 30;
                  break;
                  }
      int frame = int(rest);
      int subframe = int((rest-frame)*100);
      QString s;
      s.sprintf("%03d:%02d:%02d:%02d", min, sec, frame, subframe);
      return s;
      }
#endif

/* DELETETHIS
//---------------------------------------------------------
//   text
//---------------------------------------------------------

QString ClipItem::text(int col) const
      {
      QString s("");
      switch(col) {
            case COL_NAME:
                  s = _wf.name();
                  break;
            case COL_POS:
            case COL_LEN:
                  break;
            case COL_REFS:
                  s.setNum(_wf.getRefCount());
                  break;
            }
      return s;
      }
*/

//---------------------------------------------------------
//   ClipListEdit
//---------------------------------------------------------

ClipListEdit::ClipListEdit(QWidget* parent)
   : TopWin(TopWin::CLIPLIST, parent, "cliplist", Qt::Window)
      {
      setWindowTitle(tr("MusE: Clip List Editor"));

      editor = new ClipListEditorBaseWidget;
      setCentralWidget(editor);


      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);      
      settingsMenu->addAction(shareAction);      
      settingsMenu->addAction(fullscreenAction);      
      
      QFontMetrics fm(editor->view->font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth,0, this); // ddskrjo 0
      int w  = 2 + fm.width('9') * 9 + fm.width(':') * 3 + fw * 4;
      editor->view->setColumnWidth(COL_SAMPLERATE, w);
      editor->view->setColumnWidth(COL_LEN, w);

      connect(editor->view, SIGNAL(itemSelectionChanged()), SLOT(clipSelectionChanged()));
      connect(editor->view, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(clicked(QTreeWidgetItem*, int)));

      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(editor->start, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(startChanged(const MusECore::Pos&)));
      connect(editor->len, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(lenChanged(const MusECore::Pos&)));

      updateList();
      finalizeInit();
      }

ClipListEdit::~ClipListEdit()
{

}

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void ClipListEdit::updateList()
      {
      editor->view->clear();
      for (MusECore::iSndFile f = MusECore::SndFile::sndFiles.begin(); f != MusECore::SndFile::sndFiles.end(); ++f) {
            new ClipItem(editor->view, *f);
            }
      clipSelectionChanged();
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void ClipListEdit::closeEvent(QCloseEvent* e)
      {
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ClipListEdit::songChanged(int type)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(type == SC_MIDI_CONTROLLER)
        return;
    
      updateList();
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ClipListEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
                  break;
            switch (token) {
                  case MusECore::Xml::TagStart:
                        if (tag == "topwin")
                              TopWin::readStatus(xml);
                        else
                              xml.unknown("CliplistEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "cliplist")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void ClipListEdit::writeStatus(int level, MusECore::Xml& xml) const
      {
      xml.tag(level++, "cliplist");
      TopWin::writeStatus(level, xml);
      xml.etag(level, "cliplist");
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void ClipListEdit::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "topwin")
                              TopWin::readConfiguration(CLIPLIST, xml);
                        else
                              xml.unknown("ClipListEdit");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "cliplistedit")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void ClipListEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "cliplistedit");
      TopWin::writeConfiguration(CLIPLIST, level, xml);
      xml.tag(level, "/cliplistedit");
      }

//---------------------------------------------------------
//   startChanged
//---------------------------------------------------------

void ClipListEdit::startChanged(const MusECore::Pos& /*pos*/)//prevent compiler warning: unsused parameter
      {
//      editor->view->triggerUpdate(); DELETETHIS whole function?
      }

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void ClipListEdit::lenChanged(const MusECore::Pos& /*pos*/) //prevent compiler warning: unsused parameter
      {
//      curClip.setLenFrame(pos.frame()); DELETETHIS whole function?
//      editor->view->triggerUpdate();
      }

//---------------------------------------------------------
//   clipSelectionChanged
//---------------------------------------------------------

void ClipListEdit::clipSelectionChanged()
      {
//      ClipItem* item = (ClipItem*)(editor->view->selectedItem());

//      if (item == 0) {
            editor->start->setEnabled(false);
            editor->len->setEnabled(false);
            return;
/* DELETETHIS and the above two comments
            }
      editor->start->setEnabled(true);
      editor->len->setEnabled(true);
      MusECore::Pos pos, len;
      pos.setType(MusECore::Pos::FRAMES);
      len.setType(MusECore::Pos::FRAMES);
      pos.setFrame(curClip.spos());
      len.setFrame(curClip.lenFrame());
      editor->start->setValue(pos);
      editor->len->setValue(len);
*/
      }

//---------------------------------------------------------
//   clicked
//---------------------------------------------------------

void ClipListEdit::clicked(QTreeWidgetItem*, int)
      {
//      printf("clicked\n"); DELETETHIS whole function
      }

} // namespace MusEGui
