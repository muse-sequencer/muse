//=========================================================
//  MusE
//  Linux Music Editor
//
//  file.h
//  (C) Copyright 2025 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __FILE_H__
#define __FILE_H__

#include <QObject>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QIODevice>

namespace MusEFile {

class File : public QObject
{
    Q_OBJECT

  public:
    enum ZipType { UnZippedType = 0, GZipType, BZip2Type };
    enum ErrorCode
    { NoError = 0, GeneralError, PermissionsError, ReadError, WriteError, TimeOutError, AbortError,
      CorruptCompressError, WarningError, InternalError, UnknownError };

  private:
    QFileInfo _fi;
    QString _name;
    QString _ext;
    QFile _qfile;
    QProcess _qproc;
    ZipType _zipType;

    void init();
    QString zipName(ZipType type) const;

  public:
    File();
    File(const QString &name, const QString &ext, QObject *parent = nullptr);

    void setFile(const QString &name, const QString &ext);
    QString filePath() const;
    bool exists() const;
    bool isCompressed() const;
    ZipType zipType() const;
    bool open(QIODevice::OpenMode mode);
    void reset();
    bool close();
    QIODevice *iodevice();
    ErrorCode error() const;
    QString errorString() const;
};

} // namespace MusEFile

#endif
