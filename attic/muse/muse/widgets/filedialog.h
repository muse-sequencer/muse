//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: filedialog.h,v 1.2.2.2 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <qfiledialog.h>

class FileDialogButtons;
class QStringList;

//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

class MFileDialog : public QFileDialog {
      Q_OBJECT

      enum ViewType { GLOBAL_VIEW, PROJECT_VIEW, USER_VIEW }; //!< The three different viewtypes
      static ViewType lastViewUsed;
      static QString  lastUserDir, lastGlobalDir;
      bool showButtons;
      QString baseDir;

   private slots:
      void globalToggled(bool);
      void userToggled(bool);
      void projectToggled(bool);
      void directoryChanged(const QString& directory);

   public:
      FileDialogButtons* buttons;
      MFileDialog(const QString& dir, const QString& filter = QString::null,
         QWidget* parent = 0, bool writeFlag = false);
      };

//---------------------------------------------------------
//   ContentsPreview
//---------------------------------------------------------

class ContentsPreview : public QWidget, public QFilePreview {
      Q_OBJECT

      virtual void previewUrl(const QUrl &url);
      QString path;
      QPixmap* bg;

   public:
      ContentsPreview(QWidget* parent, const char* name=0)
         : QWidget(parent, name) {
            bg = 0;
            }
      ~ContentsPreview();
      };

//QString getSaveFileName(const QString& startWidth, const char** filter,
QString getSaveFileName(const QString& startWidth, const QStringList& filters,
         QWidget* parent, const QString& name);
//QString getOpenFileName(const QString& startWidth, const char** filter,
QString getOpenFileName(const QString& startWidth, const QStringList& filters,
         QWidget* parent, const QString& name, bool* openAll);
//QString getImageFileName(const QString& startWith, const char** filters, 
QString getImageFileName(const QString& startWith, const QStringList& filters, 
         QWidget* parent, const QString& name);

FILE* fileOpen(QWidget*, QString, const QString&,
   const char*, bool&, bool = false, bool = false);


//---------------------------------------------------------
//   MFile
//    "Muse" File
//---------------------------------------------------------

class MFile {
      bool isPopen;
      FILE* f;
      QString path;
      QString ext;

   public:
      MFile(const QString& path, const QString& ext);
      ~MFile();
      //FILE* open(const char* mode, const char** pattern,
      FILE* open(const char* mode, const QStringList& pattern,
         QWidget* parent, bool noError,
         bool warnIfOverwrite, const QString& caption);
      };

