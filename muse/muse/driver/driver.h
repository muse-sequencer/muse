//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: driver.h,v 1.4 2005/12/19 16:16:27 wschweer Exp $
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

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

