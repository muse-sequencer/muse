#ifndef RECTOOLBAR_H
#define RECTOOLBAR_H

#include <QToolBar>

namespace MusEGui {

class RecToolbar : public QToolBar
{
    Q_OBJECT

public:
    RecToolbar(const QString& title, QWidget* parent = nullptr);

private slots:
    void setRecMode(int);
    void setCycleMode(int);
};

} // namespace

#endif // RECTOOLBAR_H
