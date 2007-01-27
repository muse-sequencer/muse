//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "partdrag.h"
#include "al/xml.h"
#include "part.h"

const char MidiPartDrag::type[] = "application/muse/part/midi";
const char AudioPartDrag::type[] = "application/muse/part/audio";
const char WavUriDrag::type[] = "text/uri-list";

//---------------------------------------------------------
//   MidiPartDrag
//---------------------------------------------------------

MidiPartDrag::MidiPartDrag(Part* part, QWidget* src)
   : QDrag(src)
      {
      QBuffer buffer;
      buffer.open(QIODevice::WriteOnly);
      AL::Xml xml(&buffer);
      part->write(xml);
      buffer.close();
      QMimeData* mimeData = new QMimeData;
      mimeData->setData(type, buffer.buffer());
      setMimeData(mimeData);
      }

//---------------------------------------------------------
//   canDecode
//---------------------------------------------------------

bool MidiPartDrag::canDecode(const QMimeData* s)
      {
      return s->hasFormat(type);
      }

//---------------------------------------------------------
//   decode
//---------------------------------------------------------

bool MidiPartDrag::decode(const QMimeData* s, Part*& p)
      {
      p = 0;
      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(s->data(type), false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n    at line: " + ln + " col: " + col;
            printf("error parsing part: %s\n", error.toLatin1().data());
            return false;
            }
      for (QDomNode node = doc.documentElement(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "part") {
                  p = new Part(0);
                  p->ref();
                  p->read(node, true);
                  }
            else
                  printf("MusE: %s not supported\n", e.tagName().toLatin1().data());
            }
      return (p != 0);
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
      QBuffer buffer;
      buffer.open(QIODevice::WriteOnly);
      Xml xml(&buffer);
      part->write(xml);
      buffer.close();

      QMimeData* mimeData = new QMimeData;
      mimeData->setData(type, buffer.buffer());
      setMimeData(mimeData);
      }

//---------------------------------------------------------
//   canDecode
//---------------------------------------------------------

bool AudioPartDrag::canDecode(const QMimeData* s)
      {
      return s->hasFormat(type);
      }

//---------------------------------------------------------
//   decode
//---------------------------------------------------------

bool AudioPartDrag::decode(const QMimeData* s, Part*& p)
      {
      p = 0;
      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(s->data(type), false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n    at line: " + ln + " col: " + col;
            printf("error parsing part: %s\n", error.toLatin1().data());
            return false;
            }
      for (QDomNode node = doc.documentElement(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "part") {
                  p = new Part(0);
                  p->ref();
                  p->read(node, false);
                  }
            else
                  printf("MusE: %s not supported\n", e.tagName().toLatin1().data());
            }
      return (p != 0);
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

bool WavUriDrag::canDecode(const QMimeData* s)
      {
      if (!s->hasFormat(type))
            return false;
      QByteArray data = s->data(type);
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

bool WavUriDrag::decode(const QMimeData* s, QString* uri)
      {
      QByteArray data = s->data(type);
      QUrl url(data);
      *uri = url.toLocalFile().trimmed();
      return true;
      }

