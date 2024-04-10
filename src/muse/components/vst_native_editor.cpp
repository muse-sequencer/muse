//=========================================================
//  MusE
//  Linux Music Editor
//  vst_native_editor.cpp
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//  Some of the editor window coding was adapted from QTractor (by rncbc aka Rui Nuno Capela)
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

#include "config.h"

#ifdef VST_NATIVE_SUPPORT

#include "vst_native_editor.h"
#include "vst_native.h"
//#include "gconfig.h"
// REMOVE Tim. tmp. Added.
//#include "song.h"

#include <QtGlobal>
#if defined(Q_WS_X11)
#include <QX11Info>
#endif
#include <QApplication>


namespace MusEGui {

//---------------------------------------------------------------------
// Helpers for editor widget.
//---------------------------------------------------------------------

#if defined(Q_WS_X11)

static bool g_bXError = false;

static int tempXErrorHandler ( Display *, XErrorEvent * )
{
        g_bXError = true;
        return 0;
}

static XEventProc getXEventProc ( Display *pDisplay, Window w )
{
        int iSize;
        unsigned long iBytes, iCount;
        unsigned char *pData;
        XEventProc eventProc = nullptr;
        Atom aType, aName = XInternAtom(pDisplay, "_XEventProc", false);

        g_bXError = false;
        XErrorHandler oldErrorHandler = XSetErrorHandler(tempXErrorHandler);
        XGetWindowProperty(pDisplay, w, aName, 0, 1, false,
                AnyPropertyType, &aType,  &iSize, &iCount, &iBytes, &pData);
        if (g_bXError == false && iCount == 1)
                eventProc = (XEventProc) (pData);
        XSetErrorHandler(oldErrorHandler);

        return eventProc;
}

static Window getXChildWindow ( Display *pDisplay, Window w )
{
        Window wRoot, wParent, *pwChildren;
        unsigned int iChildren = 0;

        XQueryTree(pDisplay, w, &wRoot, &wParent, &pwChildren, &iChildren);

        return (iChildren > 0 ? pwChildren[0] : 0);
}

#endif // Q_WS_X11

VstNativeEditor::VstNativeEditor(QWidget *parent, Qt::WindowFlags wflags)
  : QWidget(parent, wflags),
        #if defined(Q_WS_X11)
          _display(QX11Info::display()),
          _vstEditor(0),
          _vstEventProc(0),
          _buttonPress(false),
        #endif
          _sif(nullptr)
{
  setAttribute(Qt::WA_DeleteOnClose);
  m_fixScaling = false;

// REMOVE Tim. tmp. Added.
  // _songChangedMetaConn = connect(MusEGlobal::song, &MusECore::Song::songChanged,
  //                                [this](MusECore::SongChangedStruct_t type) { songChanged(type); } );

  // TODO TEST Test if these might be helpful, especially opaque event.
            //setBackgroundRole(QPalette::NoRole);
            //setAttribute(Qt::WA_NoSystemBackground);
  //         setAttribute(Qt::WA_StaticContents);
  //         // This is absolutely required for speed! Otherwise painfully slow because of full background
  //         //  filling, even when requesting small udpdates! Background is drawn by us.
            //setAttribute(Qt::WA_OpaquePaintEvent);
  //         //setFrameStyle(QFrame::Raised | QFrame::StyledPanel);
}

VstNativeEditor::~VstNativeEditor()
{
// REMOVE Tim. tmp. Added.
   // // In case closeEvent() wasn't already called, where we disconnect there as well.
   // disconnect(_songChangedMetaConn);

// REMOVE Tim. tmp. Removed.
// We delete on close already. Calling close() calls QWidget::close() causing recursive crash.
// Also changed "delete _editor" to "_editor->close()" in VstNativeSynthIF::deactivate3().
   //close();
   if(_sif)
   {
     _sif->editorDeleted();
     _sif = nullptr;
   }
   if(_pstate)
   {
      _pstate->editorDeleted();
      _pstate = nullptr;
   }
}

// REMOVE Tim. tmp. Added.
// void VstNativeEditor::songChanged(MusECore::SongChangedStruct_t type)
// {
//   // Catch when the track name changes or track is moved or the rack position changes.
//   if(type & (SC_TRACK_MODIFIED | SC_TRACK_MOVED | SC_RACK))
//     updateWindowTitle();
// }

// REMOVE Tim. tmp. Added.
// void VstNativeEditor::updateWindowTitle(const QString& title)
// {
//   QString windowTitle = "VST plugin editor";
//   if(_sif && _sif->track())
//   {
// // REMOVE Tim. tmp. Changed.
//      windowTitle = _sif->track()->name() + ":" + _sif->pluginLabel();
// //     windowTitle = _sif->track()->name() + ":" + _sif->name();
//   }
//   else if(_pstate && _pstate->pluginI && _pstate->pluginI->track())
//   {
// //     windowTitle = _pstate->pluginI->track()->name() + ":" + _pstate->pluginWrapper->_synth->name();
//      windowTitle = _pstate->pluginI->track()->name() + ":" + _pstate->pluginI->pluginLabel();
//   }
//
//   setWindowTitle(windowTitle);
// }

void VstNativeEditor::updateWindowTitle(const QString& title)
{
  setWindowTitle(title);
}

//---------------------------------------------------------------------
// open
//---------------------------------------------------------------------

void VstNativeEditor::open(MusECore::VstNativeSynthIF* sif, MusECore::VstNativePluginWrapper_State *state)
{
  _sif = sif;
  _pstate = state;

  // Start the proper (child) editor...
  long  value = 0;
  void *ptr = (void *) winId();
#if defined(Q_WS_X11)
  value = (long) _display;
#endif

  MusECore::VstRect* pRect;

  AEffect *vstPlug = _sif ? _sif->_plugin : _pstate->plugin;

  vstPlug->dispatcher(vstPlug, effEditOpen, 0, value, ptr, 0.0f);

  const MusECore::PluginQuirks& quirks = _sif ? _sif->cquirks() : _pstate->pluginI->cquirks();
  m_fixScaling = quirks.fixNativeUIScaling();

  if(vstPlug->dispatcher(vstPlug, effEditGetRect, 0, 0, &pRect, 0.0f))
  {
      int w = pRect->right - pRect->left;
      int h = pRect->bottom - pRect->top;
      if (w > 0 && h > 0)
      {
          if (fixScaling() && devicePixelRatio() >= 1.0) {
              w = qRound((qreal)w / devicePixelRatio());
              h = qRound((qreal)h / devicePixelRatio());
              setFixedSize(w, h);
          } else {
              QWidget::setMinimumSize(w, h);
              if ((w != width()) || (h != height()))
                  setFixedSize(w, h);
          }
      }
  }


  //int rv = _sif->dispatch(effEditOpen, 0, value, ptr, 0.0f);
  //fprintf(stderr, "VstNativeEditor::open effEditOpen returned:%d effEditGetRect rect l:%d r:%d t:%d b:%d\n", rv, pRect->left, pRect->right, pRect->top, pRect->bottom); // REMOVE Tim.
  
#if defined(Q_WS_X11)
  _vstEditor = getXChildWindow(_display, (Window) winId());
  if(_vstEditor)
    _vstEventProc = getXEventProc(_display, _vstEditor);
#endif
    
// REMOVE Tim. tmp. Changed.
//   QString windowTitle = "VST plugin editor";
//   if(_sif && _sif->track())
//   {
// // REMOVE Tim. tmp. Changed.
// //     windowTitle = _sif->track()->name() + ":" + _sif->pluginLabel();
//      windowTitle = _sif->track()->name() + ":" + _sif->name();
//   }
//   else if(_pstate && _pstate->pluginI->track())
//   {
//      windowTitle = _pstate->pluginI->track()->name() + ":" + _pstate->pluginWrapper->_synth->name();
//   }
//
//   setWindowTitle(windowTitle);
//   updateWindowTitle();

  //_sif->editorOpened();
  if(!isVisible())
    show();
  raise();
  activateWindow();
  ///_sif->idleEditor();  // REMOVE Tim. Or keep.

  //resizeTimerId = startTimer(500);
}

#if defined(Q_WS_X11)

//---------------------------------------------------------------------
// x11EventFilter
//---------------------------------------------------------------------

bool VstNativeEditor::x11EventFilter(XEvent *pEvent)
{
  if(_vstEventProc && pEvent->xany.window == _vstEditor)
  {
    // Avoid mouse tracking events...
    switch (pEvent->xany.type) {
    case ButtonPress:
      _buttonPress = true;
      break;
    case ButtonRelease:
      _buttonPress = false;
      break;
    case MotionNotify:
      if(!_buttonPress)
        return false;
      // Fall thru...
    default:
      break;
    }
    // Process as intended...
    (*_vstEventProc)(pEvent);
    return true;
  }
  else
    return false;
}

#endif

//---------------------------------------------------------------------
// showEvent
//---------------------------------------------------------------------

void VstNativeEditor::showEvent(QShowEvent *pShowEvent)
{
  QWidget::showEvent(pShowEvent);

  if(_sif)
    _sif->editorOpened();
  if(_pstate)
     _pstate->editorOpened();
}

//---------------------------------------------------------------------
// closeEvent
//---------------------------------------------------------------------

void VstNativeEditor::closeEvent(QCloseEvent *pCloseEvent)
{
   /*if(resizeTimerId)
   {
      killTimer(resizeTimerId);
      resizeTimerId = 0;
   }*/

// REMOVE Tim. tmp. Added.
   // // Disconnect before songChanged() has a chance to be called.
   // // The caller might destroy some things before calling close(),
   // //  and songChanged() might be called later which causes crashes.
   // // We also do this in the destructor just in case.
   // disconnect(_songChangedMetaConn);

   pCloseEvent->accept();
   QWidget::closeEvent(pCloseEvent);
}

void VstNativeEditor::close()
{
   // Note that we delete on close, but that will happen later.
   QWidget::close();
   if(_sif)
   {
     _sif->dispatch(effEditClose, 0, 0, nullptr, 0.0f);
     _sif->editorClosed();
   }

   if(_pstate && _pstate->plugin)
   {
      _pstate->plugin->dispatcher(_pstate->plugin, effEditClose, 0, 0, nullptr, 0.0f);
      _pstate->editorClosed();
   }
}

//---------------------------------------------------------------------
// moveEvent
//---------------------------------------------------------------------

void VstNativeEditor::moveEvent(QMoveEvent *pMoveEvent)
{
  QWidget::moveEvent(pMoveEvent);
#if defined(Q_WS_X11)
  if(_vstEditor)
  {
    XMoveWindow(_display, _vstEditor, 0, 0);
    //QWidget::update();  // REMOVE Tim. Or keep? Commented in Qtractor.
  }
#endif
}

void VstNativeEditor::resizeEvent(QResizeEvent *pResizeEvent)
{
   //setFixedSize(pResizeEvent->size());
   pResizeEvent->accept();
}

void VstNativeEditor::timerEvent(QTimerEvent *event)
{
   if(event->timerId() == resizeTimerId)
   {
      MusECore::VstRect* pRect;
      bool bGotRect = false;
      if(_sif)
         bGotRect = _sif->dispatch(effEditGetRect, 0, 0, &pRect, 0.0f);
      else if(_pstate)
         bGotRect = _pstate->plugin->dispatcher(_pstate->plugin, effEditGetRect, 0, 0, &pRect, 0.0f);
      if(bGotRect)
      {
         int w = pRect->right - pRect->left;
         int h = pRect->bottom - pRect->top;
         if (w > 0 && h > 0)
         {
            if((w != width()) || (h != height()))
            {
               setFixedSize(w, h);
            }
         }
      }

   }
}

} // namespace MusEGui

#endif // VST_NATIVE_SUPPORT

