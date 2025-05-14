#include <iostream>

#include "scripts.h"
#include "track.h"
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
#include <QTemporaryFile>


namespace MusECore {


Scripts::Scripts(QObject *parent) : QObject(parent) {}

//---------------------------------------------------------
//   executeScript
//---------------------------------------------------------

void Scripts::executeScript(QWidget *parent, const QString &scriptfile, PartList* parts, int quant, bool onlyIfSelected)
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
        QMessageBox m;
        m.setText(tr("Do you want to process all or only selected events?"));
        m.addButton(tr("&Selected"), QMessageBox::YesRole);
        m.addButton(tr("&All"), QMessageBox::NoRole);
        m.addButton(tr("&Cancel"), QMessageBox::RejectRole);
        m.exec();
        if (m.buttonRole(m.clickedButton()) == QMessageBox::RejectRole)
            return;
        else if (m.buttonRole(m.clickedButton()) == QMessageBox::NoRole)
            onlyIfSelected = false;
    }

    QProgressDialog progress(parent);
    progress.setLabelText("Process parts");
    progress.setRange(0,parts->size());
    progress.setValue(0);
    progress.setCancelButton(nullptr);

    Undo ops;

    for (const auto& i : *parts)
    {
        QTemporaryFile tmpfile;
        if(!tmpfile.open())
          continue;

        const QString tmpfname = tmpfile.fileName();
        if (MusEGlobal::debugMsg)
            std::cerr << QString("executeScript: script input filename=%1").arg(tmpfname).toStdString() << std::endl;

        MidiPart *part = (MidiPart*)(i.second);
        if (MusEGlobal::debugMsg)
            std::cerr << "SENDING TO SCRIPT, part start: " << part->tick() << std::endl;

        int z, n;
        MusEGlobal::sigmap.timesig(part->tick(), z, n);
        writeStringToFile(&tmpfile, QString("TIMESIG %1 %2\n").arg(z).arg(n));

        writeStringToFile(&tmpfile, QString("PART %1 %2\n").arg(part->tick()).arg(part->lenTick()));

        writeStringToFile(&tmpfile, QString("BEATLEN %1\n").arg(MusEGlobal::sigmap.ticksBeat(part->tick())));

        writeStringToFile(&tmpfile, QString("QUANTLEN %1\n").arg(quant));

        MidiTrack *track = part->track();
        if (track->type() == Track::MIDI)
        {
          writeStringToFile(&tmpfile, QString("TYPE MIDI\n"));
        } else if (track->type() == Track::DRUM) {
          writeStringToFile(&tmpfile, QString("TYPE DRUM\n"));
        }

        if (MusEGlobal::debugMsg)
            std::cerr << "Events in part " << part->events().size() << std::endl;

        EventList elist = part->events();
        for (const auto& e : elist)
        {
            Event ev = e.second;

            if (ev.isNote())
            {
                if (onlyIfSelected && ev.selected() == false)
                    continue;

                writeStringToFile(&tmpfile, QString("NOTE %1 %2 %3 %4\n")
                  .arg(ev.tick()).arg(ev.dataA()).arg(ev.lenTick()).arg(ev.dataB()));

                // Indicate do not do port controller values and clone parts.
                ops.push_back(UndoOp(UndoOp::DeleteEvent, ev, part, false, false));

            } else if (ev.type()==Controller) {
                writeStringToFile(&tmpfile, QString("CONTROLLER %1 %2 %3 %4\n")
                  .arg(ev.tick()).arg(ev.dataA()).arg(ev.dataB()).arg(ev.dataC()));
                // Indicate do not do port controller values and clone parts.
                ops.push_back(UndoOp(UndoOp::DeleteEvent, ev, part, false, false));
            }
        }
        tmpfile.close();

        QStringList arguments;
        arguments << tmpfname;

        QProcess *myProcess = new QProcess(parent);
        myProcess->start(scriptfile, arguments);
        myProcess->waitForFinished();
        QByteArray errStr = myProcess->readAllStandardError();

        if (myProcess->exitCode()) {
            QMessageBox::warning(parent, tr("MusE - external script failed"),
                                 tr("MusE was unable to launch the script, error message:") + (QString(errStr)));
            return;
        }
        if (errStr.size()> 0) {
            std::cerr << "script execution produced the following error:"
              << std::endl << QString(errStr).toStdString() << std::endl;
        }
        QFile file(tmpfname);
        if (MusEGlobal::debugMsg)
            file.copy(file.fileName() + "_input");

        if ( file.open( QIODevice::ReadOnly ) )
        {
            QTextStream stream( &file );
            QString line;
            if (MusEGlobal::debugMsg)
                std::cerr << "RECEIVED FROM SCRIPT:" << std::endl;
            while ( !stream.atEnd() )
            {
                line = stream.readLine(); // line of text excluding '\n'
                if (MusEGlobal::debugMsg) {
                    std::cerr << line.toStdString() << std::endl;
                }

                if (line.startsWith("NOTE"))
                {
                    QStringList sl = line.split(" ");

                    Event e(Note);
                    int tick = sl[1].toInt();
                    int pitch = sl[2].toInt();
                    int len = sl[3].toInt();
                    int velo = sl[4].toInt();
                    std::cerr << QString("extracted %1 %2 %3 %4")
                      .arg(tick).arg(pitch).arg(len).arg(velo).toStdString() << std::endl;
                    e.setTick(tick);
                    e.setPitch(pitch);
                    e.setVelo(velo);
                    e.setLenTick(len);
                    // Indicate do not do port controller values and clone parts.
                    ops.push_back(UndoOp(UndoOp::AddEvent, e, part, false, false));
                }
                if (line.startsWith("CONTROLLER"))
                {
                    QStringList sl = line.split(" ");

                    Event e(Controller);
                    int tick = sl[1].toInt();
                    int a = sl[2].toInt();
                    int b = sl[3].toInt();
                    int c = sl[4].toInt();
                    e.setTick(tick);
                    e.setA(a);
                    e.setB(b);
                    e.setC(c);
                    // Indicate do not do port controller values and clone parts.
                    ops.push_back(UndoOp(UndoOp::AddEvent, e, part, false, false));
                }
            }
            file.close();
        }

        // This seems to already be handled in the 'file.copy(file.fileName() + "_input")' line above. Tim.
        // if (!MusEGlobal::debugMsg) // if we are writing debug info we also keep the script data
        //     tmpfile.setAutoRemove(false);

        progress.setValue(progress.value()+1);
    } // for

    // Operation is undoable but do not start/end undo.
    MusEGlobal::song->applyOperationGroup(ops);
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

    QAction* refreshScriptsAction = menuScripts->addAction(tr("Reload Script Names from Disc"));
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

void Scripts::writeStringToFile(QIODevice *dev, const QString &writeString)
{
    if (MusEGlobal::debugMsg)
        std::cerr << writeString.toLocal8Bit().constData();
    dev->write(writeString.toUtf8());
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
