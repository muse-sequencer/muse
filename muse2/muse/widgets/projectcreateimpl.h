#ifndef PROJECTCREATEIMPL_H
#define PROJECTCREATEIMPL_H

#include <QDialog>
#include "ui_projectcreate.h"

class ProjectCreateImpl : public QDialog, Ui::ProjectCreate
{
    Q_OBJECT

    QString directoryPath;
public:
    explicit ProjectCreateImpl(QWidget *parent = 0);
    QString getProjectPath();
    QString getSongInfo();

signals:

public slots:
    void updateDirectoryPath();
    void selectDirectory();
    void ok();

};

#endif // PROJECTCREATEIMPL_H
