#ifndef UNUSEDWAVEFILES_H
#define UNUSEDWAVEFILES_H

#include <QDialog>

namespace Ui {
    class UnusedWaveFiles;
}

class UnusedWaveFiles : public QDialog
{
    Q_OBJECT
    QStringList allWaveFiles;
public:
    explicit UnusedWaveFiles(QWidget *parent = 0);
    ~UnusedWaveFiles();

public slots:
    void accept();
    void findWaveFiles();
private:
    Ui::UnusedWaveFiles *ui;
};

#endif // UNUSEDWAVEFILES_H
