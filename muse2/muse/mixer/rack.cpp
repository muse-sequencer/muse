//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.cpp,v 1.7.2.7 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
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
#include <QStyledItemDelegate>
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

namespace MusEGui {

//---------------------------------------------------------
//   class EffectRackDelegate
//---------------------------------------------------------

class EffectRackDelegate : public QStyledItemDelegate {
  
      EffectRack* er;
      MusECore::AudioTrack* tr;

   public:
      void paint ( QPainter * painter, 
                   const QStyleOptionViewItem & option, 
                   const QModelIndex & index ) const;
      EffectRackDelegate(QObject * parent, MusECore::AudioTrack* at );
};

EffectRackDelegate::EffectRackDelegate(QObject * parent, MusECore::AudioTrack* at ) : QStyledItemDelegate(parent) { 
      er = (EffectRack*) parent; 
      tr = at;
}

void EffectRackDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing);

      QRect rr = er->visualItemRect(er->item(index.row()));
      QRect cr = QRect(rr.x()+1, rr.y()+1, 
                       rr.width()-2, rr.height() -2);
      painter->fillRect(rr, option.palette.dark().color().darker(130));

      QColor mask_edge = QColor(110, 110, 110, 55);
      QColor mask_center = QColor(220, 220, 220, 55);
      QLinearGradient mask;
      mask.setColorAt(0, mask_edge);
      mask.setColorAt(0.5, mask_center);
      mask.setColorAt(1, mask_edge);
      mask.setStart(QPointF(0, cr.y()));
      mask.setFinalStop(QPointF(0, cr.y() + cr.height()));

      painter->setBrush(tr->efxPipe()->isOn(index.row()) ?
                        er->getActiveColor() :
                        option.palette.dark());
      painter->setPen(Qt::NoPen);
      painter->drawRoundedRect(cr, 2, 2);
      painter->setBrush(mask);
      painter->drawRoundedRect(cr, 2, 2);

      QString name = tr->efxPipe()->name(index.row());
      if (name.length() > 11)
            name = name.left(9) + "...";
  
      if (option.state & QStyle::State_Selected)
            {
            if (option.state & QStyle::State_MouseOver)
                  painter->setPen(QPen(QColor(239,239,239)));
            else
                  painter->setPen(QPen(Qt::white));
            }
      else if (option.state & QStyle::State_MouseOver)
            painter->setPen(QPen(QColor(48,48,48)));
      else
            painter->setPen(QPen(Qt::black));
  
      painter->drawText(cr.x()+2, cr.y()+1, 
                        cr.width()-2, cr.height()-1, 
                        Qt::AlignLeft, name);

      painter->restore();
}


//---------------------------------------------------------
//   class RackSlot
//---------------------------------------------------------

class RackSlot : public QListWidgetItem {
      int idx;
      MusECore::AudioTrack* node;

   public:
      RackSlot(QListWidget* lb, MusECore::AudioTrack* t, int i, int h);
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

RackSlot::RackSlot(QListWidget* b, MusECore::AudioTrack* t, int i, int h)
   : QListWidgetItem(b)
      {
      node = t;
      idx  = i;
      setSizeHint(QSize(10,h));
      }

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, MusECore::AudioTrack* t)
   : QListWidget(parent)
      {
      setObjectName("Rack");
      setAttribute(Qt::WA_DeleteOnClose);
      track = t;
      itemheight = 19;
      setFont(MusEGlobal::config.fonts[1]);
      activeColor = QColor(74, 165, 49);

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setSelectionMode(QAbstractItemView::SingleSelection);

      for (int i = 0; i < PipelineDepth; ++i)
            new RackSlot(this, track, i, itemheight);
      updateContents();

      connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
         this, SLOT(doubleClicked(QListWidgetItem*)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      EffectRackDelegate* er_delegate = new EffectRackDelegate(this, track);
      setItemDelegate(er_delegate);

      setSpacing(0);

      setAcceptDrops(true);
      setFocusPolicy(Qt::NoFocus);
      }

void EffectRack::updateContents()
      {
      for (int i = 0; i < PipelineDepth; ++i) {
            QString name = track->efxPipe()->name(i);
            item(i)->setText(name);
            item(i)->setBackground(track->efxPipe()->isOn(i) ? activeColor : palette().dark());
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
      // FIXME(Orcan): Why do we have to manually add 6 pixels?
      return QSize(10, itemheight * PipelineDepth + 6);
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
      MusECore::Plugin* plugin = PluginDialog::getPlugin(this);
      if (plugin) {
            MusECore::PluginI* plugi = new MusECore::PluginI();
            if (plugi->initPluginInstance(plugin, track->channels())) {
                  printf("cannot instantiate plugin <%s>\n",
                      plugin->name().toLatin1().constData());
                  delete plugi;
                  return;
                  }
            int idx = row(it);
	    if (replace)
                  MusEGlobal::audio->msgAddPlugin(track, idx, 0);
            MusEGlobal::audio->msgAddPlugin(track, idx, plugi);
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
      //bool mute;
      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe) {
            name  = pipe->name(idx);
            //mute  = pipe->isOn(idx);
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
                  MusEGlobal::audio->msgAddPlugin(track, idx, 0);
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
      MusEGlobal::song->update(SC_RACK);
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
      MusECore::Pipeline* pipe = track->efxPipe();

      if (pipe->name(idx) == QString("empty")) {
            choosePlugin(it);
            return;
            }
      if (pipe) {
            bool flag;
            if (pipe->has_dssi_ui(idx))
            {
              flag = !pipe->nativeGuiVisible(idx);
              pipe->showNativeGui(idx, flag);

            }
            else {
              flag = !pipe->guiVisible(idx);
              pipe->showGui(idx, flag);
            }
            }
      }

void EffectRack::savePreset(int idx)
      {
      //QString name = MusEGui::getSaveFileName(QString(""), plug_file_pattern, this,
      QString name = MusEGui::getSaveFileName(QString(""), MusEGlobal::preset_file_save_pattern, this,
         tr("MusE: Save Preset"));
      
      if(name.isEmpty())
        return;
        
      //FILE* presetFp = fopen(name.ascii(),"w+");
      bool popenFlag;
      FILE* presetFp = MusEGui::fileOpen(this, name, QString(".pre"), "w", popenFlag, false, true);
      if (presetFp == 0) {
            //fprintf(stderr, "EffectRack::savePreset() fopen failed: %s\n",
            //   strerror(errno));
            return;
            }
      MusECore::Xml xml(presetFp);
      MusECore::Pipeline* pipe = track->efxPipe();
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
      if (MusEGlobal::debugMsg) {
          QString fileName;
          MusEGlobal::getUniqueTmpfileName("tmp","preset", fileName);
          tmp = fopen(fileName.toLatin1().data(), "w+");
      }
      else
          tmp = tmpfile();
      if (tmp == 0) {
            fprintf(stderr, "EffectRack::startDrag fopen failed: %s\n",
               strerror(errno));
            return;
            }
      MusECore::Xml xml(tmp);
      MusECore::Pipeline* pipe = track->efxPipe();
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
      printf("[%s]\n", xmlconf.toLatin1().constData());

      
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
      
      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe) 
      {
            if ((*pipe)[idx] != NULL) {
                QWidget *sw = event->source();
                if(sw)
                {
                  if(strcmp(sw->metaObject()->className(), "EffectRack") == 0) 
                  { 
                    EffectRack *ser = (EffectRack*)sw;
                    MusECore::Pipeline* spipe = ser->getTrack()->efxPipe();
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
                        MusEGlobal::audio->msgAddPlugin(track, idx, 0);
                        MusEGlobal::song->update(SC_RACK);
                      }
                else {
                      return;
                      }
                }
            
            if(event->mimeData()->hasFormat("text/x-muse-plugin"))
            {
              char *tmpStr = new char[event->mimeData()->data("text/x-muse-plugin").size()];
              strcpy(tmpStr, event->mimeData()->data("text/x-muse-plugin").data());
              MusECore::Xml xml(tmpStr);
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
                  FILE* fp = MusEGui::fileOpen(this, text, ".pre", "r", popenFlag, false, false);
                  if (fp) 
                  {
                      MusECore::Xml xml(fp);
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
      RackSlot* item = (RackSlot*) itemAt(event->pos());
      if(event->button() & Qt::LeftButton) {
          dragPos = event->pos();
      }
      else if(event->button() & Qt::RightButton) {
          setCurrentItem(item);
          menuRequested(item);
          return;
      }
      else if(event->button() & Qt::MidButton) {
          int idx = row(item);
          bool flag = !track->efxPipe()->isOn(idx);
          track->efxPipe()->setOn(idx, flag);
          updateContents();
      }

      QListWidget::mousePressEvent(event);  
      }

void EffectRack::mouseMoveEvent(QMouseEvent *event)
{
      if (event->buttons() & Qt::LeftButton) {
            MusECore::Pipeline* pipe = track->efxPipe();
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


void EffectRack::initPlugin(MusECore::Xml xml, int idx)
      {      
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "plugin") {
                              MusECore::PluginI* plugi = new MusECore::PluginI();
                              if (plugi->readConfiguration(xml, false)) {
                                  //QString d;
                                  //xml.dump(d);
                                  //printf("cannot instantiate plugin [%s]\n", d.toLatin1().data());
                                  delete plugi;
                                  }
                              else {
                                  //printf("instantiated!\n");
                                  MusEGlobal::audio->msgAddPlugin(track, idx, plugi);
                                  MusEGlobal::song->update(SC_RACK);
                                  if (plugi->guiVisible())
                                    plugi->gui()->setWindowTitle(plugi->titlePrefix() + plugi->name());
                                  return;
                                  }
                              }
                        else if (tag =="muse")
                              break;
                        else
                              xml.unknown("EffectRack");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "muse")
                              return;
                  default:
                        break;
                  }
            }
      }                        

} // namespace MusEGui
