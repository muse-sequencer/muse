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

#ifndef __PORT_H__
#define __PORT_H__

#include <jack/jack.h>

//---------------------------------------------------------
//    Port
//---------------------------------------------------------

class Port {
      enum { JACK_TYPE, ALSA_TYPE, ZERO_TYPE } type;
      union {
            jack_port_t* _jackPort;
            struct {
                  unsigned char _alsaPort;
                  unsigned char _alsaClient;
                  };
            };
   public:
      Port() { 
            type = ZERO_TYPE; 
            }
      Port(jack_port_t* p) {
            _jackPort = p;
            type = JACK_TYPE;
            }
      Port(unsigned char client, unsigned char port) {
            _alsaPort = port;
            _alsaClient = client;
            type = ALSA_TYPE;
            }
      void setZero()       { type = ZERO_TYPE; }
      bool isZero()  const { return type == ZERO_TYPE; }
      bool operator==(const Port& p) const {
            if (type == JACK_TYPE)
                  return _jackPort == p._jackPort;
            else if (type == ALSA_TYPE)
                  return _alsaPort == p._alsaPort && _alsaClient == p._alsaClient;
            else 
                  return true;
            }
      bool operator<(const Port& p) const {
            if (type == ALSA_TYPE) {
                  if (_alsaPort != p._alsaPort)
                        return _alsaPort < p._alsaPort;
                  return _alsaClient < p._alsaClient;
                  }
            return false;
            }
      unsigned char alsaPort() const   { return _alsaPort; }
      unsigned char alsaClient() const { return _alsaClient; }
      jack_port_t* jackPort() const    { return _jackPort; }
      };

#endif

