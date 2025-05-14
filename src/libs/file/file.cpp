//=========================================================
//  MusE
//  Linux Music Editor
//
//  file.cpp
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

#include <QStringList>

#include "file.h"

namespace MusEFile {

File::File()
  : _zipType(UnZippedType)
{
}

File::File(const QString &name, const QString &ext, QObject *parent)
  : QObject(parent), _fi(name), _name(name), _ext(ext), _zipType(UnZippedType)
{
  init();
}

void File::setFile(const QString &name, const QString &ext)
{
  _name = name;
  _fi.setFile(_name);
  _ext = ext;
  init();
}

QString File::filePath() const { return _fi.filePath(); }

void File::init()
{
  if(_fi.completeSuffix() == "")
  {
    _fi.setFile(_name + _ext);
    _zipType = UnZippedType;
  }
  else if(_fi.suffix() == "gz")
  {
    _zipType = GZipType;
  }
  else if(_fi.suffix() == "bz2")
  {
    _zipType = BZip2Type;
  }
}

QString File::zipName(ZipType type) const
{
  switch(type)
  {
    case UnZippedType:
      return QString();
    break;

    case GZipType:
      return "gzip";
    break;

    case BZip2Type:
      return "bzip2";
    break;
  }
  return QString();
}

bool File::exists() const { return _fi.exists(); }

bool File::isCompressed() const { return zipType() != UnZippedType; }

File::ZipType File::zipType() const { return _zipType; }

bool File::open(QIODevice::OpenMode mode)
{
  bool isErr = false;
  QString errtxt;
  if (isCompressed())
  {
    _qproc.setParent(this);
    QStringList args;

    if (mode == QIODevice::ReadOnly)
      args << "-d";
    args << "-c" << _fi.filePath();

    _qproc.start(zipName(zipType()), args, mode);

    // TESTING: Hm. Program must finish before we can read? What if the file is very large?
    // All that data goes into the stdout before we can actually read it? Worried about buffer limits.
    // QProcess seems to use internal dynamic (growable) buffers called QRingBuffer. Good.
    // But are there any stdout buffers as well?
    // Some solutions say to use unbuffered stdout if possible.
    // This involves the setbuf and setvbuf system commands.
    // And QIODevice has a flag:
    //   "QIODevice::Unbuffered  0x0020  Any buffer in the device is bypassed."
    // Wondering if that flag affects stdout or just the internal buffers.

    if(!_qproc.waitForFinished(5000)) // 5 secs.
    {
      isErr = true;
      errtxt = _qproc.errorString();
    }
  }
  else
  {
    _qfile.setParent(this);
    _qfile.setFileName(_fi.filePath());
    if(!_qfile.open(mode))
    {
      isErr = true;
      errtxt = _qfile.errorString();
    }
  }
  return !isErr;
}

void File::reset()
{
  if(!isCompressed())
    _qfile.reset();
}

bool File::close()
{
  if(isCompressed())
  {
    _qproc.close();
    return true;
  }

  _qfile.close();
  return true;
}

QIODevice *File::iodevice()
{
  if(isCompressed())
    return &_qproc;
  return &_qfile;
}

File::ErrorCode File::error() const
{
  if(isCompressed())
  {
    if(_qproc.exitStatus() == QProcess::NormalExit)
    {
      switch(_qproc.exitCode())
      {
        case 0:
          // Continue on to any QProcess errors below.
        break;

        case 1:
          return GeneralError;
        break;

        case 2:
          switch (zipType())
          {
            case UnZippedType:
              return GeneralError;
            break;

            case GZipType:
              return WarningError;
            break;

            case BZip2Type:
              return CorruptCompressError;
            break;
          }
        break;

        case 3:
          switch (zipType())
          {
            case UnZippedType:
              return GeneralError;
            break;

            case GZipType:
              return GeneralError;
            break;

            case BZip2Type:
              return InternalError;
            break;
          }
        break;
      }
    }

    switch(_qproc.error())
    {
      case QProcess::FailedToStart:
        return GeneralError;
      break;

      case QProcess::Crashed:
        return GeneralError;
      break;

      case QProcess::Timedout:
        return TimeOutError;
      break;

      case QProcess::WriteError:
        return WriteError;
      break;

      case QProcess::ReadError:
        return ReadError;
      break;

      case QProcess::UnknownError:
        // REMOVE Tim. tmp. FIXME: TODO: Check meaning of this.
        // QProcess help: "An unknown error occurred. This is the default return value of error()."
        // Tested: It seems to mean no error. Thus we shouldn't treat it as an error ???
        //return UnknownError;
      break;
    }
  }
  else
  {
    switch(_qfile.error())
    {
      case QFileDevice::NoError:
        // Continue on...
      break;

      case QFileDevice::ReadError:
        return ReadError;
      break;

      case QFileDevice::WriteError:
        return WriteError;
      break;

      case QFileDevice::FatalError:
        return GeneralError;
      break;

      case QFileDevice::ResourceError:
        return GeneralError;
      break;

      case QFileDevice::OpenError:
        return GeneralError;
      break;

      case QFileDevice::AbortError:
        return AbortError;
      break;

      case QFileDevice::TimeOutError:
        return TimeOutError;
      break;

      case QFileDevice::UnspecifiedError:
        return UnknownError;
      break;

      case QFileDevice::RemoveError:
        return GeneralError;
      break;

      case QFileDevice::RenameError:
        return GeneralError;
      break;

      case QFileDevice::PositionError:
        return GeneralError;
      break;

      case QFileDevice::ResizeError:
        return GeneralError;
      break;

      case QFileDevice::PermissionsError:
        return PermissionsError;
      break;

      case QFileDevice::CopyError:
        return GeneralError;
      break;
    }
  }

  return NoError;
}

QString File::errorString() const
{
  if(isCompressed())
    return _qproc.errorString();
  return _qfile.errorString();
}

} // namespace MusEFile

