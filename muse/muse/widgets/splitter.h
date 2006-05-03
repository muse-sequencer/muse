//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: splitter.h,v 1.8 2005/11/23 13:55:32 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SPLITTER_H__
#define __SPLITTER_H__

namespace AL {
      class Xml;
      };

using AL::Xml;

//---------------------------------------------------------
//   SplitterHandle
//---------------------------------------------------------

class SplitterHandle : public QSplitterHandle {
      Q_OBJECT
      virtual void paintEvent(QPaintEvent*);

   public:
      SplitterHandle(Qt::Orientation o, QSplitter* parent)
         : QSplitterHandle(o, parent) {
            }
      };

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

class Splitter : public QSplitter {
      Q_OBJECT

   protected:
      virtual QSplitterHandle* createHandle() {
            return new SplitterHandle(orientation(), this);
            }
   public:
      Splitter(Qt::Orientation);
      void writeStatus(const char* name, Xml&);
      void readStatus(QDomNode node);
      };

#endif

