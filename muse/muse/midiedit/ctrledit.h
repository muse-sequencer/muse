//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrledit.h,v 1.9 2006/02/08 17:33:41 wschweer Exp $
//  (C) Copyright 1999-2006 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_EDIT_H__
#define __CTRL_EDIT_H__

#include "ctrl/ctrleditor.h"
#include "gui.h"

class SimpleButton;
class TimeCanvas;

//---------------------------------------------------------
//   CtrlEdit
//---------------------------------------------------------

class CtrlEdit : public QObject, public CtrlEditor {
      Q_OBJECT

    	Track* const _track;
      Track* _ctrlTrack;
      Ctrl* _ctrl;
      TimeCanvas* _tc;
      int _height;

      virtual Ctrl* ctrl() const { return _ctrl; }
      virtual TimeCanvas* tc() const { return _tc; }
      virtual Track* track() const { return _track; }
      virtual Track* ctrlTrack() const { return _ctrlTrack; }

   public:
      int ctrlId;
      int y;
      SimpleButton* minus;
      SimpleButton* sel;
      QMenu* ctrlList;
      bool _drawCtrlName;

   private slots:
      void populateController();
      void changeController(QAction*);
      void controllerListChanged(int);

   public:
      CtrlEdit(QWidget*, TimeCanvas*, Track*);
      ~CtrlEdit();
      int pixel2val(int) const;
      void setHeight(int val)     { _height = val; }
      int height() const          { return _height; }
      virtual int cheight() const { return _height - splitWidth; }
      void setCtrl(Ctrl* c)       { _ctrl = c; }
      Ctrl* ctrl()                { return _ctrl; }
      void setSinglePitch(int);
      };

typedef std::vector<CtrlEdit*> CtrlEditList;
typedef CtrlEditList::iterator iCtrlEdit;
typedef CtrlEditList::const_iterator ciCtrlEdit;
#endif

