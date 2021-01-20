#ifndef SAVENEWREVISIONDIALOG_H
#define SAVENEWREVISIONDIALOG_H

#include <QDialog>
#include <QFileInfo>

namespace Ui {
class SaveNewRevisionDialog;
}

namespace MusEGui {



class SaveNewRevisionDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SaveNewRevisionDialog(QWidget *parent, QFileInfo projectFileInfo);
  ~SaveNewRevisionDialog();

  QString getNewRevision();

private slots:
  void accept() override;
  void reject() override;

private:
  QString buildFilePath(QString newName);

  Ui::SaveNewRevisionDialog *ui;
  QFileInfo _projectFileInfo;
};

} // namespace MusEGui

#endif // SAVENEWREVISIONDIALOG_H
