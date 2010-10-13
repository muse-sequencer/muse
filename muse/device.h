//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: device.h,v 1.1.1.1 2003/10/27 18:51:58 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <qstring.h>

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

