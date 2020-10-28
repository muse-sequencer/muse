#ifndef SYNCTOOLBAR_H
#define SYNCTOOLBAR_H

#include <QToolBar>
#include <QToolButton>

#include "type_defs.h"


namespace MusEGui {

class SyncToolbar : public QToolBar
{
    Q_OBJECT

    QAction* syncAction;
    QAction* jackTransportAction;
    QAction* timebaseMasterAction;
    QToolButton* timebaseMasterButton;
    QTimer* blinkTimer;
    QString buttonDefColor;
    QString buttonCheckedColor;
    bool blinkState;

private slots:
    void extSyncClicked(bool v);
    void useJackTransportClicked(bool v);
    void timebaseMasterClicked(bool v);
    void songChanged(MusECore::SongChangedStruct_t);
    void timebaseBlink();

public:
    SyncToolbar(const QString& title, QWidget* parent = nullptr);

};

} // namespace

#endif // SYNCTOOLBAR_H
