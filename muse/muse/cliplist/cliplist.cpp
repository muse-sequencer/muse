//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cliplist.cpp,v 1.10 2005/09/22 20:12:59 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "cliplist.h"
#include "song.h"
#include "globals.h"
#include "wave.h"
#include "xml.h"
// #include "posedit.h"
#include "cliplisteditorbase.h"
//Added by qt3to4:
#include <QCloseEvent>

extern int mtcType;
enum { COL_NAME=0, COL_REFS, COL_POS, COL_LEN };

//---------------------------------------------------------
//   ClipItem
//---------------------------------------------------------

class ClipItem : public Q3ListViewItem {
      SndFileR _wf;

      virtual QString text(int) const;

   public:
      ClipItem(Q3ListView*, const SndFileR&);
      SndFileR* wf() { return &_wf; }
      };

ClipItem::ClipItem(Q3ListView* parent, const SndFileR& w)
   : Q3ListViewItem(parent), _wf(w)
      {
      }

//---------------------------------------------------------
//   samples2smpte
//---------------------------------------------------------

#if 0
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

//---------------------------------------------------------
//   ClipListEdit
//---------------------------------------------------------

ClipListEdit::ClipListEdit()
   : TopWin(0, "cliplist", Qt::WType_TopLevel /*|WDestructiveClose*/)
      {
      setCaption(tr("MusE: Clip List Editor"));

      editor = new ClipListEditorBase(this, "edit");
      setCentralWidget(editor);

      editor->view->setColumnAlignment(COL_REFS, AlignRight);

      QFontMetrics fm(editor->view->font());
      int fw = style().pixelMetric(QStyle::PM_DefaultFrameWidth, this);
      int w  = 2 + fm.width('9') * 9 + fm.width(':') * 3 + fw * 4;
      editor->view->setColumnAlignment(COL_POS, AlignRight);
      editor->view->setColumnWidth(COL_POS, w);
      editor->view->setColumnAlignment(COL_LEN, AlignRight);
      editor->view->setColumnWidth(COL_LEN, w);

      connect(editor->view, SIGNAL(selectionChanged()), SLOT(clipSelectionChanged()));
      connect(editor->view, SIGNAL(clicked(Q3ListViewItem*)), SLOT(clicked(Q3ListViewItem*)));

      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(editor->start, SIGNAL(valueChanged(const Pos&)), SLOT(startChanged(const Pos&)));
      connect(editor->len, SIGNAL(valueChanged(const Pos&)), SLOT(lenChanged(const Pos&)));

      updateList();
      }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void ClipListEdit::updateList()
      {
      editor->view->clear();
      for (iSndFile f = SndFile::sndFiles.begin(); f != SndFile::sndFiles.end(); ++f) {
            new ClipItem(editor->view, *f);
            }
      clipSelectionChanged();
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void ClipListEdit::closeEvent(QCloseEvent* e)
      {
      emit deleted((int)this);
      e->accept();
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void ClipListEdit::songChanged(int)
      {
      updateList();
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ClipListEdit::readStatus(QDomNode node)
      {
#if 0 //TODO2
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            if (token == Xml::Error || token == Xml::End)
                  break;
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "topwin")
                              TopWin::readStatus(xml);
                        else
                              xml.unknown("CliplistEdit");
                        break;
                  case Xml::TagEnd:
                        if (tag == "cliplist")
                              return;
                  default:
                        break;
                  }
            }
#endif
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void ClipListEdit::writeStatus(Xml& xml) const
      {
      xml.tag("cliplist");
      TopWin::writeStatus(xml);
      xml.etag("cliplist");
      }

//---------------------------------------------------------
//   startChanged
//---------------------------------------------------------

void ClipListEdit::startChanged(const Pos& pos)
      {
//      editor->view->triggerUpdate();
      }

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void ClipListEdit::lenChanged(const Pos& pos)
      {
//      curClip.setLenFrame(pos.frame());
//      editor->view->triggerUpdate();
      }

//---------------------------------------------------------
//   clipSelectionChanged
//---------------------------------------------------------

void ClipListEdit::clipSelectionChanged()
      {
      ClipItem* item = (ClipItem*)(editor->view->selectedItem());
//      if (item == 0) {
            editor->start->setEnabled(false);
            editor->len->setEnabled(false);
            return;
#if 0
            }
      editor->start->setEnabled(true);
      editor->len->setEnabled(true);
      Pos pos, len;
      pos.setType(Pos::FRAMES);
      len.setType(Pos::FRAMES);
      pos.setFrame(curClip.spos());
      len.setFrame(curClip.lenFrame());
      editor->start->setValue(pos);
      editor->len->setValue(len);
#endif
      }

//---------------------------------------------------------
//   clicked
//---------------------------------------------------------

void ClipListEdit::clicked(Q3ListViewItem*)
      {
//      printf("clicked\n");
      }

