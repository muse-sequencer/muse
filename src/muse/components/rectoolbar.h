#ifndef RECTOOLBAR_H
#define RECTOOLBAR_H

#include <QToolBar>

class QComboBox;

namespace MusEGui {

class RecToolbar : public QToolBar
{
    Q_OBJECT

    QComboBox *recMode;
    QComboBox *cycleMode;

public:
    RecToolbar(const QString& title, QWidget* parent = nullptr);

public slots:
    void setRecMode(int);
    void setCycleMode(int);
};

} // namespace

#endif // RECTOOLBAR_H
