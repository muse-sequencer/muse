//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cobject.h,v 1.3.2.1 2005/12/11 21:29:24 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __COBJECT_H__
#define __COBJECT_H__

#include "config.h"

#include <QMainWindow>
#include <list>

class Xml;

//---------------------------------------------------------
//   TopWin
//---------------------------------------------------------

class TopWin : public QMainWindow
      {
      Q_OBJECT

   public:
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      TopWin(QWidget* parent=0, const char* name=0,
         Qt::WindowFlags f = Qt::Window);
      };

//---------------------------------------------------------
//   Toplevel
//---------------------------------------------------------

class Toplevel {
   public:
      enum ToplevelType { PIANO_ROLL, LISTE, DRUM, MASTER, WAVE, 
         LMASTER, CLIPLIST, MARKER, SCORE
#ifdef PATCHBAY
         , M_PATCHBAY
#endif /* PATCHBAY */
         };
      Toplevel(ToplevelType t, unsigned long obj, TopWin* cobj) {
            _type = t;
            _object = obj;
            _cobject = cobj;
            }
      ToplevelType type() const { return _type; }
      unsigned long object()        const { return _object; }
      TopWin* cobject()   const { return _cobject; }

   private:
      ToplevelType _type;
      unsigned long _object;
      TopWin* _cobject;
      };

typedef std::list <Toplevel> ToplevelList;
typedef ToplevelList::iterator iToplevel;
typedef ToplevelList::const_iterator ciToplevel;

#endif

