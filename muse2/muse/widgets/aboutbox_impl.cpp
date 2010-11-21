#include "aboutbox_impl.h"
#include "config.h"
#include "icons.h"

AboutBoxImpl::AboutBoxImpl()
{
  setupUi(this);
  imageLabel->setPixmap(*aboutMuseImage);
  QString version(VERSION);
  QString svnrevision(SVNVERSION);
  versionLabel->setText("Version: " + version + " (svn  revision: "+ svnrevision +")");
}
