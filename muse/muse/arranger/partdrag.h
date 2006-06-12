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
      static bool canDecode(const QMimeData*);
      static bool decode(const QMimeData* s, Part*& p);
      };

//---------------------------------------------------------
//   AudioPartDrag
//---------------------------------------------------------

class AudioPartDrag : public QDrag {
      static const char type[];
      Q_OBJECT

   public:
      AudioPartDrag(Part*, QWidget* src);
      static bool canDecode(const QMimeData*);
      static bool decode(const QMimeData* s, Part*& p);
      };

//---------------------------------------------------------
//   WavUriDrag
//---------------------------------------------------------

class WavUriDrag : public QDrag {
      static const char type[];
      Q_OBJECT

   public:
      WavUriDrag(const QString&, QWidget* src);
      static bool canDecode(const QMimeData*);
      static bool decode(const QMimeData* s, QString* p);
      };



#endif

