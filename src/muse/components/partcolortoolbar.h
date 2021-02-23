#ifndef PARTCOLORTOOLBAR_H
#define PARTCOLORTOOLBAR_H

#include <QToolBar>

namespace MusEGui {

class PartColorToolbar : public QToolBar
{
    Q_OBJECT

    QAction* buttonAction;
    QMenu* colorPopup;

    void buildMenu();
    void popupActionTriggered(QAction *buttonAction);

public:
    PartColorToolbar(QWidget* parent);

signals:
    void partColorTriggered(int);
    void partColorIndexChanged(int);

public slots:
    void configChanged();
    void setCurrentIndex(int idx);
};

} // namespace MusEGui

#endif // PARTCOLORTOOLBAR_H
