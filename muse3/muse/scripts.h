#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QObject>

#include "script_delivery.h"

class QMenu;

namespace MusECore {

class PartList;

class Scripts : public QObject
{
    Q_OBJECT

    QStringList deliveredScriptNames;
    QStringList userScriptNames;

    void refreshScriptsTriggered(QMenu* menuScripts, ScriptReceiver* receiver);
    void writeStringToFile(FILE *filePointer, const char *writeString);

public:
    explicit Scripts(QObject *parent = nullptr);

    void populateScriptMenu(QMenu* menuScripts, ScriptReceiver* receiver);
    void executeScript(QWidget *parent, const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected);
    QString getScriptPath(int id, bool delivered);
};

}
#endif // SCRIPTS_H
