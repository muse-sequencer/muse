//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "pipeline.h"
#include "auxplugin.h"
#include "plugingui.h"
#include "widgets/filedialog.h"
#include "muse.h"
#include "gui.h"

static const int PipelineDepth = 4;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, AudioTrack* t, bool flag)
   : QListWidget(parent)
      {
      setUniformItemSizes(true);
      setAlternatingRowColors(true);
      prefader = flag;
      setAttribute(Qt::WA_DeleteOnClose, true);
      verticalScrollBar()->setStyle(smallStyle);

      track = t;
//      setFont(config.fonts[1]);

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      setSelectionMode(QAbstractItemView::SingleSelection);
      songChanged(SC_RACK);   // force update
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
	return QSize(STRIP_WIDTH, h);
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void EffectRack::songChanged(int typ)
      {
      if (!(typ & (SC_ROUTE | SC_RACK)))
            return;

      clear();
      Pipeline* pipe = prefader ? track->prePipe() : track->postPipe();
      foreach (PluginI* plugin, *pipe) {
            QListWidgetItem* item = new QListWidgetItem(this);
            item->setText(plugin->name());
            // tooltip should only be set if name does not fit
            // (is elided)
            item->setToolTip(plugin->name());
            item->setBackgroundColor(plugin->on() ? Qt::white : Qt::gray);
            }
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::contextMenuEvent(QContextMenuEvent* ev)
      {
      QPoint pt(ev->pos());
      QListWidgetItem* item = itemAt(pt);

      int idx = -1;
      QString name;
      Pipeline* pipe = prefader ? track->prePipe() : track->postPipe();

      QMenu* menu               = new QMenu;
      QAction* upAction         = menu->addAction(QIcon(*upIcon), tr("move up"));
      QAction* downAction       = menu->addAction(QIcon(*downIcon), tr("move down"));
      QAction* removeAction     = menu->addAction(tr("remove"));
                                  menu->addSeparator();
      QAction* bypassAction     = menu->addAction(tr("bypass"));
      QAction* showAction       = menu->addAction(tr("show gui"));
      QAction* showCustomAction = menu->addAction(tr("show native gui"));
                                  menu->addSeparator();
      QAction* newAction        = menu->addAction(tr("New Plugin"));
      QAction* auxAction        = menu->addAction(tr("New Aux Send"));

      bypassAction->setCheckable(true);
      showAction->setCheckable(true);
      showCustomAction->setCheckable(true);

      if (!item) {
            upAction->setEnabled(false);
            downAction->setEnabled(false);
            removeAction->setEnabled(false);
            bypassAction->setEnabled(false);
            showAction->setEnabled(false);
            showCustomAction->setEnabled(false);
            }
      else {
            idx = row(item);
            upAction->setEnabled(idx != 0);
            downAction->setEnabled(idx < pipe->size()-1);
            idx = item->type();
            showCustomAction->setEnabled(pipe->hasNativeGui(idx));
            bypassAction->setEnabled(true);
            showAction->setEnabled(true);

            bypassAction->setChecked(!pipe->isOn(idx));
            showAction->setChecked(pipe->guiVisible(idx));
            showCustomAction->setChecked(pipe->nativeGuiVisible(idx));
            }
      if (track->type() != Track::WAVE && track->type() != Track::AUDIO_INPUT)
            auxAction->setEnabled(false);

      QAction* sel = menu->exec(mapToGlobal(pt), newAction);
      delete menu;
      if (sel == 0)
            return;

      if (sel == newAction) {
            selectNew();
            return;
            }
      if (sel == removeAction) {
            audio->msgAddPlugin(track, idx, 0, prefader);
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
      else if (sel == auxAction)
            addPlugin(auxPlugin);
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   doubleClicked
//    toggle gui and open requester if needed.
//---------------------------------------------------------

void EffectRack::doubleClicked(QListWidgetItem* it)
      {
      if (track == 0)
            return;
      int idx        = row(it);
      Pipeline* pipe = prefader ? track->prePipe() : track->postPipe();
      bool flag      = !pipe->guiVisible(idx);
      pipe->showGui(idx, flag);
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void EffectRack::startDrag(int idx)
      {
      QString buffer;
      AL::Xml xml(NULL);
      xml.setString(&buffer);
      Pipeline* pipe = prefader ? track->prePipe() : track->postPipe();
      if (pipe) {
            if ((*pipe)[idx] != NULL) {
                PluginI *plug  = (*pipe)[idx];
                xml.header();
                xml.stag("muse version=\"1.0\"");
                // header info
                plug->writeConfiguration1(xml, prefader); // wC1 does not append endtag
                // parameters
                int noParams = plug->plugin()->parameter();
                for (int i=0;i<noParams;i++) {
                    QString fval;
                    QString name(plug->getParameterName(i));
                    fval.setNum(plug->param(i)); // wierd stuff to avoid localization
                    QString str="<control name=\"" + name + "\" val=\""+ fval+"\" />";
                    xml.put(str.toLatin1().data());
                    printf("%s\n",str.toLatin1().data());
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
      //printf("and i wish you were here: %s\n", xml.readAll().toLatin1().data());
      QByteArray xmldump = xml.readAll().toLatin1();
      //printf("xmldump=%s\n",xmldump.data());
      QDrag *drag = new QDrag(this);
      QMimeData *mime = new QMimeData();
      mime->setData("text/x-muse-plugin", xmldump);
      drag->setMimeData(mime);
/*      Qt::DropAction dropAction =*/ drag->start();
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void EffectRack::dropEvent(QDropEvent *event)
      {
      // printf("drop!\n");
      QString text;
      QListWidgetItem* i = itemAt( (event->pos()) );
      int idx = -1;
      if (i)
            idx = row(i);

      Pipeline* pipe = prefader ? track->prePipe() : track->postPipe();
      if (pipe) {
            if (i) {
                if(!QMessageBox::question(this, tr("Replace effect"),tr("Do you really want to replace the effect %1?").arg(pipe->name(idx)),
                      tr("&Yes"), tr("&No"),
                      QString::null, 0, 1 ))
                      {
                      audio->msgAddPlugin(track, idx, 0, prefader);
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
                //printf("DATA:%s\n",outxml.data());
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
                  audio->msgAddPlugin(track, idx, plugi, prefader);
                  song->update(SC_RACK);
                  int i = 0;
                  for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
                        QDomElement e = n.toElement();
                        if (e.nodeName() == "control") {
                            //QString name  = e.attribute("name"); // currently this value is just thrown.
                            QString value  = e.attribute("val");
                            QLocale::setDefault(QLocale::C);
                            double val = value.toFloat();
                            CVal cval;
                            cval.f = val;
                            song->setControllerVal(plugi->track(), plugi->controller(i), cval);
                            i++;
                            }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   mouseDoubleClickEvent
//---------------------------------------------------------

void EffectRack::mouseDoubleClickEvent(QMouseEvent* event)
      {
      QListWidgetItem* it = itemAt(event->pos());
      if (it || (track == 0)) {
            QListWidget::mouseDoubleClickEvent(event);
            return;
            }
      selectNew();
      }

//---------------------------------------------------------
//   selectNew
//---------------------------------------------------------

void EffectRack::selectNew()
      {
      Plugin* plugin = PluginDialog::getPlugin(this);
      addPlugin(plugin);
      song->update(SC_RACK);
      }

//---------------------------------------------------------
//   addPlugin
//---------------------------------------------------------

void EffectRack::addPlugin(Plugin* plugin)
      {
      if (plugin == 0)
            return;
      PluginI* plugi = new PluginI(track);
      if (plugi->initPluginInstance(plugin, track->channels())) {
            printf("cannot instantiate plugin <%s>\n",
               plugin->name().toLatin1().data());
            delete plugi;
            }
      else
            audio->msgAddPlugin(track, -1, plugi, prefader);
      }

