//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.cpp,v 1.7.2.7 2007/01/27 14:52:43 spamatica Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QScrollBar>

#include "popupmenu.h"

#include <errno.h>

#include "xml.h"
#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "globaldefs.h"
#include "plugin.h"
#include "plugindialog.h"
#include "filedialog.h"
#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QEvent>
#include "track.h"
#include "background_painter.h"

namespace MusEGui {

QString MUSE_MIME_TYPE = "text/x-muse-plugin";

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
      
      virtual QSize sizeHint(const QStyleOptionViewItem& option, 
                             const QModelIndex& index) const;
      static const int itemXMargin;
      static const int itemYMargin;
      static const int itemTextXMargin;
      static const int itemTextYMargin;
};

const int EffectRackDelegate::itemXMargin = 1;
const int EffectRackDelegate::itemYMargin = 1;
const int EffectRackDelegate::itemTextXMargin = 2;
const int EffectRackDelegate::itemTextYMargin = 1;

EffectRackDelegate::EffectRackDelegate(QObject * parent, MusECore::AudioTrack* at ) : QStyledItemDelegate(parent) { 
      er = (EffectRack*) parent; 
      tr = at;
}

QSize EffectRackDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
  return QSize(10, option.fontMetrics.height() + 2 * itemYMargin + 2 * itemTextYMargin);
}

void EffectRackDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing);

      const QRect rr = option.rect;
      QRect cr = QRect(rr.x() + itemXMargin, rr.y() + itemYMargin,
                       rr.width() - 2 * itemXMargin, rr.height() - 2 * itemYMargin);

      const QRect onrect = (tr->efxPipe() && tr->efxPipe()->isOn(index.row())) ? rr : QRect();
      ItemBackgroundPainter* ibp = er->getBkgPainter();
      ibp->drawBackground(painter,
                          rr,
                          option.palette,
                          itemXMargin,
                          itemYMargin,
                          onrect,
                          er->radius(),
                          er->style3d(),
                          MusEGlobal::config.rackItemBgActiveColor,
                          MusEGlobal::config.rackItemBorderColor,
                          MusEGlobal::config.rackItemBackgroundColor);

      QString name = tr->efxPipe() ? tr->efxPipe()->name(index.row()) : QString();
  
      if (option.state & QStyle::State_MouseOver)
          painter->setPen(MusEGlobal::config.rackItemFontColorHover);
      else if (onrect.isNull())
          painter->setPen(MusEGlobal::config.rackItemFontColor);
      else
          painter->setPen(MusEGlobal::config.rackItemFontActiveColor);

      painter->drawText(cr.x() + itemTextXMargin, 
                        cr.y() + itemTextYMargin, 
                        cr.width() - 2 * itemTextXMargin, 
                        cr.height() - 2 * itemTextYMargin, 
                        Qt::AlignLeft | Qt::AlignVCenter,
                        name);

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
      //void setBackgroundColor(const QBrush& brush) {setBackground(brush);}
      };

RackSlot::~RackSlot()
      {
      node = nullptr;
      }

//---------------------------------------------------------
//   RackSlot
//---------------------------------------------------------

RackSlot::RackSlot(QListWidget* b, MusECore::AudioTrack* t, int i, int /*h*/)
   : QListWidgetItem(b)
      {
      node = t;
      idx  = i;
      }

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

EffectRack::EffectRack(QWidget* parent, MusECore::AudioTrack* t)
   : QListWidget(parent)
      {
      setObjectName("Rack");
      viewport()->setObjectName("EffectRack"); // needed for context help
      setStatusTip(tr("Effect rack: Double-click a slot to insert/edit effect. RMB to open context menu. Press F1 for help."));

      setAttribute(Qt::WA_DeleteOnClose);

     _bkgPainter = new ItemBackgroundPainter(this);

      track = t;
      itemheight = 19;

      _style3d = true;
      _radius = 2;
      _customScrollbar = true;

      setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

      ensurePolished();

      if (_customScrollbar) {
          // FIXME: put into external stylesheet
          // I tried, but there is a bug in QT, not possible to address scrollbar in individual widget (kybos)
          QFile file(":/qss/scrollbar_small_vertical.qss");
          file.open(QFile::ReadOnly);
          QString style = file.readAll();
          style.replace("darkgrey", MusEGlobal::config.rackItemBackgroundColor.name());
          style.replace("lightgrey", MusEGlobal::config.rackItemBackgroundColor.lighter().name());
          style.replace("grey", MusEGlobal::config.rackItemBackgroundColor.darker().name());
          verticalScrollBar()->setStyleSheet(style);
      }

      setSelectionMode(QAbstractItemView::SingleSelection);

      for (int i = 0; i < MusECore::PipelineDepth; ++i)
            new RackSlot(this, track, i, itemheight);

      updateContents();

      connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
         this, SLOT(doubleClicked(QListWidgetItem*)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));

      EffectRackDelegate* er_delegate = new EffectRackDelegate(this, track);
      setItemDelegate(er_delegate);
      viewport()->setAttribute( Qt::WA_Hover );

      setSpacing(0);

      setAcceptDrops(true);
      setFocusPolicy(Qt::NoFocus);
      }

void EffectRack::updateContents()
      {
      if(!track)
        return;

      MusECore::Pipeline* pipe = track->efxPipe();
      if(!pipe)
        return;

      for (int i = 0; i < MusECore::PipelineDepth; ++i) {
            const QString name = pipe->name(i);
            const QString uri = pipe->uri(i);
            item(i)->setText(name);
            const QString ttname = name + (uri.isEmpty() ? QString() : QString(" \n") + uri);
            item(i)->setToolTip(pipe->empty(i) ? tr("Effect rack\nDouble-click a slot to insert FX") : ttname );
            //item(i)->setBackground(track->efxPipe()->isOn(i) ? activeColor : palette().dark());
            if(viewport())
            {
              QRect r(visualItemRect(item(i)));
              viewport()->update(r);
            }
      }
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void EffectRack::songChanged(MusECore::SongChangedStruct_t typ)
      {
      if (typ & (SC_TRACK_REMOVED)) {
        if(!MusEGlobal::song->trackExists(track))
        {
          track = nullptr;
          return;
        }
      }

      if (typ & (SC_ROUTE | SC_RACK)) {
            updateContents();
       	    }
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize EffectRack::minimumSizeHint() const
      {
      return QSize(10, 
        2 * frameWidth() + 
        (fontMetrics().height() + 2 * EffectRackDelegate::itemYMargin + 2 * EffectRackDelegate::itemTextYMargin) 
        * MusEGlobal::config.audioEffectsRackVisibleItems);
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
      if(!it || !track)
        return;
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
                  MusEGlobal::audio->msgAddPlugin(track, idx, nullptr);
            MusEGlobal::audio->msgAddPlugin(track, idx, plugi);
            updateContents();
            }
      }

//---------------------------------------------------------
//   menuRequested
//---------------------------------------------------------

void EffectRack::menuRequested(QListWidgetItem* it)
      {
      if (it == nullptr || track == nullptr)
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
      QAction* newAction = menu->addAction(tr("New"));
      QAction* changeAction = menu->addAction(tr("Change"));
      QAction* upAction = menu->addAction(QIcon(*upIcon), tr("Move up"));//,   UP, UP);
      QAction* downAction = menu->addAction(QIcon(*downIcon), tr("Move down"));//, DOWN, DOWN);
      QAction* removeAction = menu->addAction(tr("Remove"));//,    REMOVE, REMOVE);
      QAction* bypassAction = menu->addAction(tr("Bypass"));//,    BYPASS, BYPASS);
      QAction* showGuiAction = menu->addAction(tr("Show gui"));//,  SHOW, SHOW);
      QAction* showNativeGuiAction = menu->addAction(tr("Show native gui"));//,  SHOW_NATIVE, SHOW_NATIVE);
      QAction* saveAction = menu->addAction(tr("Save preset"));

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

#ifdef LV2_SUPPORT
      PopupMenu *mSubPresets = nullptr;
#endif

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
            if (idx == (MusECore::PipelineDepth-1))
                  downAction->setEnabled(false);
            //if(!pipe->isDssiPlugin(idx))
            if(!pipe->has_dssi_ui(idx))     // p4.0.19 Tim.
                  showNativeGuiAction->setEnabled(false);
#ifdef LV2_SUPPORT
            //show presets submenu for lv2 plugins
            mSubPresets = new PopupMenu(tr("Presets"));
            if(pipe->isLV2Plugin(idx))
            {
               menu->addMenu(mSubPresets);
               MusECore::PluginI *plugI = pipe->at(idx);
               static_cast<MusECore::LV2PluginWrapper *>(plugI->plugin())->populatePresetsMenu(plugI, mSubPresets);
            }
            else
            {
               delete mSubPresets;
               mSubPresets = nullptr;
            }
#endif
            }


      #ifndef OSC_SUPPORT
      showNativeGuiAction->setEnabled(false);
      #endif

      QPoint pt = QCursor::pos();
      QAction* act = menu->exec(pt, nullptr);

      //delete menu;
      if (!act)
      {
        delete menu;
        return;
      }      
#ifdef LV2_SUPPORT
      if (mSubPresets != nullptr) {
         QWidget *mwidget = act->parentWidget();
         if (mwidget != nullptr) {
            if(mSubPresets == dynamic_cast<QMenu*>(mwidget)) {
               MusECore::PluginI *plugI = pipe->at(idx);
               static_cast<MusECore::LV2PluginWrapper *>(plugI->plugin())->applyPreset(plugI, act->data().value<void *>());
               delete menu;
               return;
            }
         }
      }
#endif
      
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
                  MusEGlobal::audio->msgAddPlugin(track, idx, nullptr);
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
                  if (idx < (MusECore::PipelineDepth-1)) {
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
      if (it == nullptr || track == nullptr)
            return;

      RackSlot* item = (RackSlot*)it;
      int idx        = row(item);
      MusECore::Pipeline* pipe = track->efxPipe();

      if (!pipe)
        return;

      if (pipe->empty(idx))
      {
        choosePlugin(it);
        return;
      }

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

void EffectRack::savePreset(int idx)
      {
      if(!track)
        return;
      //QString name = MusEGui::getSaveFileName(QString(""), plug_file_pattern, this,
      QString name = MusEGui::getSaveFileName(QString(""), MusEGlobal::preset_file_save_pattern, this,
         tr("MusE: Save Preset"));
      
      if(name.isEmpty())
        return;
        
      //FILE* presetFp = fopen(name.ascii(),"w+");
      bool popenFlag;
      FILE* presetFp = MusEGui::fileOpen(this, name, QString(".pre"), "w", popenFlag, false, true);
      if (presetFp == nullptr) {
            //fprintf(stderr, "EffectRack::savePreset() fopen failed: %s\n",
            //   strerror(errno));
            return;
            }
      MusECore::Xml xml(presetFp);
      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != nullptr) {
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

void EffectRack::startDragItem(int idx)
      {
      if(!track)
        return;
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
      if (tmp == nullptr) {
            fprintf(stderr, "EffectRack::startDrag fopen failed: %s\n",
               strerror(errno));
            return;
            }
      MusECore::Xml xml(tmp);
      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe) {
            if ((*pipe)[idx] != nullptr) {
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
      QMimeData* md = new QMimeData();
      QByteArray data(xmlconf.toLatin1().constData());

      if (MusEGlobal::debugMsg)
          printf("Sending %d [%s]\n", data.length(), xmlconf.toLatin1().constData());

      md->setData(MUSE_MIME_TYPE, data);
      
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
      mTypes << MUSE_MIME_TYPE;
      return mTypes;
      }

void EffectRack::dropEvent(QDropEvent *event)
{
      if(!event || !track)
        return;
      QListWidgetItem *i = itemAt( event->pos() );
      if (!i)
            return;
      int idx = row(i);
      
      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe) 
      {
            if ((*pipe)[idx] != nullptr) {
                QWidget *sw = static_cast<QWidget *>(event->source());
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
                        MusEGlobal::audio->msgAddPlugin(track, idx, nullptr);
                        MusEGlobal::song->update(SC_RACK);
                      }
                else {
                      return;
                      }
                }
            
            if(event->mimeData()->hasFormat(MUSE_MIME_TYPE))
            {
              QByteArray mimeData = event->mimeData()->data(MUSE_MIME_TYPE).constData();
              MusECore::Xml xml(mimeData.constData());
              if (MusEGlobal::debugMsg)
                  printf("received %d [%s]\n", mimeData.size(), mimeData.constData());

              initPlugin(xml, idx);
            }
            else if (event->mimeData()->hasUrls())
            {
              // Multiple urls not supported here. Grab the first one.
              QString text = event->mimeData()->urls()[0].path();
               
              if (text.endsWith(".pre", Qt::CaseInsensitive) || 
                  text.endsWith(".pre.gz", Qt::CaseInsensitive) || 
                  text.endsWith(".pre.bz2", Qt::CaseInsensitive))
              {
                  bool popenFlag;
                  FILE* fp = MusEGui::fileOpen(this, text, ".pre", "r", popenFlag, false, false);
                  if (fp) 
                  {
                      MusECore::Xml xml(fp);
                      initPlugin(xml, idx);
                      
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
      if(event && track)
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
      }

      QListWidget::mousePressEvent(event);  
      }

void EffectRack::mouseMoveEvent(QMouseEvent *event)
{
      if(event && track)
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
                      startDragItem(idx);
                  }
              }
        }
      }
      QListWidget::mouseMoveEvent(event);
}

void EffectRack::enterEvent(QEvent *event)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  QListWidget::enterEvent(event);
}

void EffectRack::leaveEvent(QEvent *event)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QListWidget::leaveEvent(event);
}

void EffectRack::initPlugin(MusECore::Xml xml, int idx)
      {      
      if(!track)
        return;
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
                                    plugi->gui()->updateWindowTitle();
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
