//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: partdrag.cpp,v 1.5 2006/01/25 22:25:48 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "partdrag.h"

class Part;

const char MidiPartDrag::type[] = "application/muse/part/midi";
const char AudioPartDrag::type[] = "application/muse/part/audio";
const char WavUriDrag::type[] = "text/uri-list";

//---------------------------------------------------------
//   MidiPartDrag
//    does only transfer reference to part, this does
//    not allow for transfers between different apps
//    TODO: transfer content (xml representation)
//---------------------------------------------------------

MidiPartDrag::MidiPartDrag(Part* part, QWidget* src)
   : QDrag(src)
      {
      QByteArray a((const char*)&part, sizeof(part));
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(type, a);
      setMimeData(mimeData);
      }

//---------------------------------------------------------
//   canDecode
//---------------------------------------------------------

bool MidiPartDrag::canDecode(const QMimeSource* s)
      {
      return !strcmp(s->format(0), type);
      }

//---------------------------------------------------------
//   decode
//---------------------------------------------------------

bool MidiPartDrag::decode(const QMimeSource* s, Part*& p)
      {
      QByteArray a = s->encodedData(type);
      char* cp = (char*)(&p);
      for (unsigned i = 0; i < sizeof(p); ++i)
            *cp++ = a[i];
      return true;
      }

//---------------------------------------------------------
//   PartDrag
//    does only transfer reference to part, this does
//    not allow for transfers between different apps
//    TODO: transfer content (xml representation)
//---------------------------------------------------------

AudioPartDrag::AudioPartDrag(Part* part, QWidget* src)
   : QDrag(src)
      {
      QByteArray a((char*)&part, sizeof(part));

      QMimeData* mimeData = new QMimeData;
      mimeData->setData(type, a);
      setMimeData(mimeData);
      }

//---------------------------------------------------------
//   canDecode
//---------------------------------------------------------

bool AudioPartDrag::canDecode(const QMimeSource* s)
      {
      return !strcmp(s->format(0), type);
      }

//---------------------------------------------------------
//   decode
//---------------------------------------------------------

bool AudioPartDrag::decode(const QMimeSource* s, Part*& p)
      {
      QByteArray a = s->encodedData(type);
      char* cp = (char*)(&p);
      for (unsigned i = 0; i < sizeof(p); ++i)
            *cp++ = a[i];
      return true;
      }

//---------------------------------------------------------
//   WavUriDrag
//---------------------------------------------------------

WavUriDrag::WavUriDrag(const QString& s, QWidget* src)
   : QDrag(src)
      {
      QByteArray a(s.toAscii());
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(type, a);
      setMimeData(mimeData);
      }

//---------------------------------------------------------
//   canDecode
//---------------------------------------------------------

bool WavUriDrag::canDecode(const QMimeSource* s)
      {
      if (strcmp(s->format(0), type))
            return false;
      QByteArray data = s->encodedData("text/uri-list");
      QUrl url(data);
      if (url.scheme() != "file")
            return false;
      QFileInfo fi(url.toLocalFile().trimmed());
      if (!fi.exists()) {
            printf("drag file <%s> does not exist\n", fi.filePath().toLatin1().data());
            return false;
            }
      if (fi.suffix() != "wav") {
            printf("drag file <%s> has no wav suffix\n", fi.filePath().toLatin1().data());
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   decode
//---------------------------------------------------------

bool WavUriDrag::decode(const QMimeSource* s, QString* uri)
      {
      QByteArray data = s->encodedData("text/uri-list");
      QUrl url(data);
      *uri = url.toLocalFile().trimmed();
      return true;
      }

