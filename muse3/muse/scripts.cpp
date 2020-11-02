#include <stdio.h>
#include <iostream>

#include "scripts.h"
#include "part.h"
#include "song.h"
#include "debug.h"
#include "undo.h"
#include "icons.h"

#include <QProgressDialog>
#include <QMessageBox>
#include <QProcess>
#include <QMenu>
#include <QDir>
#include <QTextStream>


namespace MusECore {


Scripts::Scripts(QObject *parent) : QObject(parent) {}

//---------------------------------------------------------
//   executeScript
//---------------------------------------------------------
void Scripts::executeScript(QWidget *parent, const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected)
{
    // a simple format for external processing
    // will be extended if there is a need
    //
    // Semantics:
    // TIMESIG <n> <z>
    // PARTLEN <len in ticks>
    // BEATLEN <len in ticks>
    // QUANTLEN <len in ticks>
    // NOTE <tick> <nr> <len in ticks> <velocity>
    // CONTROLLER <tick> <a> <b> <c>
    //

    if (onlyIfSelected) // if this is set means we are probably inside a midi editor and we ask again to be sure
    {
        if(QMessageBox::question(parent, QString("Process events"),
                                 tr("Do you want to process ALL or only selected events?"), tr("&Selected"), tr("&All"),
                                 QString(), 0, 1 ) == 1)
        {
            onlyIfSelected = false;
        }
    }
    QProgressDialog progress(parent);
    progress.setLabelText("Process parts");
    progress.setRange(0,parts->size());
    progress.setValue(0);
    progress.setCancelButton(nullptr);
    MusEGlobal::song->startUndo(); // undo this entire block
    for (const auto& i : *parts) {
        //const char* tmp = tmpnam(NULL);
        char tmp[16] = "muse-tmp-XXXXXX";
        char tempStr[200];
        int fd = mkstemp(tmp);
        if (MusEGlobal::debugMsg)
            fprintf(stderr, "executeScript: script input filename=%s\n",tmp);

        FILE *fp = fdopen(fd , "w");
        MidiPart *part = (MidiPart*)(i.second);
        if (MusEGlobal::debugMsg)
            fprintf(stderr, "SENDING TO SCRIPT, part start: %d\n", part->tick());

        int z, n;
        MusEGlobal::sigmap.timesig(part->tick(), z, n);
        sprintf(tempStr, "TIMESIG %d %d\n", z, n);
        writeStringToFile(fp,tempStr);
        sprintf(tempStr, "PART %d %d\n", part->tick(), part->lenTick());
        writeStringToFile(fp,tempStr);
        sprintf(tempStr, "BEATLEN %d\n", MusEGlobal::sigmap.ticksBeat(part->tick()));
        writeStringToFile(fp,tempStr);
        sprintf(tempStr, "QUANTLEN %d\n", quant);
        writeStringToFile(fp,tempStr);

        if (MusEGlobal::debugMsg)
            std::cout << "Events in part " << part->events().size() << std::endl;

        EventList elist = part->events();
        for (const auto& e : elist)
        {
            Event ev = e.second;

            if (ev.isNote())
            {
                if (onlyIfSelected && ev.selected() == false)
                    continue;

                sprintf(tempStr,"NOTE %d %d %d %d\n", ev.tick(), ev.dataA(),  ev.lenTick(), ev.dataB());
                writeStringToFile(fp,tempStr);

                // Operation is undoable but do not start/end undo.
                // Indicate do not do port controller values and clone parts.
                MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteEvent,
                                                        ev, part, false, false), Song::OperationUndoable);

            } else if (ev.type()==Controller) {
                sprintf(tempStr,"CONTROLLER %d %d %d %d\n", ev.tick(), ev.dataA(), ev.dataB(), ev.dataC());
                writeStringToFile(fp,tempStr);
                // Operation is undoable but do not start/end undo.
                // Indicate do not do port controller values and clone parts.
                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                                                                  ev, part, false, false), Song::OperationUndoable);
            }
        }
        fclose(fp);

        QStringList arguments;
        arguments << tmp;

        QProcess *myProcess = new QProcess(parent);
        myProcess->start(scriptfile, arguments);
        myProcess->waitForFinished();
        QByteArray errStr = myProcess->readAllStandardError();

        if (myProcess->exitCode()) {
            QMessageBox::warning(parent, tr("MusE - external script failed"),
                                 tr("MusE was unable to launch the script, error message:\n%1").arg(QString(errStr)));
            MusEGlobal::song->endUndo(SC_EVENT_REMOVED);
            return;
        }
        if (errStr.size()> 0) {
            fprintf(stderr, "script execution produced the following error:\n%s\n", QString(errStr).toLatin1().data());
        }
        QFile file(tmp);
        if (MusEGlobal::debugMsg)
            file.copy(file.fileName() + "_input");

        if ( file.open( QIODevice::ReadOnly ) )
        {
            QTextStream stream( &file );
            QString line;
            if (MusEGlobal::debugMsg)
                fprintf(stderr, "RECEIVED FROM SCRIPT:\n");
            while ( !stream.atEnd() )
            {
                line = stream.readLine(); // line of text excluding '\n'
                if (MusEGlobal::debugMsg) {
                    std::cout << line.toStdString() << std::endl;
                }

                if (line.startsWith("NOTE"))
                {
                    QStringList sl = line.split(" ");

                    Event e(Note);
                    int tick = sl[1].toInt();
                    int pitch = sl[2].toInt();
                    int len = sl[3].toInt();
                    int velo = sl[4].toInt();
                    fprintf(stderr, "extracted %d %d %d %d\n", tick, pitch, len, velo);
                    e.setTick(tick);
                    e.setPitch(pitch);
                    e.setVelo(velo);
                    e.setLenTick(len);
                    // Operation is undoable but do not start/end undo.
                    // Indicate do not do port controller values and clone parts.
                    MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent,
                                                            e, part, false, false), Song::OperationUndoable);
                }
                if (line.startsWith("CONTROLLER"))
                {
                    QStringList sl = line.split(" ");

                    Event e(Controller);
                    int a = sl[2].toInt();
                    int b = sl[3].toInt();
                    int c = sl[4].toInt();
                    e.setA(a);
                    e.setB(b);
                    e.setB(c);
                    // Operation is undoable but do not start/end undo.
                    // Indicate do not do port controller values and clone parts.
                    MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent,
                                                            e, part, false, false), Song::OperationUndoable);
                }
            }
            file.close();
        }

        if (!MusEGlobal::debugMsg) // if we are writing debug info we also keep the script data
            remove(tmp);
        progress.setValue(progress.value()+1);
    } // for

    MusEGlobal::song->endUndo(SC_EVENT_REMOVED);
}


void Scripts::populateScriptMenu(QMenu* menuScripts)
{
    menuScripts->clear();

    // List scripts
    QString distScripts = MusEGlobal::museGlobalShare + "/scripts";
    QString userScripts = MusEGlobal::configPath + "/scripts";

    QFileInfo distScriptsFi(distScripts);
    if (distScriptsFi.isDir()) {
        QDir dir = QDir(distScripts);
        dir.setFilter(QDir::Executable | QDir::Files);
        deliveredScriptNames = dir.entryList();
    }
    QFileInfo userScriptsFi(userScripts);
    if (userScriptsFi.isDir()) {
        QDir dir(userScripts);
        dir.setFilter(QDir::Executable | QDir::Files);
        userScriptNames = dir.entryList();
    }

    int id = 0;
    if (deliveredScriptNames.size() > 0) {
        for (QStringList::Iterator it = deliveredScriptNames.begin(); it != deliveredScriptNames.end(); it++, id++) {
            QAction* act = menuScripts->addAction(*it);
            connect(act, &QAction::triggered, [this, id]() { receiveExecDeliveredScript(id); } );
        }
        menuScripts->addSeparator();
    }
    if (userScriptNames.size() > 0) {
        for (QStringList::Iterator it = userScriptNames.begin(); it != userScriptNames.end(); it++, id++) {
            QAction* act = menuScripts->addAction(*it);
            connect(act, &QAction::triggered, [this, id]() { receiveExecUserScript(id); } );
        }
        menuScripts->addSeparator();
    }

    QAction* refreshScriptsAction = menuScripts->addAction(tr("Re-read script names from disc"));
    refreshScriptsAction->setIcon(*MusEGui::fileopenSVGIcon);
    connect(refreshScriptsAction, &QAction::triggered, [this, menuScripts]() { populateScriptMenu(menuScripts); } );

}

//---------------------------------------------------------
//   getScriptPath
//---------------------------------------------------------
QString Scripts::getScriptPath(int id, bool isdelivered)
{
    if (isdelivered) {
        QString path = MusEGlobal::museGlobalShare + "/scripts/" + deliveredScriptNames[id];
        return path;
    }

    QString path = MusEGlobal::configPath + "/scripts/" + userScriptNames[id - deliveredScriptNames.size()];
    return path;
}

void Scripts::writeStringToFile(FILE *filePointer, const char *writeString)
{
    if (MusEGlobal::debugMsg)
        std::cout << writeString;
    fputs(writeString, filePointer);
}

void Scripts::receiveExecDeliveredScript(int id)
{
    execDeliveredScriptReceived(id);
}

void Scripts::receiveExecUserScript(int id)
{
    execUserScriptReceived(id);
}


}
