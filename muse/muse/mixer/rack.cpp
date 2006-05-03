//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.cpp,v 1.27 2006/01/25 16:24:33 wschweer Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

//#include <QDrag>
//#include <QMimeData>
#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "plugin.h"
#include "plugingui.h"
#include "widgets/filedialog.h"

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, AudioTrack* t)
   : QListWidget(parent)
      {
      setAttribute(Qt::WA_DeleteOnClose, true);
      track = t;
      setFont(*config.fonts[1]);

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      setSelectionMode(QAbstractItemView::SingleSelection);
      for (int i = 0; i < PipelineDepth; ++i) {
            QListWidgetItem* item = new QListWidgetItem;
            item->setText(t->efxPipe()->name(i));
            item->setBackgroundColor(t->efxPipe()->isOn(i) ? Qt::white : Qt::gray);
            addItem(item);
            }
      connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
         this, SLOT(doubleClicked(QListWidgetItem*)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      setToolTip(tr("effect rack"));
      setAcceptDrops(true);
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize EffectRack::sizeHint() const
      {
	QFontMetrics fm(font());
	int h = fm.lineSpacing() * PipelineDepth + 1;
	return QSize(100, h);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void EffectRack::songChanged(int typ)
      {
      if (typ & (SC_ROUTE | SC_RACK)) {
            for (int i = 0; i < PipelineDepth; ++i) {
                  QListWidgetItem* it = item(i);
                  it->setText(track->efxPipe()->name(i));
                  it->setBackgroundColor(track->efxPipe()->isOn(i) ? Qt::white : Qt::gray);
                  }
            }
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::contextMenuEvent(QContextMenuEvent* ev)
      {
      QPoint pt(ev->pos());
      QListWidgetItem* item = itemAt(pt);
      if (item == 0 || track == 0)
            return;

      int idx = row(item);
      QString name;
      bool mute;
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            name = pipe->name(idx);
            mute = pipe->isOn(idx);
            }

      QAction* newAction;
      QAction* upAction;
      QAction* downAction;
      QAction* removeAction;
      QAction* bypassAction;
      QAction* showAction;
      QAction* showCustomAction;

      QMenu* menu      = new QMenu;
      upAction         = menu->addAction(QIcon(*upIcon), tr("move up"));
      downAction       = menu->addAction(QIcon(*downIcon), tr("move down"));
      removeAction     = menu->addAction(tr("remove"));
      bypassAction     = menu->addAction(tr("bypass"));
      showAction       = menu->addAction(tr("show gui"));
      showCustomAction = menu->addAction(tr("show native gui"));

      bypassAction->setChecked(!pipe->isOn(idx));
      showAction->setChecked(pipe->guiVisible(idx));
      showCustomAction->setChecked(pipe->nativeGuiVisible(idx));

      if (pipe->empty(idx)) {
            newAction = menu->addAction(tr("new"));
            upAction->setEnabled(false);
            downAction->setEnabled(false);
            removeAction->setEnabled(false);
            bypassAction->setEnabled(false);
            showAction->setEnabled(false);
            showCustomAction->setEnabled(false);
            }
      else {
            newAction = menu->addAction(tr("change"));
            if (idx == 0)
                  upAction->setEnabled(false);
            if (idx == (PipelineDepth-1))
                  downAction->setEnabled(false);
            showCustomAction->setEnabled(pipe->hasNativeGui(idx));
            }

      QAction* sel = menu->exec(mapToGlobal(pt), newAction);
      delete menu;
      if (sel == 0)
            return;

      if (sel == newAction) {
            Plugin* plugin = PluginDialog::getPlugin(this);
            if (plugin) {
                  PluginI* plugi = new PluginI(track);
                  if (plugi->initPluginInstance(plugin, track->channels())) {
                        printf("cannot instantiate plugin <%s>\n",
                           plugin->name().toLatin1().data());
                        delete plugi;
                        }
                  else
                        audio->msgAddPlugin(track, idx, plugi);
                  }
            }
      else if (sel == removeAction) {
            audio->msgAddPlugin(track, idx, 0);
            }
      else if (sel == bypassAction) {
            bool flag = !pipe->isOn(idx);
            pipe->setOn(idx, flag);
            }
      else if (sel == showAction) {
            bool flag = !pipe->guiVisible(idx);
            pipe->showGui(idx, flag);
            }
      else if (sel == showCustomAction) {
            bool flag = !pipe->nativeGuiVisible(idx);
            pipe->showNativeGui(idx, flag);
            }
      else if (sel == upAction) {
            if (idx > 0) {
                  setCurrentRow(idx-1);
                  pipe->move(idx, true);
                  }
            }
      else if (sel == downAction) {
            if (idx < (PipelineDepth-1)) {
                  setCurrentRow(idx+1);
                  pipe->move(idx, false);
                  }
            }
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   doubleClicked
//    toggle gui and open requester if needed.
//---------------------------------------------------------

void EffectRack::doubleClicked(QListWidgetItem* it)
      {
      if (it == 0 || track == 0)
            return;
      int idx        = row(it);
      Pipeline* pipe = track->efxPipe();

      if (!pipe->empty(idx)) {
            bool flag = !pipe->guiVisible(idx);
            pipe->showGui(idx, flag);
            }
      else {
            Plugin* plugin = PluginDialog::getPlugin(this);
            if (plugin) {
                  PluginI* plugi = new PluginI(track);
                  if (plugi->initPluginInstance(plugin, track->channels())) {
                        printf("cannot instantiate plugin <%s>\n",
                           plugin->name().toLatin1().data());
                        delete plugi;
                        }
                  else {
                        audio->msgAddPlugin(track, idx, plugi);
                        }
                  song->update(SC_RACK);
                  }
            }
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void EffectRack::startDrag(int idx)
      {
      QBuffer buffer;
      AL::Xml xml(&buffer);
      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != NULL) {
                PluginI *plug  = (*pipe)[idx];
                xml.header();
                xml.tag("muse version=\"1.0\"");
                // header info
                plug->writeConfiguration1(xml); // wC1 does not append endtag
                // parameters
                int noParams = plug->plugin()->parameter();
                for (int i=0;i<noParams;i++) {
                    QString fval;
                    QString name(plug->getParameterName(i));
                    fval.setNum(plug->param(i)); // wierd stuff to avoid localization
                    QString str="<control name=\"" + name + "\" val=\""+ fval+"\" />";
                    xml.put(str.toLatin1().data());
                    }
                xml.etag("plugin");

                xml.etag("muse");
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
	
      QByteArray xmldump = buffer.buffer();
      QDrag *drag = new QDrag(this);
      QMimeData *mime = new QMimeData();
      mime->setData("text/x-muse-plugin", xmldump);
      drag->setMimeData(mime);
      Qt::DropAction dropAction = drag->start();
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------
void EffectRack::dropEvent(QDropEvent *event)
      {
      QString text;
      QListWidgetItem *i = itemAt( (event->pos()) );
      int idx = row(i);

      Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != NULL) {
                if(!QMessageBox::question(this, tr("Replace effect"),tr("Do you really want to replace the effect %1?").arg(pipe->name(idx)),
                      tr("&Yes"), tr("&No"),
                      QString::null, 0, 1 ))
                      {
                      audio->msgAddPlugin(track, idx, 0);
                      song->update(SC_RACK);
                      }
                else {
                      printf("nothing here\n");
                      return;
                      }
                }
            if(event->mimeData()->hasText())
                {
                //printf("has text\n");
                text = event->mimeData()->text().trimmed();
                if (QString(text).endsWith(".pre"))
                    {
                    QUrl url(text);
                    QString newPath = url.path();

                    QFile xmlfile(newPath);
                    xmlfile.open(QIODevice::ReadOnly);
                    QDomDocument doc;
                    doc.setContent(&xmlfile);
                    QDomNode node = doc.documentElement().firstChild();
                    initPlugin(node, idx);
                    }
                }
            else if (event->mimeData()->hasFormat("text/x-muse-plugin"))
                {
                QByteArray outxml = event->mimeData()->data("text/x-muse-plugin");
                //printf("%s\n",outxml.data());
                QDomDocument doc;
                doc.setContent(outxml);
                QDomNode node = doc.documentElement().firstChild();

                initPlugin(node, idx);
                }
            }
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------
void EffectRack::dragEnterEvent(QDragEnterEvent *event)
      {
      //printf("dragEnterEvent\n");
      //if (event->mimeData()->hasFormat("text/x-muse-plugin"))
          event->acceptProposedAction();
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------
void EffectRack::dragMoveEvent(QDragMoveEvent *event)
      {
      //printf("dragMoveEvent\n");
      //if (event->mimeData()->hasFormat("text/x-muse-plugin"))
          event->acceptProposedAction();
      }

//---------------------------------------------------------
//   contentsMousePressEvent
//---------------------------------------------------------
void EffectRack::mousePressEvent(QMouseEvent *event)
      {
      //printf("mousePressEvent\n");
      if(event->button() & Qt::LeftButton) {
          dragPos = event->pos();
      }
      QListWidget::mousePressEvent(event);
      }

//---------------------------------------------------------
//   contentsMouseMoveEvent
//---------------------------------------------------------
void EffectRack::mouseMoveEvent(QMouseEvent *event)
      {
      if (event->buttons() & Qt::LeftButton) {
            int distance = (dragPos-event->pos()).manhattanLength();
            if (distance > QApplication::startDragDistance()) {
                  QListWidgetItem *i = itemAt( event->pos() );
                  int idx = row(i);
                  startDrag(idx);
                  }
            }
      QListWidget::mouseMoveEvent(event);
      }


//---------------------------------------------------------
//   initPlugin
//---------------------------------------------------------

void EffectRack::initPlugin(QDomNode &node, int idx)
      {
      QDomElement e = node.toElement();
      //QString version  = e.attribute("version");
      QString file  = e.attribute("file");
      QString label = e.attribute("label");

      //printf("version=%s file=%s label=%s channel=%d\n",version.toLatin1().data(),file.toLatin1().data(), label.toLatin1().data(), channel);

      //Plugin* plugin = PluginDialog::getPlugin(this);
      Plugin* plugin = plugins.find(file, label);
      if (plugin) {
            PluginI* plugi = new PluginI(track);
            if (plugi->initPluginInstance(plugin, track->channels())) {
                  printf("cannot instantiate plugin <%s>\n",
                      plugin->name().toLatin1().data());
                  delete plugi;
                  }
            else {
                  audio->msgAddPlugin(track, idx, plugi);
                  song->update(SC_RACK);
                  int i = 0;
                  for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
                        QDomElement e = n.toElement();
                        if (e.nodeName() == "control") {
                            //QString name  = e.attribute("name"); // currently this value is just thrown.
                            QString value  = e.attribute("val");
                            QLocale::setDefault(QLocale::C);
                            float val = value.toFloat();
                            CVal cval;
                            cval.f = val;
                            song->setControllerVal(plugi->track(), plugi->controller(i), cval);
                            i++;
                            }
                        }
                  }
            }
      }
