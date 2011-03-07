#include <qlabel.h>
#include "aboutbox_impl.h"
#include "config.h"

AboutBoxImpl::AboutBoxImpl()
{
  QString version(VERSION);
  QString svnrevision(SVNVERSION);
  versionLabel->setText("Version: " + version + " (svn  revision: "+ svnrevision +")");
}
