//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: filedialog.h,v 1.8 2006/01/25 16:24:33 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

class FileDialogButtons;

//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

class MFileDialog : public QFileDialog {
      bool showButtons;
      QString baseDir;
      Q_OBJECT

   public:
      MFileDialog(const QString& dir, const QString& filter = QString::null,
         QWidget* parent = 0, bool writeFlag = false);
      };

QString getSaveFileName(const QString& startWidth, const QStringList& filter,
         QWidget* parent, const QString& name);
QString getOpenFileName(const QString& startWidth, const QStringList& filter,
         QWidget* parent, const QString& name);
QString getImageFileName(const QString& startWith, const QStringList& filter,
         QWidget* parent, const QString& name);

QFile* fileOpen(QWidget*, QString, const QString&,
   QIODevice::OpenMode, bool = false);


//---------------------------------------------------------
//   MFile
//    "Muse" File
//---------------------------------------------------------

class MFile {
      QFile* f;
      QString path;
      QString ext;

   public:
      MFile(const QString& path, const QString& ext);
      ~MFile();
      QFile* open(QIODevice::OpenMode, const QStringList& pattern,
         QWidget* parent,
         bool warnIfOverwrite, const QString& caption);
      };

