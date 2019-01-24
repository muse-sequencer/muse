//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_rdf.h
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __PLUGIN_RDF_H__
#define __PLUGIN_RDF_H__

#include <QString>
#include <QStringList>

namespace MusEPlugin {

// This might be called recursively!
void scanLrdfDir(const QString& dirname,
                 QStringList* rdfs,
                 bool debugStdErr,
                 // Only for recursions, original top caller should not touch!
                 int recurseLevel = 0);

void scanLrdfPlugins(QStringList* rdfs, bool debugStdErr);
  
  
} // namespace MusEPlugin

#endif
