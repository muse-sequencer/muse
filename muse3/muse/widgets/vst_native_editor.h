//=========================================================
//  MusE
//  Linux Music Editor
//  vst_native_editor.h
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

#ifndef __VST_NATIVE_EDITOR_H__
#define __VST_NATIVE_EDITOR_H__

#include <QWidget>

#include "config.h"

#ifdef VST_NATIVE_SUPPORT

#if defined(Q_WS_X11)
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#undef Bool
#undef Status
#undef None
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef Type
#undef FontChange
#undef CursorShape
#undef Unsorted
typedef void (*XEventProc)(XEvent *);
#endif

namespace MusECore {
class VstNativeSynthIF;
}

#endif  // VST_NATIVE_SUPPORT

namespace MusEGui {

class VstNativeEditor : public QWidget
{
    Q_OBJECT

#ifdef VST_NATIVE_SUPPORT

#if defined(Q_WS_X11)
    Display*   _display;
    Window     _vstEditor;
    XEventProc _vstEventProc;
    bool       _buttonPress;
#endif

    MusECore::VstNativeSynthIF* _sif;
        
protected:

    virtual void showEvent(QShowEvent *pShowEvent);
    virtual void closeEvent(QCloseEvent *pCloseEvent);
    virtual void moveEvent(QMoveEvent *pMoveEvent);
    virtual void resizeEvent(QResizeEvent *pResizeEvent);

public:
    VstNativeEditor(QWidget *parent, Qt::WindowFlags wflags = 0);
    ~VstNativeEditor();

    void open(MusECore::VstNativeSynthIF* sif);
    //void close();

#if defined(Q_WS_X11)
    // Local X11 event filter.
    bool x11EventFilter(XEvent *pEvent);
#endif

    //MusECore::VstNativeSynthIF* sif() const { return _sif; }
    
#endif  // VST_NATIVE_SUPPORT
};

} // namespace MusEGui

#endif
