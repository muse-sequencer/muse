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
//#include "ui_cliplisteditorbase.h"
#include "app.h"
#include "wavepreview.h"
#include "helper.h"

namespace MusEGui {

extern int mtcType;
// REMOVE Tim. wave. Changed.
// enum { COL_NAME=0, COL_REFS, COL_SAMPLERATE, COL_LEN, COL_DATA, COL_STATUS };
enum { COL_NAME=0, COL_SAMPLERATE, COL_SAMPLES, COL_CLIP_POS, COL_CLIP_LEN };

enum { 
  CLIPLIST_ROLE_ID = Qt::UserRole,
  CLIPLIST_ROLE_SAMPLERATE,
  CLIPLIST_ROLE_SAMPLES,
  CLIPLIST_ROLE_CLIP_POS,
  CLIPLIST_ROLE_CLIP_LEN
};

// REMOVE Tim. wave. Added.
MusECore::EventList ClipListEdit::waveClipEvents;

//---------------------------------------------------------
//   ClipItem
//---------------------------------------------------------

class ClipItem : public QTreeWidgetItem {
//       MusECore::SndFileR _wf;
      MusECore::Event _event;

      //virtual QString text(int) const;

   public:
//       ClipItem(QTreeWidget*, const MusECore::SndFileR&);
      ClipItem(QTreeWidget* parent, const MusECore::Event& e) : QTreeWidgetItem(parent), _event(e) { updateItem(); }

//       MusECore::SndFileR* wf() { return &_wf; }
      MusECore::Event* event() { return &_event; };

      void updateItem(MusECore::Event* pe = nullptr)
      {
        if(pe)
          _event = *pe;

        MusECore::SndFileR sndfile = _event.sndFile();
        setText(COL_NAME,       sndfile.path());

        setText(COL_SAMPLERATE, QString().setNum(sndfile.samplerate()));
        setData(COL_SAMPLERATE, CLIPLIST_ROLE_SAMPLERATE, sndfile.samplerate());

        // Getting samples can be time-consuming.
        const sf_count_t sz = sndfile.samples();
        setText(COL_SAMPLES, QString().setNum(sz));
        setData(COL_SAMPLES, CLIPLIST_ROLE_SAMPLES, (long long)sz);

        setText(COL_CLIP_POS, QString().setNum(_event.spos()));
        setData(COL_CLIP_POS, CLIPLIST_ROLE_CLIP_POS, _event.spos());

        setText(COL_CLIP_LEN, QString().setNum(_event.lenValue()));
        setData(COL_CLIP_LEN, CLIPLIST_ROLE_CLIP_LEN, _event.lenValue());
      }
      
      };

// // ClipItem::ClipItem(QTreeWidget* parent, const MusECore::SndFileR& sndfile)
// //    : QTreeWidgetItem(parent), _wf(sndfile)
// ClipItem::ClipItem(QTreeWidget* parent, const MusECore::Event& e)
// //    : QTreeWidgetItem(parent), _wf(e.sndFile())
//    : QTreeWidgetItem(parent), _event(e)
//       {
//         MusECore::SndFileR sndfile = e.sndFile();
// //         setText(COL_NAME,       sndfile.name());
//         setText(COL_NAME,       sndfile.path());
// 
// //         setText(COL_REFS,       QString().setNum(_wf.getRefCount()));
// 
//         setText(COL_SAMPLERATE, QString().setNum(sndfile.samplerate()));
//         setData(COL_SAMPLERATE, CLIPLIST_ROLE_SAMPLERATE, sndfile.samplerate());
// 
//         // Getting samples can be time-consuming.
//         const sf_count_t sz = sndfile.samples();
//         setText(COL_SAMPLES, QString().setNum(sz));
//         setData(COL_SAMPLES, CLIPLIST_ROLE_SAMPLES, (long long)sz);
// 
//         setText(COL_CLIP_POS, QString().setNum(e.spos()));
//         setData(COL_CLIP_POS, CLIPLIST_ROLE_CLIP_POS, e.spos());
// 
//         setText(COL_CLIP_LEN, QString().setNum(e.lenValue()));
//         setData(COL_CLIP_LEN, CLIPLIST_ROLE_CLIP_LEN, e.lenValue());
// 
// //         setText(COL_STATUS, _wf.isOpen() ? QObject::tr("Open") : QObject::tr("Closed"));
//       }

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

      _curItem = nullptr;
      editor = new ClipListEditorBaseWidget;
      setCentralWidget(editor);

            
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);      
      settingsMenu->addAction(shareAction);      
      settingsMenu->addAction(fullscreenAction);      
      
      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      QFontMetrics fm(editor->view->font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth,0, this); // ddskrjo 0
// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
      int w  = 2 + fm.horizontalAdvance('9') * 9 + fm.horizontalAdvance(':') * 3 + fw * 4;
#else
      int w  = 2 + fm.width('9') * 9 + fm.width(':') * 3 + fw * 4;
#endif
      editor->view->setColumnWidth(COL_SAMPLERATE, w);
      editor->view->setColumnWidth(COL_SAMPLES, w);
      editor->view->setColumnWidth(COL_CLIP_LEN, w);
      editor->view->setColumnWidth(COL_CLIP_POS, w);

      connect(editor->view, SIGNAL(itemSelectionChanged()), SLOT(clipSelectionChanged()));
      connect(editor->view, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(clicked(QTreeWidgetItem*, int)));

      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(editor->start, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(startChanged(const MusECore::Pos&)));
      connect(editor->len, SIGNAL(valueChanged(const MusECore::Pos&)), SLOT(lenChanged(const MusECore::Pos&)));

      connect(editor->addClipButton, SIGNAL(clicked()), SLOT(addClipClicked()));
      connect(editor->deleteClipButton, SIGNAL(clicked()), SLOT(deleteClipClicked()));
      connect(editor->closeButton, SIGNAL(clicked()), SLOT(closeClicked()));
      
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


// REMOVE Tim. wave. Changed.
//       for (MusECore::iSndFile f = MusECore::SndFile::sndFiles.begin(); f != MusECore::SndFile::sndFiles.end(); ++f) {
//             new ClipItem(editor->view, *f);
//             }

//       // Show what's already in use in the song...
//       for(MusECore::ciWaveTrack it = MusEGlobal::song->waves()->begin(); it != MusEGlobal::song->waves()->end(); ++it)
//       {
//         const MusECore::WaveTrack* track = *it;
//         for(MusECore::ciPart ip = track->cparts()->begin(); ip != track->cparts()->end(); ++ip)
//         {
//           const MusECore::Part* part = ip->second;
//           for(MusECore::ciEvent ie = part->events().begin(); ie != part->events().end(); ++ie)
//           {
//             const MusECore::Event& e = ie->second;
//             if(e.type() != MusECore::Wave)
//               continue;
//             const MusECore::SndFileR sf = e.sndFile();
//             
//             
//             QList<QTreeWidgetItem*> found = editor->view->findItems(sf.path(), Qt::MatchExactly);
//             if(!found.isEmpty())
//               new ClipItem(editor->view, sf);
//           }
//         }
//       }


// REMOVE Tim. wave. Added.
      for(MusECore::ciEvent ie = waveClipEvents.begin(); ie != waveClipEvents.end(); ++ie)
      {
        const MusECore::Event& e = ie->second;
        if(e.type() != MusECore::Wave)
          continue;
        const MusECore::SndFileR sf = e.sndFile();
        
        QList<QTreeWidgetItem*> found = editor->view->findItems(sf.path(), Qt::MatchExactly);
        if(!found.isEmpty())
        {
//           /*ClipItem* item =*/ new ClipItem(editor->view, sf);
          /*ClipItem* item =*/ new ClipItem(editor->view, e);
          //item->setData(0, Qt::UserRole, (long long)e.id());
        }
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

void ClipListEdit::songChanged(MusECore::SongChangedStruct_t type)
      {
      if(type & (SC_CLIP_MODIFIED | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED))
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

void ClipListEdit::startChanged(const MusECore::Pos& pos)
      {
//      editor->view->triggerUpdate(); DELETETHIS whole function?
        if(!_curItem)
          return;
        _curItem->event()->setSpos(pos.posValue());
        _curItem->updateItem();
      }

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void ClipListEdit::lenChanged(const MusECore::Pos& pos) //prevent compiler warning: unsused parameter
      {
//      curClip.setLenFrame(pos.frame()); DELETETHIS whole function?
//      editor->view->triggerUpdate();
        if(!_curItem)
          return;
        _curItem->event()->setLenValue(pos.posValue());
        _curItem->updateItem();
      }

//---------------------------------------------------------
//   clipSelectionChanged
//---------------------------------------------------------

void ClipListEdit::clipSelectionChanged()
      {
// REMOVE Tim. wave. Added. Diagnostics.
      fprintf(stderr, "clipSelectionChanged\n");

//      ClipItem* item = nullptr;
      //QTreeWidgetItem* item = nullptr;
      _curItem = nullptr;
      
      QList<QTreeWidgetItem *> sel_list = editor->view->selectedItems();

//      ClipItem* item = (ClipItem*)(editor->view->currentItem());

//      if (item == 0) {
      if(sel_list.isEmpty() || !(_curItem = static_cast<ClipItem*>(sel_list.at(0))))
      {
            editor->start->setEnabled(false);
            editor->len->setEnabled(false);
            return;
      }
      
      editor->start->setEnabled(true);
      editor->len->setEnabled(true);
      
      MusECore::Pos pos, len;
      pos.setType(MusECore::Pos::FRAMES);
      len.setType(MusECore::Pos::FRAMES);
//       pos.setFrame(curClip.spos());
//       len.setFrame(curClip.lenFrame());
      
      const int clip_pos = _curItem->data(COL_CLIP_POS, CLIPLIST_ROLE_CLIP_POS).toInt();
      const unsigned int clip_len = _curItem->data(COL_CLIP_LEN, CLIPLIST_ROLE_CLIP_LEN).toUInt();
        
      pos.setFrame(clip_pos);
      len.setFrame(clip_len);
      editor->start->setValue(pos);
      editor->len->setValue(len);
      
      }

//---------------------------------------------------------
//   clicked
//---------------------------------------------------------

void ClipListEdit::clicked(QTreeWidgetItem*, int)
      {
//      printf("clicked\n"); DELETETHIS whole function
// REMOVE Tim. wave. Added. Diagnostics.
     fprintf(stderr, "clicked\n");
      }

void ClipListEdit::addClipClicked()
{
  MusECore::SndFileR sndfile = importWave();
  if(sndfile.isNull())
  {
    //sndfile.close();
    return;
  }

  for(MusECore::ciEvent ie = waveClipEvents.begin(); ie != waveClipEvents.end(); ++ie)
  {
    const MusECore::Event& e = ie->second;
    if(e.type() != MusECore::Wave)
      continue;
    const MusECore::SndFileR sf = e.sndFile();
    if(sf.path() == sndfile.path())
    {
      sndfile.close();
      return;
    }
  }
  
  MusECore::Event new_ev(MusECore::Wave);
  new_ev.setSndFile(sndfile);
  new_ev.setSpos(0);
  new_ev.setLenFrame(sndfile.samples());
  
  waveClipEvents.add(new_ev);
  
//   new ClipItem(editor->view, sndfile);
  new ClipItem(editor->view, new_ev);
  
  sndfile.close();
  
  clipSelectionChanged();
}

void ClipListEdit::deleteClipClicked()
{
  
}

void ClipListEdit::closeClicked()
{
  close();
}

MusECore::SndFileR ClipListEdit::importWave()
{
   MusECore::AudioPreviewDialog afd(this, MusEGlobal::sampleRate);
   afd.setDirectory(MusEGlobal::lastWavePath);
   afd.setWindowTitle(tr("Import Audio File"));
   /*QString fn = afd.getOpenFileName(MusEGlobal::lastWavePath, MusEGlobal::audio_file_pattern, this,
         tr("Import Audio File"), 0);
*/
   if(afd.exec() == QFileDialog::Rejected)
   {
      return MusECore::SndFileR();
   }

   QStringList filenames = afd.selectedFiles();
   if(filenames.size() < 1)
   {
      return MusECore::SndFileR();
   }
   QString fn = filenames [0];

   if (!fn.isEmpty()) {
      MusEGlobal::lastWavePath = fn;
      // Open it as well...
      return MusECore::getWave(fn, true, true);
   }
   
   return MusECore::SndFileR();
}

} // namespace MusEGui
