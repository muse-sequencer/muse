//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: device.h,v 1.1.1.1 2003/10/27 18:51:58 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __DEVICE_H__
#define __DEVICE_H__

class QString;

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

