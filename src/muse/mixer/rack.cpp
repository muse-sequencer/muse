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
#include <QPainter>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QScrollBar>
#include <QTemporaryFile>
#include <QList>
#include <QMessageBox>

#include "popupmenu.h"
#include "rack.h"
#include "song.h"
#include "audio.h"
#include "icons.h"
#include "gconfig.h"
#include "globaldefs.h"
#include "plugindialog.h"
#include "filedialog.h"
#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif
#include "undo.h"
#include "libs/file/file.h"
#include "background_painter.h"

// Forwards from header:
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QEnterEvent>

#include "track.h"
#include "plugin.h"
#include "xml.h"
#include "ctrl.h"

namespace MusEGui {

const QString MUSE_MIME_TYPE = "text/x-muse-plugin";

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
const int EffectRackDelegate::itemTextXMargin = 1;
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

      const QRect onrect =
        (tr->efxPipe() && tr->efxPipe()->isOn(index.row()) && tr->efxPipe()->isActive(index.row())) ? rr : QRect();
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
            const QString ttname = (name + (uri.isEmpty() ? QString() : QString(" \n") + uri)) +
              (pipe->at(i) && !pipe->at(i)->plugin() ? QString(tr("\nPLUGIN IS UNAVAILABLE!")) : QString(""));
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

void EffectRack::choosePlugin(QListWidgetItem* it)
{
    if(!it || !track)
          return;
    MusECore::Plugin* plugin = PluginDialog::getPlugin(this);
    if (!plugin)
      return;
    MusECore::PluginI* plugi = new MusECore::PluginI();
    if (plugi->initPluginInstance(plugin, track->channels())) {
          printf("cannot instantiate plugin <%s>\n",
                plugin->name().toLocal8Bit().constData());
          delete plugi;
          return;
          }
    int idx = row(it);

    MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ChangeRackEffectPlugin, track, plugi, idx));
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
      MusECore::Pipeline* pipe = track->efxPipe();

      enum { NEW, CHANGE, UP, DOWN, REMOVE, ACTIVE, BYPASS, SHOW, SHOW_NATIVE, SAVE };
      QMenu* menu = new QMenu;

      if (pipe->empty(idx)) {
        QAction* newAction = menu->addAction(*dummySVGIcon, tr("New"));
        newAction->setData(NEW);
      }
      else {
        QAction* changeAction = menu->addAction(tr("Change"));
        changeAction->setData(CHANGE);
      }

      QAction* upAction = menu->addAction(tr("Move Up"));//,   UP, UP);
      QAction* downAction = menu->addAction(tr("Move Down"));//, DOWN, DOWN);
      QAction* removeAction = menu->addAction(tr("Remove"));//,    REMOVE, REMOVE);
      menu->addSeparator();
      QAction* activeAction = menu->addAction(tr("Active"));//,    ACTIVE, ACTIVE);
      QAction* bypassAction = menu->addAction(tr("Bypass"));//,    BYPASS, BYPASS);
      menu->addSeparator();
      QAction* showGuiAction = menu->addAction(tr("Show Generic GUI"));//,  SHOW, SHOW);
      QAction* showNativeGuiAction = menu->addAction(tr("Show Native GUI"));//,  SHOW_NATIVE, SHOW_NATIVE);

      if (!pipe->empty(idx)) {
        QAction* saveAction = menu->addAction(tr("Save Preset"));
        saveAction->setData(SAVE);
      }

      upAction->setData(UP);
      downAction->setData(DOWN);
      removeAction->setData(REMOVE);
      activeAction->setData(ACTIVE);
      bypassAction->setData(BYPASS);
      showGuiAction->setData(SHOW);
      showNativeGuiAction->setData(SHOW_NATIVE);

      activeAction->setCheckable(true);
      bypassAction->setCheckable(true);
      showGuiAction->setCheckable(true);
      showNativeGuiAction->setCheckable(true);

      activeAction->setChecked(pipe->isActive(idx));
      bypassAction->setChecked(!pipe->isOn(idx));
      showGuiAction->setChecked(pipe->guiVisible(idx));
      showNativeGuiAction->setChecked(pipe->nativeGuiVisible(idx));

#ifdef LV2_SUPPORT
      PopupMenu *mSubPresets = nullptr;
#endif

      if (pipe->empty(idx)) {
            upAction->setEnabled(false);
            downAction->setEnabled(false);
            removeAction->setEnabled(false);
            activeAction->setEnabled(false);
            bypassAction->setEnabled(false);
            showGuiAction->setEnabled(false);
            showNativeGuiAction->setEnabled(false);
            }
      else {
            if (idx == 0)
                  upAction->setEnabled(false);
            if (idx == (MusECore::PipelineDepth-1))
                  downAction->setEnabled(false);
            if(!pipe->hasNativeGui(idx))
                  showNativeGuiAction->setEnabled(false);
#ifdef LV2_SUPPORT
            const MusEPlugin::PluginType ptype = pipe->pluginType(idx);
            if(ptype == MusEPlugin::PluginTypeLV2)
            {
               //show presets submenu for lv2 plugins
               mSubPresets = new PopupMenu(tr("Presets"));
               menu->addMenu(mSubPresets);
               MusECore::PluginI *plugI = pipe->at(idx);
               static_cast<MusECore::LV2PluginWrapper *>(plugI->plugin())->populatePresetsMenu(plugI, mSubPresets);
            }
#endif
            }


      #ifndef OSC_SUPPORT
      showNativeGuiAction->setEnabled(false);
      #endif

      QPoint pt = QCursor::pos();
      QAction* act = menu->exec(pt, nullptr);

      if (!act)
      {
        delete menu;
        return;
      }      
#ifdef LV2_SUPPORT
      if (mSubPresets != nullptr) {
         QWidget *mwidget = qobject_cast<QWidget*>(act->parent());
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
                  choosePlugin(it);
                  break;
                  }
            case REMOVE:
                  if(pipe && pipe->at(idx))
                  {
                    MusEGlobal::song->applyOperation(MusECore::UndoOp(
                      MusECore::UndoOp::ChangeRackEffectPlugin, track, (MusECore::PluginI*)nullptr, idx));
                  }
                  break;
            case ACTIVE:
                  {
                  bool flag = !pipe->isActive(idx);
                  pipe->setActive(idx, flag);
                  updateContents();
                  MusEGlobal::song->update(SC_RACK);
                  break;
                  }
            case BYPASS:
                  {
                  bool flag = !pipe->isOn(idx);
                  pipe->setOn(idx, flag);
                  updateContents();
                  MusEGlobal::song->update(SC_RACK);
                  break;
                  }
            case SHOW:
                  {
                  bool flag = !pipe->guiVisible(idx);
                  pipe->showGui(idx, flag);
                  updateContents();
                  MusEGlobal::song->update(SC_RACK);
                  break;
                  }
            case SHOW_NATIVE:
                  {
                  bool flag = !pipe->nativeGuiVisible(idx);
                  pipe->showNativeGui(idx, flag);
                  updateContents();
                  MusEGlobal::song->update(SC_RACK);
                  break;
                  }
            case UP:
                  if (idx > 0) {
                        if(pipe && pipe->at(idx))
                        {
                          setCurrentItem(item(idx-1));
                          MusEGlobal::song->applyOperation(MusECore::UndoOp(
                            MusECore::UndoOp::SwapRackEffectPlugins, track, double(idx), double(idx-1), double(0), double(0), double(0)));
                        }
                        }
                  break;
            case DOWN:
                  if (idx < (MusECore::PipelineDepth-1)) {
                        if(pipe && pipe->at(idx))
                        {
                          setCurrentItem(item(idx+1));
                          MusEGlobal::song->applyOperation(MusECore::UndoOp(
                            MusECore::UndoOp::SwapRackEffectPlugins, track, double(idx), double(idx+1), double(0), double(0), double(0)));
                        }
                        }
                  break;
            case SAVE:
                  savePreset(idx);
                  updateContents();
                  MusEGlobal::song->update(SC_RACK);
                  break;
            }
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
      if (pipe->hasNativeGui(idx))
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
      QString name = MusEGui::getSaveFileName(QString(""), MusEGlobal::preset_file_save_pattern, this,
         tr("MusE: Save Preset"));

      if(name.isEmpty())
        return;

      MusEFile::File f(name, QString(".pre"), this);
      MusEFile::File::ErrorCode res = MusEGui::fileOpen(f, QIODevice::WriteOnly, this, false, true);
      if (res != MusEFile::File::NoError)
      {
        //fprintf(stderr, "EffectRack::savePreset() fopen failed: %s\n", f.errorString().toLocal8Bit().constData());
        return;
      }
      MusECore::Xml xml(f.iodevice());
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
                f.close();
                return;
                }
            }
      else {
          printf("no pipe!\n");
          f.close();
          return;
          }
      f.close();
      }

void EffectRack::startDragItem(int idx)
      {
      if(!track)
        return;
      if (idx < 0) {
          printf("illegal to drag index %d\n",idx);
          return;
      }

      QString xmlconf;

      // fprintf(stderr, "EffectRack::startDrag QTemporaryFile name:%s\n", tmp.fileName().toLocal8Bit().constData());

      MusECore::Xml xml(&xmlconf);
      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe) {
            MusECore::PluginI *pi = (*pipe)[idx];
            if (pi != nullptr) {
                xml.header();
                xml.tag(0, "muse version=\"1.0\"");
                // Write extra information including automation controllers and midi assignments.
                pi->writeConfiguration(1, xml, true);

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

      QMimeData* md = new QMimeData();
      const QByteArray data = xmlconf.toUtf8();

      if (MusEGlobal::debugMsg)
          printf("Sending %" PRIiQSIZETYPE " [%s]\n", data.length(), xmlconf.toLocal8Bit().constData());

      // FIXME: Drag to desktop? Tried, but no luck. Nothing happens.
      //        Tried application/xml, text/xml. text/plain works but just
      //         ends up us binary garbage in a desktop Sticky Note.
      //        Tried passing the tmp filename as URL with setUrls(), the drop
      //         just asks if we want to link to the file. No copy?
      //        If Ctrl key is held for copying, drop says malformed URL. ???
      //        Docs say we may need to register our type. See QMimeData::setData().
      md->setData(MUSE_MIME_TYPE, data);

      QDrag* drag = new QDrag(this);
      drag->setMimeData(md);

      drag->exec(Qt::CopyAction | Qt::MoveAction);
      // NOTE: We don't get here until the drop ends. Exec will block execution until dropped.
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
      const QListWidgetItem *i = itemAt( event->position().toPoint() );
      if (!i)
            return;
      const int idx = row(i);

      const Qt::DropAction act = event->proposedAction();
      EffectRack *ser = nullptr;
      MusECore::AudioTrack *strack = nullptr;
      MusECore::Pipeline* spipe = nullptr;
      QListWidgetItem *sitem = nullptr;
      MusECore::PluginI *splug = nullptr;
      int sidx = -1;

      QWidget *sw = static_cast<QWidget *>(event->source());
      if(sw)
      {
        if(strcmp(sw->metaObject()->className(), "MusEGui::EffectRack") == 0)
        {
          ser = (EffectRack*)sw;
          strack = ser->getTrack();
          if(strack)
          {
            spipe = strack->efxPipe();
            if(spipe)
            {
              sitem = ser->itemAt(ser->getDragPos());
              sidx = ser->row(sitem);

              // Ignore if dragging from/to the same item.
              if(sidx == idx && ser == this)
                return;

              splug = (*spipe)[sidx];
            }
          }
        }
      }

      MusECore::Pipeline* pipe = track->efxPipe();
      if (pipe)
      {
            // Does a plugin already exist in the target slot?
            if ((*pipe)[idx])
            {
              if(QMessageBox::question(this, tr("Replace effect"),tr("Do you really want to replace the effect %1?").arg(pipe->name(idx)),
                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
                return;
            }

            if(event->mimeData()->hasFormat(MUSE_MIME_TYPE))
            {
              const QByteArray mimeData = event->mimeData()->data(MUSE_MIME_TYPE);
              MusECore::Xml xml(mimeData.constData());
              if (MusEGlobal::debugMsg)
                  printf("received %" PRIiQSIZETYPE " [%s]\n", mimeData.size(), mimeData.constData());

              if(act == Qt::MoveAction)
              {
                // If the source is available, it means the drag is from within this app.
                if(splug)
                {
                  // Manipulate the plugins directly instead of using the XML.
                  // The reason is that if we use the XML, then any open UIs will have to close
                  //  and re-open upon re-creation of the plugin from the XML.
                  // Bypassing the XML ensures a smooth move without closing any UIs,
                  //  except for DSSI and external LV2, which cannot update their window titles
                  //  and must be closed and re-opened to update their window titles.
                  // This operation will move controller data and midi mappings as well as the plugin itself.
                  MusEGlobal::song->applyOperation(MusECore::UndoOp(
                    MusECore::UndoOp::MoveRackEffectPlugin, strack, track, sidx, idx));
                }
                // The source is not available. It means the drag is from outside this app instance.
                else
                {
                  // TODO: Investigate how each app instance responds to this. Figure out how to do it.
                  //       The big problem is that to move a plugin from another instance,
                  //        we cannot use direct pointer methods (like above) because there will be no
                  //        event source - that is only given to us if the drag is from INSIDE the app.
                  //       Therefore we must use the dropped XML. And that means if we really want to
                  //        move the whole 'package' we would have to add controller lists and midi maps
                  //        to the dragged XML, which currently is only just the same as preset XML.
                  //       Although this can be done, controller lists introduce a problem: Time values.
                  //       The other app's sample rate would need to be included so conversions could be done.
                  //       And... Well, including controller lists and midi maps inside preset XML
                  //        doesn't sound desirable. We could do it exclusively for drag and drop, not presets,
                  //        but the problem is that the information must already be included in the XML
                  //        at drag time, and if it is to be draggable to the desktop, then we have
                  //        a preset XML file containing controllers and midi maps, sitting on the desktop.
                  //       It is not clear how dragging such a file back into the app should work.
                  //       Use the drop position as an offset for the start of the controller data?
                  //       Or just add the data verbosely?
                  //       Also, we must deal with erasing existing data. Use the global erase settings?
                  //       Another big problem is the undo system: When redo/undo is clicked in one instance,
                  //        the other instance needs to redo/undo as well. Both of them need to sync.
                  //fprintf(stderr, "EffectRack::dropEvent: Drag-move from outside app not supported yet.\n");
                  QMessageBox::information(this, tr("Drag and Drop Effect"),tr("Drag-move from outside app not supported yet"));
                }
              }
              else if(act == Qt::CopyAction)
              {
                // TODO TODO: Ask user if they want controllers and/or midi mapping copied.

                // A copy of the plugin's track automation controllers is provided in the XML.
                // Prepare a new controller list to hold them.
                MusECore::CtrlListList *cll = new MusECore::CtrlListList();
                // A copy of any midi assignments to the plugin's track automation controllers is provided in the XML.
                // Prepare a new mapping list to hold them.
                MusECore::MidiAudioCtrlMap *macm = new MusECore::MidiAudioCtrlMap();

                // Read the plugin and any controllers and midi mappings. The plugin's idx is also set here.
                MusECore::PluginI *newplug = initPlugin(xml, idx, cll, macm);

                // No plugin or no controllers found? Delete the new controller list.
                if(!newplug || cll->empty())
                {
                  // Be sure to delete all the allocated controller items.
                  cll->clearDelete();
                  delete cll;
                  cll = nullptr;
                }
                // No plugin or no midi mappings found? Delete the new mapping list.
                if(!newplug || macm->empty())
                {
                  delete macm;
                  macm = nullptr;
                }

                // Initialize the controller ranges, names, modes etc. with info gathered from the plugin.
                if(newplug && cll)
                {
                  newplug->setupControllers(cll);

                  MusEGlobal::song->applyOperation(MusECore::UndoOp(
                    MusECore::UndoOp::ChangeRackEffectPlugin, track, newplug, idx, cll, macm));
                }
              }
              else
              {
                fprintf(stderr, "EffectRack::dropEvent: Unsupported action type:%d\n", act);
              }
            }
            else if (event->mimeData()->hasUrls())
            {
              // Multiple urls not supported here. Grab the first one.
              const QString text = event->mimeData()->urls()[0].path();

              if (text.endsWith(".pre", Qt::CaseInsensitive) ||
                  text.endsWith(".pre.gz", Qt::CaseInsensitive) ||
                  text.endsWith(".pre.bz2", Qt::CaseInsensitive))
              {
                  MusEFile::File f(text, QString(".pre"), this);
                  MusEFile::File::ErrorCode res = MusEGui::fileOpen(f, QIODevice::ReadOnly, this, false, false);
                  if (res == MusEFile::File::NoError)
                  {
                      MusECore::Xml xml(f.iodevice());
                      MusECore::PluginI *newplug = initPlugin(xml, idx);
                      if(newplug)
                        MusEGlobal::song->applyOperation(MusECore::UndoOp(
                          MusECore::UndoOp::ChangeRackEffectPlugin, track, newplug, idx));
                      f.close();
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
        else if(event->button() & Qt::MiddleButton) {
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

              const QPoint pos = event->pos();
              const QRect itemrect = visualItemRect(i);

              // NOTE: Tried comparing manhattan length distance with QApplication::startDragDistance(),
              //        as per the examples and recommendations. But it does not work correctly here.
              //       If clicked near the bottom and dragged downward for example, it thinks that the
              //        space in the slot below is forbidden and shows the forbidden cursor.
              //       This method seems to work much better.
              if(!itemrect.contains(pos))
                startDragItem(idx0);
        }
      }
      QListWidget::mouseMoveEvent(event);
}

void EffectRack::enterEvent(QEnterEvent *event)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  QListWidget::enterEvent(event);
}

void EffectRack::leaveEvent(QEvent *event)
{
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  QListWidget::leaveEvent(event);
}

MusECore::PluginI* EffectRack::initPlugin(
  MusECore::Xml xml, int idx, MusECore::CtrlListList *cll, MusECore::MidiAudioCtrlMap *macm)
      {
      if(!track)
        return nullptr;
      MusECore::PluginI* plugi = nullptr;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            QString tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        if(plugi)
                        {
                          plugi->initialConfiguration()._ctrlListList.clearDelete();
                          delete plugi;
                        }
                        return nullptr;
                  case MusECore::Xml::TagStart:
                        if (tag == "plugin") {
                              plugi = new MusECore::PluginI();
                              // Set the track and index now, in case anything needs them BEFORE the operations set them.
                              plugi->setTrack(track);
                              plugi->setID(idx);

                              if (plugi->readConfiguration(xml, false, track->channels())) {
                                  // Be sure to clear and delete the controller list.
                                  plugi->initialConfiguration()._ctrlListList.clearDelete();
                                  delete plugi;
                                  plugi = nullptr;
                                  }
                              else {
                                  {
                                    //printf("instantiated!\n");

                                    // TODO Hm... return? Shouldn't it continue with reading more plugins?
                                    //      Well, actually this system is meant for one plugin only.
                                    //      Dragging two or more plugins from source rack positions onto a
                                    //       single destination rack position is a weird concept.
                                    //      What would we do with all the other non-leading dragged plugins?
                                    //      Hard to imagine how it would work. Where would they all be dropped?
                                    //      But we'll leave out the return so that other elements (like automation)
                                    //       could potentially be included and scanned in the xml.
                                    //return;
                                    }
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
                        {
                              if(plugi)
                              {
                                //---------------------------------------------------------
                                // If any automation controllers were included with in XML,
                                //  convert controller IDs and transfer to given list.
                                //---------------------------------------------------------
                                MusECore::CtrlListList &conf_cll = plugi->initialConfiguration()._ctrlListList;
                                if(cll)
                                {
                                  for(MusECore::ciCtrlList icl = conf_cll.cbegin(); icl != conf_cll.cend(); )
                                  {
                                    MusECore::CtrlList *cl = icl->second;
                                    if(cl->id() < 0)
                                    {
                                      // Controller is orphaned now. Delete it.
                                      delete cl;
                                    }
                                    else
                                    {
                                      // Strip away the controller's rack position bits,
                                      //  leaving just the controller numbers.
                                      // Still, they should already be stripped by now.
                                      const int m = cl->id() & AC_PLUGIN_CTL_ID_MASK;
                                      // Generate the new id.
                                      const unsigned long new_id = MusECore::genACnum(idx, m);
                                      cl->setId(new_id);
                                      const bool res = cll->add(cl);
                                      if(!res)
                                      {
                                        // Controller is orphaned now. Delete it.
                                        delete cl;
                                        fprintf(stderr, "EffectRack::initPlugin: Error: Could not add controller #%ld!\n", new_id);
                                      }
                                    }
                                    // Done with the item. Erase it. Iterator will point to the next item.
                                    icl = conf_cll.erase(icl);
                                  }
                                  // All of the items should be erased by now.
                                }
                                else
                                {
                                  // Controllers were not transferred or deleted.
                                  // Clear the list and delete the items.
                                  conf_cll.clearDelete();
                                }

                                //---------------------------------------------------------
                                // If any midi controller mappings were included with in XML,
                                //  convert controller IDs and transfer to given list.
                                //---------------------------------------------------------
                                MusECore::MidiAudioCtrlMap &conf_macm = plugi->initialConfiguration()._midiAudioCtrlMap;
                                if(macm)
                                {
                                  for(MusECore::iMidiAudioCtrlMap imacm = conf_macm.begin(); imacm != conf_macm.end(); )
                                  {
                                    MusECore::MidiAudioCtrlStruct &macs = imacm->second;
                                    // Strip away the controller ID's rack position bits,
                                    //  leaving just the controller numbers.
                                    // Still, they should already be stripped by now.
                                    const int m = macs.id() & AC_PLUGIN_CTL_ID_MASK;
                                    // Generate the new id.
                                    const unsigned long new_id = MusECore::genACnum(idx, m);
                                    macs.setId(new_id);
                                    macs.setTrack(track);
                                    macm->add_ctrl_struct(imacm->first, macs);
                                    // Done with the item. Erase it. Iterator will point to the next item.
                                    imacm = conf_macm.erase(imacm);
                                  }
                                  // All of the items should be erased by now.
                                }
                                else
                                {
                                  // Mappings were not transferred. Clear the list.
                                  conf_macm.clear();
                                }
                              }
                              return plugi;
                        }
                  default:
                        break;
                  }
            }
      }                        

} // namespace MusEGui
