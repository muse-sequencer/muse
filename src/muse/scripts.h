#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QObject>
#include <QIODevice>
#include <QString>


class QMenu;

namespace MusECore {

class PartList;

class Scripts : public QObject
{
    Q_OBJECT


    QStringList deliveredScriptNames;
    QStringList userScriptNames;

    void writeStringToFile(QIODevice *dev, const QString &writeString);

    void receiveExecDeliveredScript(int id);
    void receiveExecUserScript(int id);

signals:
    void execDeliveredScriptReceived(int);
    void execUserScriptReceived(int);

public:
    explicit Scripts(QObject *parent = nullptr);


    void populateScriptMenu(QMenu* menuScripts);
    void executeScript(QWidget *parent, const QString &scriptfile, PartList* parts, int quant, bool onlyIfSelected);
    QString getScriptPath(int id, bool delivered);
};

}
#endif // SCRIPTS_H
