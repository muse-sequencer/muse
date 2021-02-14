#include "savenewrevisiondialog.h"
#include "ui_savenewrevisiondialog.h"
#include "helper.h"

namespace MusEGui {

SaveNewRevisionDialog::SaveNewRevisionDialog(QWidget *parent, QFileInfo projectFileInfo) :
  QDialog(parent),
  ui(new Ui::SaveNewRevisionDialog)
{
  ui->setupUi(this);
  _projectFileInfo = projectFileInfo;
}

SaveNewRevisionDialog::~SaveNewRevisionDialog()
{
  delete ui;
}
QString SaveNewRevisionDialog::buildFilePath(QString newBaseName)
{
  QString path = _projectFileInfo.path();
  QString suffix = _projectFileInfo.completeSuffix();

  return path + "/" + newBaseName + "." + suffix;
}

void SaveNewRevisionDialog::accept()
{
  QString newFilePath = buildFilePath(ui->projectNameEdit->text());
  QFileInfo newFileInfo;
  newFileInfo.setFile( newFilePath );

  if (newFileInfo.exists()) {
    QString fileName = newFileInfo.filePath();
    ui->errorInfo->setText(QString("%1 already exists!\n").arg(fileName));
    return;
  }

  QDialog::accept();
}

void SaveNewRevisionDialog::reject()
{
  QDialog::reject();
}

QString SaveNewRevisionDialog::getNewRevision()
{
  QString baseName = _projectFileInfo.baseName();
  QRegExp xRegExp("_\\d\\d\\d$");
  int index = xRegExp.indexIn(baseName);

  if(index > 0)
  {
    QString newNumberString = baseName.mid(index+1, baseName.size() - index);
    int newNumber = newNumberString.toInt();
    newNumber++;
    QString newBaseName = baseName.mid(0, index) + QString("_%1").arg(newNumber,3, 10, QChar('0'));

    return buildFilePath(newBaseName);
  }
  return "";
}

QString SaveNewRevisionDialog::getNewRevisionWithDialog()
{
  ui->oldPath->setText(QString("%1\n").arg(_projectFileInfo.filePath()));
  ui->errorInfo->clear();

  QString projectFileName = MusEGui::projectTitleFromFilename(_projectFileInfo.baseName()) + "_001";
  ui->projectNameEdit->setText(projectFileName);
  ui->projectNameEdit->setFocus();
  show();
  ui->projectNameEdit->setCursorPosition(ui->projectNameEdit->text().size());
  auto code = exec();
  if (code == QDialog::Rejected)
    return "";

  QString newFilePath = buildFilePath(ui->projectNameEdit->text());

  return newFilePath;
}



} //namespace MusEGui
