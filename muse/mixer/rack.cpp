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
#include <q3popupmenu.h>
#include <qmessagebox.h>

#include <QByteArray>
#include <QMimeData>
#include <QDrag>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <errno.h>

#include "xml.h"
#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "plugin.h"
#include "filedialog.h"
#include "config.h"

//---------------------------------------------------------
//   class RackSlot
//---------------------------------------------------------

class RackSlot : public Q3ListBoxItem {
      int idx;
      AudioTrack* node;

      virtual void paint(QPainter*);
      virtual int height(const Q3ListBox*) const { return 18; }

   public:
      RackSlot(Q3ListBox* lb, AudioTrack* t, int);
      ~RackSlot();
      };

RackSlot::~RackSlot()
      {
      node = 0;
      }

//---------------------------------------------------------
//   RackSlot
//---------------------------------------------------------

RackSlot::RackSlot(Q3ListBox* b, AudioTrack* t, int i)
   : Q3ListBoxItem(b)
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
   : Q3ListBox(parent, "Rack")
      {
      setAttribute(Qt::WA_DeleteOnClose);
      track = t;
      setFont(config.fonts[1]);

      setHScrollBarMode(AlwaysOff);
      setVScrollBarMode(AlwaysOff);
      setSelectionMode(Single);
      setMaximumHeight(18 * PipelineDepth);
      for (int i = 0; i < PipelineDepth; ++i)
            new RackSlot(this, track, i);
      connect(this, SIGNAL(contextMenuRequested(Q3ListBoxItem*, const QPoint&)),
         this, SLOT(menuRequested(Q3ListBoxItem*, const QPoint&)));
      connect(this, SIGNAL(doubleClicked(Q3ListBoxItem*)),
         this, SLOT(doubleClicked(Q3ListBoxItem*)));
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
//   SizeHint
//---------------------------------------------------------

QSize EffectRack::sizeHint() const
      {
      return minimumSizeHint();
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::menuRequested(Q3ListBoxItem* it, const QPoint& pt)
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

      //enum { NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SAVE };
      enum { NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SHOW_NATIVE, SAVE };
      Q3PopupMenu* menu = new Q3PopupMenu;
      menu->insertItem(QIcon(*upIcon), tr("move up"),   UP, UP);
      menu->insertItem(QIcon(*downIcon), tr("move down"), DOWN, DOWN);
      menu->insertItem(tr("remove"),    REMOVE, REMOVE);
      menu->insertItem(tr("bypass"),    BYPASS, BYPASS);
      menu->insertItem(tr("show gui"),  SHOW, SHOW);
      menu->insertItem(tr("show native gui"),  SHOW_NATIVE, SHOW_NATIVE);

      menu->setItemChecked(BYPASS, !pipe->isOn(idx));
      menu->setItemChecked(SHOW, pipe->guiVisible(idx));
      menu->setItemChecked(SHOW_NATIVE, pipe->nativeGuiVisible(idx));

      if (pipe->empty(idx)) {
            menu->insertItem(tr("new"), NEW, NEW);
            menu->setItemEnabled(UP, false);
            menu->setItemEnabled(DOWN, false);
            menu->setItemEnabled(REMOVE, false);
            menu->setItemEnabled(BYPASS, false);
            menu->setItemEnabled(SHOW, false);
            menu->setItemEnabled(SHOW_NATIVE, false);
            menu->setItemEnabled(SAVE, false);
            }
      else {
            menu->insertItem(tr("change"), CHANGE, CHANGE);
            menu->insertItem(tr("save preset"),  SAVE, SAVE);
            if (idx == 0)
                  menu->setItemEnabled(UP, false);
            if (idx == (PipelineDepth-1))
                  menu->setItemEnabled(DOWN, false);
            if(!pipe->isDssiPlugin(idx))
                  menu->setItemEnabled(SHOW_NATIVE, false);
            }

      #ifndef OSC_SUPPORT
      menu->setItemEnabled(SHOW_NATIVE, false);
      #endif
      
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
            case SHOW_NATIVE:
                  {
                  bool flag = !pipe->nativeGuiVisible(idx);
                  pipe->showNativeGui(idx, flag);
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

void EffectRack::doubleClicked(Q3ListBoxItem* it)
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
      //QString name = getSaveFileName(QString(""), plug_file_pattern, this,
      QString name = getSaveFileName(QString(""), preset_file_save_pattern, this,
         tr("MusE: Save Preset"));
      
      if(name.isEmpty())
        return;
        
      //FILE* presetFp = fopen(name.ascii(),"w+");
      bool popenFlag;
      FILE* presetFp = fileOpen(this, name, QString(".pre"), "w", popenFlag, false, true);
      if (presetFp == 0) {
            //fprintf(stderr, "EffectRack::savePreset() fopen failed: %s\n",
            //   strerror(errno));
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
                //fclose(presetFp);
                if (popenFlag)
                      pclose(presetFp);
                else
                      fclose(presetFp);
                return;
                }
            }
      else {
          printf("no pipe!\n");
          //fclose(presetFp);
          if (popenFlag)
                pclose(presetFp);
          else
                fclose(presetFp);
          return;
          }
      //fclose(presetFp);
      if (popenFlag)
            pclose(presetFp);
      else
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
      
      QByteArray data(xmlconf.toLatin1().data());
      QMimeData* md = new QMimeData();
      
      md->setData("text/x-muse-plugin", data);
      
      QDrag* drag = new QDrag(this);
      drag->setMimeData(md);
      
      drag->exec(Qt::CopyAction);
      }

void EffectRack::contentsDropEvent(QDropEvent * /*event*/)// prevent of compiler warning: unsued variable
      {
      }

void EffectRack::dropEvent(QDropEvent *event)
      {
      QString text;
      Q3ListBoxItem *i = itemAt( contentsToViewport(event->pos()) );
      int idx = index(i);
      
      Pipeline* pipe = track->efxPipe();
      if (pipe) 
      {
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
                    Q3ListBoxItem *i = ser->itemAt(contentsToViewport(ser->getDragPos()));
                    int idx0 = ser->index(i);
                    if (!(*spipe)[idx0] || 
                        (idx == idx0 && (ser == this || ser->getTrack()->name() == track->name())))
                      return; 
                  }
                }
                if(QMessageBox::question(this, tr("Replace effect"),tr("Do you really want to replace the effect %1?").arg(pipe->name(idx)),
                      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                      {
                        audio->msgAddPlugin(track, idx, 0);
                        song->update(SC_RACK);
                      }
                else {
                      return;
                      }
                }
            
            if(event->mimeData()->hasFormat("text/x-muse-plugin"))
            {
              QString outxml;
              Xml xml(event->mimeData()->data("text/x-muse-plugin").data());
              initPlugin(xml, idx);
            }
            else
            if (event->mimeData()->hasUrls()) 
            {
              // Multiple urls not supported here. Grab the first one.
              text = event->mimeData()->urls()[0].path();
               
              if (text.endsWith(".pre", Qt::CaseInsensitive) || 
                  text.endsWith(".pre.gz", Qt::CaseInsensitive) || 
                  text.endsWith(".pre.bz2", Qt::CaseInsensitive))
              {
                  //bool popenFlag = false;
                  bool popenFlag;
                  FILE* fp = fileOpen(this, text, ".pre", "r", popenFlag, false, false);
                  if (fp) 
                  {
                      Xml xml(fp);
                      initPlugin(xml, idx);
                      
                      // Added by T356.
                      if (popenFlag)
                            pclose(fp);
                      else
                            fclose(fp);
                  }
              }
            }
      }
      }

void EffectRack::dragEnterEvent(QDragEnterEvent *event)
      {
      ///event->accept(Q3TextDrag::canDecode(event));
      event->acceptProposedAction();  // TODO CHECK Tim.
      }

void EffectRack::contentsDragEnterEvent(QDragEnterEvent * /*event*/)// prevent of compiler warning: unused parameter
      {
      }

void EffectRack::contentsMousePressEvent(QMouseEvent *event)
      {
      if(event->button() & Qt::LeftButton) {
          dragPos = event->pos();
      }
      Q3ListBox::contentsMousePressEvent(event);  
      }

void EffectRack::contentsMouseMoveEvent(QMouseEvent *event)
      {
      if (event->state() & Qt::LeftButton) {
            Pipeline* pipe = track->efxPipe();
            if(!pipe)
              return;
            Q3ListBoxItem *i = itemAt(contentsToViewport(dragPos));
            int idx0 = index(i);
            if (!(*pipe)[idx0])
              return; 
            
            int distance = (dragPos-event->pos()).manhattanLength();
            if (distance > QApplication::startDragDistance()) {
                  Q3ListBoxItem *i = itemAt( contentsToViewport(event->pos()) );
                  int idx = index(i);
                  startDrag(idx);
                  }
            }
      Q3ListBox::contentsMouseMoveEvent(event);  
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

