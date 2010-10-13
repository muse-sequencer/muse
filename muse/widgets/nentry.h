//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: nentry.h,v 1.1.1.1.2.2 2008/05/21 00:28:54 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __NENTRY_H__
#define __NENTRY_H__

#include <q3frame.h>
#include <qlineedit.h>
#include <qstring.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <QEvent>
#include <Q3HBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QContextMenuEvent>
#include <QKeyEvent>

class QTimer;
class Q3HBoxLayout;
class QLabel;

class NentryFilter : public QObject {
      Q_OBJECT

   protected:
      bool eventFilter(QObject* object, QEvent* event);
   public:
      NentryFilter(QObject* parent);
      };

//---------------------------------------------------------
//   Nentry
//    numerical entry widget with optional label
//---------------------------------------------------------

class Nentry : public Q3Frame {
      Q_OBJECT

      int button;
      int starty;
      bool drawFrame;
      QTimer* timer;
      int evx;
      int timecount;
      Q3HBoxLayout* layout;
      QObject* filter;
      QLabel* label;
      int lPos;   // label Position 0 - left, 1 - right
      QWidget* focusW;

   protected:
      QLineEdit* edit;
      int val;
      virtual void incValue(int x) = 0;
      virtual void decValue(int x) = 0;
      virtual bool setString(int, bool editable = false) = 0;
      virtual bool setSValue(const QString&) = 0;

   private slots:
      void repeat();

   protected slots:
      void endEdit();

   public slots:
      virtual void setValue(int);

   public:
      Nentry(QWidget* parent, const QString& txt = QString(""),
         int lPos = 0, bool dark=false);

      int value() const { return val; }
      void setFrame(bool);
      //void setAlignment(int flag)    { edit->setAlignment(flag); }
      void setText(const QString& s);
      void setSize(int n);
      void setDark();

      void mousePress(QMouseEvent*);
      void mouseMove(QMouseEvent*);
      void mouseDoubleClick(QMouseEvent*);
      void mouseRelease(QMouseEvent*);
      void wheel(QWheelEvent*);
      bool keyPress(QKeyEvent*);
      void setFocusPolicy(Qt::FocusPolicy);
      bool contextMenu(QContextMenuEvent*);
      };
#endif
