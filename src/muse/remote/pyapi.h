//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
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
#ifndef PYAPI_H
#define PYAPI_H

#include <QObject>
#include <QThread>

namespace MusECore {

class PyroServerThread : public QThread
{
    Q_OBJECT
    bool _runServer;

public slots:
    void run() override;
    void stop();

public:
    PyroServerThread(QObject *parent = nullptr) : QThread(parent), _runServer(false) { }
    bool initServer();
    bool serverRunFlag() const { return _runServer; }
};

bool startPythonBridge();
bool stopPythonBridge();

} // namespace MusECore

#endif

