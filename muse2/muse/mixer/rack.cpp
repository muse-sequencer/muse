//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.cpp,v 1.7.2.7 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <QByteArray>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QUrl>

#include <errno.h>

#include "xml.h"
#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "globaldefs.h"
#include "plugin.h"
#include "filedialog.h"

//---------------------------------------------------------
//   class RackSlot
//---------------------------------------------------------

class RackSlot : public QListWidgetItem {
      int idx;
      AudioTrack* node;

   public:
      RackSlot(QListWidget* lb, AudioTrack* t, int i);
      ~RackSlot();
      void setBackgroundColor(const QBrush& brush) {setBackground(brush);};
      };

RackSlot::~RackSlot()
      {
      node = 0;
      }

//---------------------------------------------------------
//   RackSlot
//---------------------------------------------------------

RackSlot::RackSlot(QListWidget* b, AudioTrack* t, int i)
   : QListWidgetItem(b)
      {
      node = t;
      idx  = i;
      setSizeHint(QSize(10,17));
      }

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, AudioTrack* t)
   : QListWidget(parent)
      {
      setObjectName("Rack");
      setAttribute(Qt::WA_DeleteOnClose);
      track = t;
      setFont(config.fonts[1]);

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setSelectionMode(QAbstractItemView::SingleSelection);
      setMaximumHeight(19 * PipelineDepth);
      for (int i = 0; i < PipelineDepth; ++i)
            new RackSlot(this, track, i);
      updateContents();

      connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
         this, SLOT(doubleClicked(QListWidgetItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      setSpacing(0);
      QPalette qpal;
      qpal.setColor(QPalette::Base, QColor(palette().midlight().color()));
      setPalette(qpal);

      setAcceptDrops(true);
      }

void EffectRack::updateContents()
      {
	for (int i = 0; i < PipelineDepth; ++i) {
              QString name = track->efxPipe()->name(i);
              item(i)->setText(name);
              item(i)->setBackground(track->efxPipe()->isOn(i) ? palette().mid() : palette().dark());
              item(i)->setToolTip(name == QString("empty") ? tr("effect rack") : name );
	}
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
            updateContents();
       	    }
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize EffectRack::minimumSizeHint() const
      {
      return QSize(10, 19 * PipelineDepth);
      }

//---------------------------------------------------------
//   SizeHint
//---------------------------------------------------------

QSize EffectRack::sizeHint() const
      {
      return minimumSizeHint();
      }


void EffectRack::choosePlugin(QListWidgetItem* it, bool replace)
      {
      Plugin* plugin = PluginDialog::getPlugin(this);
      if (plugin) {
            PluginI* plugi = new PluginI();
            if (plugi->initPluginInstance(plugin, track->channels())) {
                  printf("cannot instantiate plugin <%s>\n",
                      plugin->name().toLatin1().constData());
                  delete plugi;
                  return;
                  }
            int idx = row(it);
	    if (replace)
                  audio->msgAddPlugin(track, idx, 0);
            audio->msgAddPlugin(track, idx, plugi);
            updateContents();
            }
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::menuRequested(QListWidgetItem* it)
      {
      if (it == 0 || track == 0)
            return;
      RackSlot* curitem = (RackSlot*)it;
      int idx = row(curitem);
      QString name;
      bool mute;
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            name  = pipe->name(idx);
            mute  = pipe->isOn(idx);
            }

      //enum { NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SAVE };
      enum { NEW, CHANGE, UP, DOWN, REMOVE, BYPASS, SHOW, SHOW_NATIVE, SAVE };
      QMenu* menu = new QMenu;
      QAction* newAction = menu->addAction(tr("new"));
      QAction* changeAction = menu->addAction(tr("change"));
      QAction* upAction = menu->addAction(QIcon(*upIcon), tr("move up"));//,   UP, UP);
      QAction* downAction = menu->addAction(QIcon(*downIcon), tr("move down"));//, DOWN, DOWN);
      QAction* removeAction = menu->addAction(tr("remove"));//,    REMOVE, REMOVE);
      QAction* bypassAction = menu->addAction(tr("bypass"));//,    BYPASS, BYPASS);
      QAction* showGuiAction = menu->addAction(tr("show gui"));//,  SHOW, SHOW);
      QAction* showNativeGuiAction = menu->addAction(tr("show native gui"));//,  SHOW_NATIVE, SHOW_NATIVE);
      QAction* saveAction = menu->addAction(tr("save preset"));

      newAction->setData(NEW);
      changeAction->setData(CHANGE);
      upAction->setData(UP);
      downAction->setData(DOWN);
      removeAction->setData(REMOVE);
      bypassAction->setData(BYPASS);
      showGuiAction->setData(SHOW);
      showNativeGuiAction->setData(SHOW_NATIVE);
      saveAction->setData(SAVE);

      bypassAction->setCheckable(true);
      showGuiAction->setCheckable(true);
      showNativeGuiAction->setCheckable(true);

      bypassAction->setChecked(!pipe->isOn(idx));
      showGuiAction->setChecked(pipe->guiVisible(idx));
      showNativeGuiAction->setChecked(pipe->nativeGuiVisible(idx));

      if (pipe->empty(idx)) {
            menu->removeAction(changeAction);
            menu->removeAction(saveAction);
            upAction->setEnabled(false);
            downAction->setEnabled(false);
            removeAction->setEnabled(false);
            bypassAction->setEnabled(false);
            showGuiAction->setEnabled(false);
            showNativeGuiAction->setEnabled(false);
            }
      else {
            menu->removeAction(newAction);
            if (idx == 0)
                  upAction->setEnabled(true);
            if (idx == (PipelineDepth-1))
                  downAction->setEnabled(false);
            //if(!pipe->isDssiPlugin(idx))
            if(!pipe->has_dssi_ui(idx))     // p4.0.19 Tim.
                  showNativeGuiAction->setEnabled(false);
            }

      #ifndef OSC_SUPPORT
      showNativeGuiAction->setEnabled(false);
      #endif

      QPoint pt = QCursor::pos();
      QAction* act = menu->exec(pt, 0);

      //delete menu;
      if (!act)
      {
        delete menu;
        return;
      }      
      
      int sel = act->data().toInt();
      delete menu;
      
      switch(sel) {
            case NEW:
                  {
                  choosePlugin(it);
                  break;
                  }
            case CHANGE:
                  {
                  choosePlugin(it, true);
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
                        setCurrentItem(item(idx-1));
                        pipe->move(idx, true);
                        }
                  break;
            case DOWN:
                  if (idx < (PipelineDepth-1)) {
                        setCurrentItem(item(idx+1));
                        pipe->move(idx, false);
                        }
                  break;
            case SAVE:
                  savePreset(idx);
                  break;
            }
      updateContents();
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   doubleClicked
//    toggle gui
//---------------------------------------------------------

void EffectRack::doubleClicked(QListWidgetItem* it)
      {
      if (it == 0 || track == 0)
            return;

      RackSlot* item = (RackSlot*)it;
      int idx        = row(item);
      Pipeline* pipe = track->efxPipe();

      if (pipe->name(idx) == QString("empty")) {
            choosePlugin(it);
            return;
            }
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
        if (idx < 0) {
            printf("illegal to drag index %d\n",idx);
            return;
        }
      FILE *tmp;
      if (debugMsg) {
          QString fileName;
          getUniqueTmpfileName("tmp","preset", fileName);
          tmp = fopen(fileName.toLatin1().data(), "w+");
      }
      else
          tmp = tmpfile();
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
      
      QByteArray data(xmlconf.toLatin1().constData());
      //printf("sending %d [%s]\n", data.length(), xmlconf.toLatin1().constData());
      QMimeData* md = new QMimeData();
      
      md->setData("text/x-muse-plugin", data);
      
      QDrag* drag = new QDrag(this);
      drag->setMimeData(md);
      
      drag->exec(Qt::CopyAction);
      }

Qt::DropActions EffectRack::supportedDropActions () const
      {
    return Qt::CopyAction | Qt::MoveAction;
      }

QStringList EffectRack::mimeTypes() const
      {
      QStringList mTypes;
      mTypes << "text/uri-list";
      mTypes << "text/x-muse-plugin";
      return mTypes;
      }

void EffectRack::dropEvent(QDropEvent *event)
      {
      QString text;
      QListWidgetItem *i = itemAt( event->pos() );
      if (!i)
            return;
      int idx = row(i);
      
      Pipeline* pipe = track->efxPipe();
      if (pipe) 
      {
            if ((*pipe)[idx] != NULL) {
                QWidget *sw = event->source();
                if(sw)
                {
                  if(strcmp(sw->metaObject()->className(), "EffectRack") == 0) 
                  { 
                    EffectRack *ser = (EffectRack*)sw;
                    Pipeline* spipe = ser->getTrack()->efxPipe();
                    if(!spipe)
                      return;

                    QListWidgetItem *i = ser->itemAt(ser->getDragPos());
                    int idx0 = ser->row(i);
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
              char *tmpStr = new char[event->mimeData()->data("text/x-muse-plugin").size()];
              strcpy(tmpStr, event->mimeData()->data("text/x-muse-plugin").data());
              Xml xml(tmpStr);
              initPlugin(xml, idx);
              delete tmpStr;
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
          event->acceptProposedAction();  // TODO CHECK Tim.
      }

void EffectRack::mousePressEvent(QMouseEvent *event)
      {
      if(event->button() & Qt::LeftButton) {
          dragPos = event->pos();
      }
      else if(event->button() & Qt::RightButton) {
          menuRequested(itemAt(event->pos()));
          return;
      }
      else if(event->button() & Qt::MidButton) {
          int idx = row(itemAt(event->pos()));
          bool flag = !track->efxPipe()->isOn(idx);
          track->efxPipe()->setOn(idx, flag);
          updateContents();
      }

      QListWidget::mousePressEvent(event);  
      }

void EffectRack::mouseMoveEvent(QMouseEvent *event)
{
      if (event->buttons() & Qt::LeftButton) {
            Pipeline* pipe = track->efxPipe();
            if(!pipe)
              return;

            QListWidgetItem *i = itemAt(dragPos);
            int idx0 = row(i);
            if (!(*pipe)[idx0])
              return; 
            
            int distance = (dragPos-event->pos()).manhattanLength();
            if (distance > QApplication::startDragDistance()) {
                  QListWidgetItem *i = itemAt( event->pos() );
                  if (i) {
                    int idx = row(i);
                    startDrag(idx);
                }
            }
      }
      QListWidget::mouseMoveEvent(event);
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
                                  //QString d;
                                  //xml.dump(d);
                                  //printf("cannot instantiate plugin [%s]\n", d.toLatin1().data());
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

