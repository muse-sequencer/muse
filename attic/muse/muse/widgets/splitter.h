//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: splitter.h,v 1.1.1.1 2003/10/27 18:54:51 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SPLITTER_H__
#define __SPLITTER_H__

#include <qsplitter.h>

class Xml;

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

class Splitter : public QSplitter {
      Q_OBJECT

   public:
      Splitter(Qt::Orientation o, QWidget* parent, const char* name);
      void writeStatus(int level, Xml&);
      void readStatus(Xml&);
      };

#endif

