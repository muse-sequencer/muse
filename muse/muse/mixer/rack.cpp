//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.cpp,v 1.7.2.7 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <qapplication.h>
#include <qtooltip.h>
#include <qpalette.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qdragobject.h>
#include <qmessagebox.h>
#include <qurl.h>
#include <errno.h>

#include "xml.h"
#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "plugin.h"
#include "filedialog.h"

//---------------------------------------------------------
//   class RackSlot
//---------------------------------------------------------

class RackSlot : public QListBoxItem {
      int idx;
      AudioTrack* node;

      virtual void paint(QPainter*);
      virtual int height(const QListBox*) const { return 18; }

   public:
      RackSlot(QListBox* lb, AudioTrack* t, int);
      ~RackSlot();
      };

RackSlot::~RackSlot()
      {
      node = 0;
      }

//---------------------------------------------------------
//   RackSlot
//---------------------------------------------------------

RackSlot::RackSlot(QListBox* b, AudioTrack* t, int i)
   : QListBoxItem(b)
      {
      node = t;
      idx  = i;
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void RackSlot::paint(QPainter* painter)
      {
      if (node == 0)
            return;
      painter->save();
      if (node == 0 || !node->efxPipe()->isOn(idx)) {
            const QColorGroup& g = listBox()->colorGroup();
            painter->fillRect(0,0,listBox()->width(),height(listBox()), g.dark());
            painter->setPen(g.light());
            }
      QFontMetrics fm = painter->fontMetrics();
      QString s(node->efxPipe()->name(idx));
      painter->drawText(3, fm.ascent() + fm.leading()/2, s);
      painter->restore();
      }

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, AudioTrack* t)
   : QListBox(parent, "Rack", Qt::WDestructiveClose)
      {
      track = t;
      setFont(config.fonts[1]);

      setHScrollBarMode(AlwaysOff);
      setVScrollBarMode(AlwaysOff);
      setSelectionMode(Single);
      setMaximumHeight(18 * PipelineDepth);
      for (int i = 0; i < PipelineDepth; ++i)
            new RackSlot(this, track, i);
      connect(this, SIGNAL(contextMenuRequested(QListBoxItem*, const QPoint&)),
         this, SLOT(menuRequested(QListBoxItem*, const QPoint&)));
      connect(this, SIGNAL(doubleClicked(QListBoxItem*)),
         this, SLOT(doubleClicked(QListBoxItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      QToolTip::add(this, tr("effect rack"));
      setAcceptDrops(true);
      }

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::~EffectRack()
      {
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void EffectRack::songChanged(int typ)
      {
      if (typ & (SC_ROUTE | SC_RACK)) {
            for (int i = 0; i < PipelineDepth; ++i)
                  updateItem(i);
            }
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize EffectRack::minimumSizeHint() const
      {
      return QSize(10, 18 * PipelineDepth);
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::menuRequested(QListBoxItem* it, const QPoint& pt)
      {
      if (it == 0 || track == 0)
            return;
      RackSlot* curitem = (RackSlot*)it;
      int idx = index(curitem);
      QString name;
      bool mute;
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            name  = pipe->name(idx);
            mute  = pipe->isOn(idx);
            }

      enum { NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SAVE };
      QPopupMenu* menu = new QPopupMenu;
      menu->insertItem(QIconSet(*upIcon), tr("move up"),   UP, UP);
      menu->insertItem(QIconSet(*downIcon), tr("move down"), DOWN, DOWN);
      menu->insertItem(tr("remove"),    REMOVE, REMOVE);
      menu->insertItem(tr("bypass"),    BYPASS, BYPASS);
      menu->insertItem(tr("show gui"),  SHOW, SHOW);

      menu->setItemChecked(BYPASS, !pipe->isOn(idx));
      menu->setItemChecked(SHOW, pipe->guiVisible(idx));

      if (pipe->empty(idx)) {
            menu->insertItem(tr("new"), NEW, NEW);
            menu->setItemEnabled(UP, false);
            menu->setItemEnabled(DOWN, false);
            menu->setItemEnabled(REMOVE, false);
            menu->setItemEnabled(BYPASS, false);
            menu->setItemEnabled(SHOW, false);
            menu->setItemEnabled(SAVE, false);
            }
      else {
            menu->insertItem(tr("change"), CHANGE, CHANGE);
            menu->insertItem(tr("save preset"),  SAVE, SAVE);
            if (idx == 0)
                  menu->setItemEnabled(UP, false);
            if (idx == (PipelineDepth-1))
                  menu->setItemEnabled(DOWN, false);
            }

      int sel = menu->exec(pt, 1);
      delete menu;
      if (sel == -1)
            return;

      switch(sel) {
            case NEW:
                  {
                  Plugin* plugin = PluginDialog::getPlugin(this);
                  if (plugin) {
                        PluginI* plugi = new PluginI();
                        if (plugi->initPluginInstance(plugin, track->channels())) {
                              printf("cannot instantiate plugin <%s>\n",
                                 plugin->name().latin1());
                              delete plugi;
                              break;
                              }
                        audio->msgAddPlugin(track, idx, plugi);
                        }
                  break;
                  }
            case CHANGE:
                  {
                  Plugin* plugin = PluginDialog::getPlugin(this);
                  if (plugin) {
                        PluginI* plugi = new PluginI();
                        if (plugi->initPluginInstance(plugin, track->channels())) {
                              printf("cannot instantiate plugin <%s>\n",
                                 plugin->name().latin1());
                              delete plugi;
                              break;
                              }
                        audio->msgAddPlugin(track, idx, 0);
                        audio->msgAddPlugin(track, idx, plugi);
                        }
                  break;
                  }
            case REMOVE:
                  audio->msgAddPlugin(track, idx, 0);
                  break;
            case BYPASS:
                  {
                  bool flag = !pipe->isOn(idx);
                  pipe->setOn(idx, flag);
                  break;
                  }
            case SHOW:
                  {
                  bool flag = !pipe->guiVisible(idx);
                  pipe->showGui(idx, flag);
                  break;
                  }
            case UP:
                  if (idx > 0) {
                        setCurrentItem(idx-1);
                        pipe->move(idx, true);
                        }
                  break;
            case DOWN:
                  if (idx < (PipelineDepth-1)) {
                        setCurrentItem(idx+1);
                        pipe->move(idx, false);
                        }
                  break;
            case SAVE:
                  savePreset(idx);
                  break;
            }
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   doubleClicked
//    toggle gui
//---------------------------------------------------------

void EffectRack::doubleClicked(QListBoxItem* it)
      {
      if (it == 0 || track == 0)
            return;
      RackSlot* item = (RackSlot*)it;
      int idx        = index(item);
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            bool flag = !pipe->guiVisible(idx);
            pipe->showGui(idx, flag);
            }
      }

void EffectRack::savePreset(int idx)
      {
      QString name = getSaveFileName(QString(""), plug_file_pattern, this,
         tr("MusE: Save Preset"));
      FILE* presetFp = fopen(name.ascii(),"w+");
      if (presetFp == 0) {
            fprintf(stderr, "EffectRack::savePreset() fopen failed: %s\n",
               strerror(errno));
            return;
            }
      Xml xml(presetFp);
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != NULL) {
                xml.header();
                xml.tag(0, "muse version=\"1.0\"");
                (*pipe)[idx]->writeConfiguration(1, xml);
                xml.tag(0, "/muse");
                }
            else {
                printf("no plugin!\n");
                fclose(presetFp);
                return;
                }
            }
      else {
          printf("no pipe!\n");
          fclose(presetFp);
          return;
          }
      fclose(presetFp);
      }

void EffectRack::startDrag(int idx)
      {
      FILE* tmp = tmpfile();
      if (tmp == 0) {
            fprintf(stderr, "EffectRack::startDrag fopen failed: %s\n",
               strerror(errno));
            return;
            }
      Xml xml(tmp);
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != NULL) {
                xml.header();
                xml.tag(0, "muse version=\"1.0\"");
                (*pipe)[idx]->writeConfiguration(1, xml);
                xml.tag(0, "/muse");
                }
            else {
                //printf("no plugin!\n");
                return;
                }
            }
      else {
          //printf("no pipe!\n");
          return;
          }
      
      QString xmlconf;
      xml.dump(xmlconf);
      QTextDrag *drag = new QTextDrag(xmlconf, this);
      drag->setSubtype("x-muse-plugin");
      drag->drag();
      }

void EffectRack::contentsDropEvent(QDropEvent * /*event*/)// prevent of compiler warning: unsued variable
      {
      }

void EffectRack::dropEvent(QDropEvent *event)
      {
      QString text;
      QListBoxItem *i = itemAt( contentsToViewport(event->pos()) );
      int idx = index(i);
      
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != NULL) {
                QWidget *sw = event->source();
                if(sw)
                {
                  if(strcmp(sw->className(), "EffectRack") == 0) 
                  { 
                    EffectRack *ser = (EffectRack*)sw;
                    Pipeline* spipe = ser->getTrack()->efxPipe();
                    if(!spipe)
                      return;
                    QListBoxItem *i = ser->itemAt(contentsToViewport(ser->getDragPos()));
                    int idx0 = ser->index(i);
                    if (!(*spipe)[idx0] || 
                        (idx == idx0 && (ser == this || ser->getTrack()->name() == track->name())))
                      return; 
                  }
                }
                if(!QMessageBox::question(this, tr("Replace effect"),tr("Do you really want to replace the effect %1?").arg(pipe->name(idx)),
                      tr("&Yes"), tr("&No"),
                      QString::null, 0, 1 ))
                      {
                      audio->msgAddPlugin(track, idx, 0);
                      song->update(SC_RACK);
                      }
                else {
                      return;
                      }
                }
            if(QTextDrag::decode(event, text))
                {
                text = text.stripWhiteSpace();
                if (text.endsWith(".pre", false))
                    {
                    QUrl url(text);
                    QString newPath = url.path();
      
                    bool popenFlag = false;
                    FILE* fp = fileOpen(this, newPath, ".pre", "r", popenFlag, false, false);
              
                    if (fp) {
                        Xml xml(fp);
                        initPlugin(xml, idx);
                        }
                    }
                else if (event->provides("text/x-muse-plugin"))
                      {
                        QString outxml;
                        QTextDrag::decode(event, outxml);
                        Xml xml(outxml);
                        initPlugin(xml, idx);
                      }
                }
           }
      }

void EffectRack::dragEnterEvent(QDragEnterEvent *event)
      {
      event->accept(QTextDrag::canDecode(event));
      }


void EffectRack::contentsDragEnterEvent(QDragEnterEvent * /*event*/)// prevent of compiler warning: unused parameter
      {
      }

void EffectRack::contentsMousePressEvent(QMouseEvent *event)
      {
      if(event->button() & LeftButton) {
          dragPos = event->pos();
      }
      QListBox::contentsMousePressEvent(event);  
      }

void EffectRack::contentsMouseMoveEvent(QMouseEvent *event)
      {
      if (event->state() & LeftButton) {
            Pipeline* pipe = track->efxPipe();
            if(!pipe)
              return;
            QListBoxItem *i = itemAt(contentsToViewport(dragPos));
            int idx0 = index(i);
            if (!(*pipe)[idx0])
              return; 
            
            int distance = (dragPos-event->pos()).manhattanLength();
            if (distance > QApplication::startDragDistance()) {
                  QListBoxItem *i = itemAt( contentsToViewport(event->pos()) );
                  int idx = index(i);
                  startDrag(idx);
                  }
            }
      QListBox::contentsMouseMoveEvent(event);  
      }


void EffectRack::initPlugin(Xml xml, int idx)
      {      
      for (;;) {
            Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "plugin") {
                              PluginI* plugi = new PluginI();
                              if (plugi->readConfiguration(xml, false)) {
                                  printf("cannot instantiate plugin\n");
                                  delete plugi;
                                  }
                              else {
                                  //printf("instantiated!\n");
                                  audio->msgAddPlugin(track, idx, plugi);
                                  song->update(SC_RACK);
                                  return;
                                  }
                              }
                        else if (tag =="muse")
                              break;
                        else
                              xml.unknown("EffectRack");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "muse")
                              return;
                  default:
                        break;
                  }
            }
      }                        

