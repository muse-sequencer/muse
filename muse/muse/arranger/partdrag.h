//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: partdrag.h,v 1.3 2005/11/04 12:03:35 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PARTDRAG_H__
#define __PARTDRAG_H__

class Part;

//---------------------------------------------------------
//   MidiPartDrag
//---------------------------------------------------------

class MidiPartDrag : public QDrag {
      static const char type[];
      Q_OBJECT

   public:
      MidiPartDrag(Part*, QWidget* src);
      static bool canDecode(const QMimeSource*);
      static bool decode(const QMimeSource* s, Part*& p);
      };

//---------------------------------------------------------
//   AudioPartDrag
//---------------------------------------------------------

class AudioPartDrag : public QDrag {
      static const char type[];
      Q_OBJECT

   public:
      AudioPartDrag(Part*, QWidget* src);
      static bool canDecode(const QMimeSource*);
      static bool decode(const QMimeSource* s, Part*& p);
      };

//---------------------------------------------------------
//   WavUriDrag
//---------------------------------------------------------

class WavUriDrag : public QDrag {
      static const char type[];
      Q_OBJECT

   public:
      WavUriDrag(const QString&, QWidget* src);
      static bool canDecode(const QMimeSource*);
      static bool decode(const QMimeSource* s, QString* p);
      };



#endif

