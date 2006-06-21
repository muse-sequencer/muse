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

#ifndef __DRIVER_H__
#define __DRIVER_H__

typedef void* Port;

struct PortName {
      Port port;
      QString name;
      };

//---------------------------------------------------------
//   Driver
//    abstract driver base class; used for midi and
//    audio
//---------------------------------------------------------

class Driver {

   public:
      Driver() {}
      virtual ~Driver() {}
      virtual bool init() = 0;

      virtual std::list<PortName>* outputPorts() = 0;
      virtual std::list<PortName>* inputPorts()  = 0;

      virtual Port registerOutPort(const QString&) = 0;
      virtual Port registerInPort(const QString&) = 0;
      virtual void unregisterPort(Port) = 0;
      virtual void setPortName(Port p, const QString&) = 0;
      virtual QString portName(Port) = 0;
      virtual Port findPort(const QString&) = 0;

      virtual bool connect(Port, Port) = 0;
      virtual bool disconnect(Port, Port) = 0;
      virtual bool equal(Port, Port) = 0;
      };

#endif

