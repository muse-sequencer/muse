//=========================================================
//  MusE
//  Linux Music Editor
//
//  plugin_rdf.cpp
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>

#include "plugin_rdf.h"
#include "config.h"

#ifdef HAVE_LRDF
  #include <lrdf.h>
#endif // HAVE_LRDF

#include <cstring>

// For debugging output: Uncomment the fprintf section.
#define DEBUG_PLUGIN_RDF(dev, format, args...)  // std::fprintf(dev, format, ##args);

namespace MusEPlugin {

//---------------------------------------------------------
//   scanLrdfDir
//---------------------------------------------------------

void scanLrdfDir(const QString& dirname,
                 QStringList* rdfs,
                 bool debugStdErr,
                 int recurseLevel)
{
  const int max_levels = 10;
  if(recurseLevel >= max_levels)
  {
    std::fprintf(stderr, "scanLrdfDir: Ignoring too-deep directory level (max:%d) at:%s\n",
                 max_levels, dirname.toLocal8Bit().constData());
    return;
  }
      
  DEBUG_PLUGIN_RDF(stderr, "scan lrdf dir <%s>\n", dirname.toLatin1().constData());
  
  QDir pluginDir;
  pluginDir.setPath(dirname);
  pluginDir.setFilter(QDir::Drives | QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
  pluginDir.setSorting(QDir::Name | QDir::IgnoreCase);
  if(pluginDir.exists())
  {
    pluginDir.setNameFilters(QStringList() << "*.rdfs" << "*.rdf");
  
    QFileInfoList fi_list = pluginDir.entryInfoList();
    QFileInfoList::iterator it = fi_list.begin();
    while(it != fi_list.end())
    {
      const QFileInfo& fi = *it;
      if(fi.isDir())
        // RECURSIVE!
        scanLrdfDir(fi.filePath(), rdfs, debugStdErr, recurseLevel + 1);
      else
      {
        const QByteArray fi_fp = fi.filePath().toLocal8Bit();
        const QString fi_bn = fi.completeBaseName();
        const int rdfs_sz = rdfs->size();
        int i = 0;
        for( ; i < rdfs_sz; ++i)
        {
          const QFileInfo s_fi = (rdfs->at(i));
          if(s_fi.completeBaseName() == fi_bn)
          {
            if(debugStdErr)
              std::fprintf(stderr, "Scanning LRDF directory: Ignoring RDF file: %s duplicate file of: %s\n",
                           fi_fp.constData(), s_fi.filePath().toLocal8Bit().constData());
            break;
          }
        }
        // Not found.
        if(i >= rdfs_sz)
          rdfs->append(fi.filePath());
      }
      
      ++it;
    }
  }
}

//---------------------------------------------------------
//   scanLrdfPlugins
//---------------------------------------------------------

void scanLrdfPlugins(QStringList* rdfs, bool debugStdErr)
{
  QString lrdfPath = qEnvironmentVariable("LRDF_PATH");
  if(lrdfPath.isEmpty())
  {
    QString share_rdf_dir(SHAREDIR);
    if(!share_rdf_dir.isEmpty())
      share_rdf_dir += "/rdf:";
    QString homePath = qEnvironmentVariable("HOME");
    if(!homePath.isEmpty())
      homePath += QString("/lrdf:");
    
    lrdfPath =
      // Our own rdf files (and fixes) take priority.
      share_rdf_dir +
      // Then the usual home place.
      homePath +
      // Then the system.
      QString("/usr/local/share/ladspa/rdf:/usr/share/ladspa/rdf");
  }
  if(!lrdfPath.isEmpty())
  {
    QStringList sl = lrdfPath.split(":", QString::SkipEmptyParts, Qt::CaseSensitive);
    for(QStringList::const_iterator it = sl.cbegin(); it != sl.cend(); ++it)
      scanLrdfDir(*it, rdfs, debugStdErr);
  }
}


} // namespace MusEPlugin
