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

#ifndef __DEVICE_H__
#define __DEVICE_H__

//---------------------------------------------------------
//   Device
//---------------------------------------------------------

class Device {

   public:
      enum DeviceType { MidiDevice, WaveDevice };

   protected:
      QString _name;
      DeviceType _type;
      int _port;

   public:
      Device() {}
      virtual ~Device() {}
      Device(const QString& name, DeviceType t = MidiDevice)
         : _name(name), _type(t) {}

      virtual QString open(int) = 0;
      virtual void close() = 0;

      const QString& name() const      { return _name; }
      void setName(const QString& s)   { _name = s; }
      const DeviceType type() const    { return _type; }
      void setDeviceType(DeviceType t) { _type = t; }
      int port() const                 { return _port; }
      void setPort(int p)              { _port = p; }
      };

#endif

