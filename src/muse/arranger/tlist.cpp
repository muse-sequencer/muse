//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tlist.cpp,v 1.31.2.31 2009/12/15 03:39:58 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include "muse_math.h"

#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QIcon>
#include <QToolTip>
#include <QList>
#include <QColorDialog>

#include "popupmenu.h"
#include "globals.h"
#include "icons.h"
#include "tlist.h"
#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "midiseq.h"
#include "comment.h"
#include "track.h"
#include "song.h"
#include "header.h"
#include "audio.h"
#include "instruments/minstrument.h"
#include "app.h"
#include "helper.h"
#include "gconfig.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "synth.h"
#include "config.h"
#include "filedialog.h"
#include "menutitleitem.h"
#include "arranger.h"
#include "midi_audio_control.h"
#include "ctrl.h"
#include "plugin.h"
#include "operations.h"
#include "shortcuts.h"
#include "drumedit.h"
#include "utils.h"


#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

#ifdef LV2_SUPPORT
#include "lv2host.h"
#endif

// Forwards from header:
#include <QKeyEvent>
#include <QLineEdit>
#include <QSpinBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QMenu>
#include "xml.h"
#include "undo.h"
#include "header.h"
#include "popupmenu.h"
#include "scrollscale.h"

using MusECore::UndoOp;

namespace MusEGui {

static const int MIN_TRACKHEIGHT = 20;
//static const int WHEEL_DELTA = 120;
QColor collist[] = { Qt::red, Qt::yellow, Qt::blue , Qt::black, Qt::white, Qt::green };
QString colnames[] = { "Red", "Yellow", "Blue", "Black", "White", "Green"};
enum { AUTO_INVALID = -1, AUTO_SHOW_ALL = 251, AUTO_HIDE_ALL = 252, AUTO_CLEAR_AUTO = 253, AUTO_CLEAR_MIDI = 254, AUTO_MIDI_ASSIGN = 255 };

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

TList::TList(Header* hdr, QWidget* parent, const char* name)
    : QWidget(parent) // Qt::WNoAutoErase | Qt::WResizeNoErase are no longer needed according to Qt4 doc
{
    setBackgroundRole(QPalette::NoRole);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_StaticContents);
    // This is absolutely required for speed! Otherwise painfully slow because we get
    //  full rect paint events even on small scrolls! See help on QPainter::scroll().
    setAttribute(Qt::WA_OpaquePaintEvent);

    setStatusTip(tr("Track list: LMB to select track, CTRL+LMB to add to selection, SHIFT+LMB for range select. RMB in empty area to create tracks. Press F1 for help."));

    setObjectName(name);
    ypos = 0;
    editMode = false;
    editJustFinished=false;
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    header    = hdr;

    _scroll   = nullptr;
    editTrack = nullptr;
    editor    = nullptr;
    chan_edit = nullptr;
    ctrl_edit = nullptr;
    mode      = NORMAL;

    _sel3d = true;
    _curSelBorder = false;
    _curSelBorderColor = Qt::red;

    //setBackgroundMode(Qt::NoBackground); // ORCAN - FIXME. DELETETHIS?
    //setAttribute(Qt::WA_OpaquePaintEvent);
    resizeFlag = false;

    connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
    connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(redraw()));
    connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(maybeUpdateVolatileCustomColumns()));
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void TList::songChanged(MusECore::SongChangedStruct_t flags)
{
    if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_TRACK_REC_MONITOR
                 | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED
                 | SC_TRACK_MOVED
                 | SC_TRACK_SELECTION | SC_ROUTE | SC_CHANNELS
                 | SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED
                 | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED ))
        update();
    if (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED))
        adjustScrollbar();
}

//---------------------------------------------------------
//   drawCenteredPixmap
//    small helper function for "draw()" below
//---------------------------------------------------------

//static void drawCenteredPixmap(QPainter& p, const QPixmap* pm, const QRect& r)
//{
//    p.drawPixmap(r.x() + (r.width() - pm->width())/2, r.y() + (r.height() - pm->height())/2, *pm);
//}

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void TList::paintEvent(QPaintEvent* ev)
{
    paint(ev->rect());
}

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void TList::redraw()
{
    update();
}

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void TList::redraw(const QRect& r)
{
    update(r);
}


//---------------------------------------------------------
//   event
//---------------------------------------------------------

bool TList::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        MusECore::TrackList* l = MusEGlobal::song->tracks();
        int idx = 0;
        int yy  = -ypos;
        for (MusECore::iTrack i = l->begin(); i != l->end(); ++idx, yy += (*i)->height(), ++i) {
            MusECore::Track* track = *i;
            MusECore::Track::TrackType type = track->type();
            int trackHeight = track->height();
            if (trackHeight==0) // not visible
                continue;
            if (helpEvent->pos().y() > yy && helpEvent->pos().y() < yy + trackHeight) {
                if (type == MusECore::Track::AUDIO_SOFTSYNTH) {
                    MusECore::SynthI *s = (MusECore::SynthI*)track;
                    QToolTip::showText(helpEvent->globalPos(),track->name() + QString(" : ") +
                                       (s->synth() ? s->synth()->description() : QString(tr("SYNTH IS UNAVAILABLE!"))) +
                                       (s->synth() ? (s->synth()->uri().isEmpty() ? QString() :
                                                                                    QString(" \n") + s->synth()->uri()) :
                                                     (s->initConfig()._uri.isEmpty() ? QString() :
                                                                                       QString(" \n") + s->initConfig()._uri)));
                }
                else
                    QToolTip::showText(helpEvent->globalPos(),track->name());
            }
        }
        return true;
    }
    return QWidget::event(event);
}

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void TList::paint(const QRect& r)
{
    if (!isVisible())
        return;
    QRect rect(r);
    QPainter p(this);

    if (bgPixmap.isNull())
        p.fillRect(rect, MusEGlobal::config.trackBg);
    else
        p.drawTiledPixmap(rect, bgPixmap, QPoint(rect.x(), ypos + rect.y()));
    p.setClipRegion(rect);

    //printf("TList::paint hasClipping:%d\n", p.hasClipping());   // Tested true.

    int y  = rect.y();
    int w  = rect.width();
    int h  = rect.height();
    int x1 = rect.x();
    int x2 = rect.x() + w;

    QFont fit(font());
    fit.setItalic(true);

    QFont fbd(font());
    fbd.setBold(true);


    //---------------------------------------------------
    //    Tracks
    //---------------------------------------------------

    QColor mask_edge = QColor(90, 90, 90, 45);
    QColor mask_center = QColor(240, 240, 240, 175);
    QLinearGradient mask;
    mask.setColorAt(0, mask_edge);
    mask.setColorAt(0.15, mask_center);
    mask.setColorAt(0.3, mask_center);
    mask.setColorAt(0.85, mask_edge);
    mask.setColorAt(1, mask_edge);

    // Find up to two tracks that are soloed.
    MusECore::Track* solo_t_1 = nullptr;
    MusECore::Track* solo_t_2 = nullptr;
    {
        MusECore::TrackList* tl = MusEGlobal::song->tracks();
        for(MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it)
        {
            MusECore::Track* t = *it;
            if(t->internalSolo() || t->solo())
            {
                if(!solo_t_1)
                    solo_t_1 = t;
                else if(!solo_t_2)
                    solo_t_2 = t;
            }
            // Did we find at least two tracks? Done.
            if(solo_t_1 && solo_t_2)
                break;
        }
    }

    //      const int header_fh = header->fontMetrics().lineSpacing();
    //      const int svg_sz = qMin(header_fh + 2, qMax(MIN_TRACKHEIGHT - 5, 6)); // ???
    const int svg_sz = MIN_TRACKHEIGHT - 4; // 2px margin

    MusECore::TrackList* l = MusEGlobal::song->tracks();
    const MusECore::Track* cur_sel_track = l->currentSelection();
    int idx = 0;
    int yy  = -ypos;
    for (MusECore::iTrack i = l->begin(); i != l->end(); ++idx, yy += (*i)->height(), ++i) {
        MusECore::Track* track = *i;
        MusECore::Track::TrackType type = track->type();
        int trackHeight = track->height();
        if (trackHeight==0) // not visible
            continue;
        if (yy >= (y + h))
            break;
        if ((yy + trackHeight) < y)
            continue;
        //
        // clear one row
        //
        QColor bg;
        if (track->selected()) {
            if (track == cur_sel_track)
                bg = MusEGlobal::config.selectTrackCurBg;
            else
                bg = MusEGlobal::config.selectTrackBg;
            p.setPen(MusEGlobal::config.selectTrackFg);
        }
        else {
            bg = track->color();
            if (MusECore::isColorBright(bg))
                p.setPen(Qt::black);
            else
                p.setPen(Qt::white);
        }

        p.fillRect(x1, yy, w, trackHeight,
                   MusECore::getGradientFromColor(bg, QPoint(x1,yy), QPoint(x1, yy+trackHeight),
                                qBound(0, MusEGlobal::config.trackGradientStrength, 100)));

        if (track->selected() && _sel3d) {
            mask.setStart(QPointF(0, yy));
            mask.setFinalStop(QPointF(0, yy + trackHeight));
            p.fillRect(x1, yy, w, trackHeight, mask);
        }

        if (track == cur_sel_track && _curSelBorder) {
            p.save();
            p.setPen(_curSelBorderColor);
            p.setClipRect(x1 - 1, yy, w + 2, trackHeight);
            p.drawRect(0, yy + 1, header->length() - 2, trackHeight - 2);
            p.setClipping(false);
            p.restore();
        }

        int x = -1; // compensate for the buggy QHeaderView, would even have to be -2 for perfect alignment
        for (int index = 0; index < header->count(); ++index) {
            int section = header->logicalIndex(index);
            int w   = header->sectionSize(section);
            QRect r = p.transform().mapRect(QRect(x + 2, yy, w - 4, trackHeight));
            QRect svg_r = p.transform().mapRect(
                        QRect(x + w / 2 - svg_sz / 2,
                              yy + trackHeight / 2 - svg_sz / 2,
                              svg_sz,
                              svg_sz));

            if(!header->isSectionHidden(section))
            {
                switch (section) {
                case COL_TRACK_IDX:
                    if (track->selected())
                        p.setFont(fbd);
                    p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, QString::number(MusEGlobal::song->tracks()->index(track) + 1));
                    if (p.font().bold())
                        p.setFont(font());
                    break;

                case COL_INPUT_MONITOR:
                    if (track->canRecordMonitor()) {
                        (track->recMonitor() ? monitorOnSVGIcon : monitorOffSVGIcon)->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                    }
                    break;

                case COL_RECORD:
                    if (track->canRecord()) {
                        if (track->type() == MusECore::Track::AUDIO_OUTPUT)
                            (track->recordFlag() ? downmixOnSVGIcon : downmixOffSVGIcon)->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                        else
                            (track->recordFlag() ? recArmOnSVGIcon : recArmOffSVGIcon)->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                    }
                    break;

                case COL_CLASS:
                {
                    if(const QIcon* icon = MusECore::Track::trackTypeIcon(type))
                        icon->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                }
                    break;

                case COL_MUTE:
                    if (track->off())
                        trackOffSVGIcon->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                    else
                    {
                        if(!track->internalSolo() && !track->solo() &&
                                ((solo_t_1 && solo_t_1 != track) || (solo_t_2 && solo_t_2 != track)))
                            (track->mute() ? muteAndProxyOnSVGIcon : muteProxyOnSVGIcon)->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                        else if(track->mute())
                            muteOnSVGIcon->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                    }
                    break;

                case COL_SOLO:
                    if(track->solo() && track->internalSolo())
                        soloAndProxyOnSVGIcon->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                    else
                        if(track->internalSolo())
                            soloProxyOnAloneSVGIcon->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                        else
                            if (track->solo())
                                soloOnAloneSVGIcon->paint(&p, svg_r, Qt::AlignCenter, QIcon::Normal, QIcon::On);
                    break;

//                case COL_TIMELOCK: // unused
//                    if (track->isMidiTrack() && track->locked()) {
//                        drawCenteredPixmap(p, lockIcon, r);
//                    }
//                    break;

                case COL_NAME:
                    if (track->selected())
                        p.setFont(fbd);
                    p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, track->name());
                    if (p.font().bold())
                        p.setFont(font());
                    break;

                case COL_OCHANNEL:
                {
                    QString s;
                    int n;
                    // Default to track port if -1 and track channel if -1.
                    if (track->isMidiTrack()) {
                        n = ((MusECore::MidiTrack*)track)->outChannel() + 1;
                    }
                    else {
                        // show number of ports
                        n = ((MusECore::WaveTrack*)track)->channels();
                    }
                    s.setNum(n);
                    p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, s);
                }
                    break;

                case COL_OPORT:
                {
                    QString s;
                    if (track->isMidiTrack()) {
                        int outport = ((MusECore::MidiTrack*)track)->outPort();
                        s = QString("%1:%2").arg(outport+1).arg(MusEGlobal::midiPorts[outport].portname());
                    }
                    else if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
                    {
                        MusECore::MidiDevice* md = dynamic_cast<MusECore::MidiDevice*>(track);
                        if(md)
                        {
                            int outport = md->midiPort();
                            if((outport >= 0) && (outport < MusECore::MIDI_PORTS))
                                s = QString("%1:%2").arg(outport+1).arg(MusEGlobal::midiPorts[outport].portname());
                            else
                                s = tr("<none>");
                        }
                    }
                    else
                    {
                        p.setFont(fit);
                        s = "n/a";
                    }

                    p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, s);
                    if (p.font().italic())
                        p.setFont(font());
                }
                    break;

                case COL_AUTOMATION:
                {
                    QString s;

                    if (!track->isMidiTrack()) {
                        MusECore::CtrlListList* cll = ((MusECore::AudioTrack*)track)->controller();
                        int countAll=0, countVisible=0;
                        for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
                            MusECore::CtrlList *cl = icll->second;
                            if (!cl->dontShow())
                                countAll++;
                            if (cl->isVisible())
                                countVisible++;
                        }
                        s = QString(" %1(%2) %3").arg(countVisible).arg(countAll).arg(tr("visible"));
                    } else {
                        p.setFont(fit);
                        s = "n/a";
                    }

                    p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, s);
                    if (p.font().italic())
                        p.setFont(font());
                }
                    break;

                case COL_CLEF:
                {
                    QString s;
                    if (track->isMidiTrack() && track->type() == MusECore::Track::MIDI) { // no drum tracks!
                        if (((MusECore::MidiTrack*)track)->getClef() == trebleClef)
                            s=tr("Treble");
                        else if (((MusECore::MidiTrack*)track)->getClef() == bassClef)
                            s=tr("Bass");
                        else if (((MusECore::MidiTrack*)track)->getClef() == grandStaff)
                            s=tr("Grand");
                    } else {
                        p.setFont(fit);
                        s = "n/a";
                    }

                    p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, s);
                    if (p.font().italic())
                        p.setFont(font());
                }
                    break;

                default:
                    if (section >= COL_CUSTOM_MIDICTRL_OFFSET)
                    {
                        if (track->isMidiTrack())
                        {
                            int col_ctrl_no=Arranger::custom_columns[section - COL_CUSTOM_MIDICTRL_OFFSET].ctrl;
                            MusECore::MidiTrack* mt=dynamic_cast<MusECore::MidiTrack*>(track);
                            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                            const int chan = mt->outChannel();
                            MusECore::MidiController* mctl = mp->midiController(col_ctrl_no, chan);
                            int val;
                            if (Arranger::custom_columns[section - COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                                    Arranger::custom_col_t::AFFECT_BEGIN)
                                val=mt->getControllerChangeAtTick(0,col_ctrl_no,MusECore::CTRL_VAL_UNKNOWN);
                            else
                            {
                                val=mp->hwCtrlState(mt->outChannel(), col_ctrl_no);
                                old_ctrl_hw_states[mt][section]=val;
                            }

                            if (val!=MusECore::CTRL_VAL_UNKNOWN)
                                val-=mctl->bias();

                            if (col_ctrl_no!=MusECore::CTRL_PROGRAM)
                            {
                                p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter,
                                           (val!=MusECore::CTRL_VAL_UNKNOWN)?QString::number(val):tr("off"));
                            }
                            else
                            {
                                MusECore::MidiInstrument* instr = mp->instrument();
                                QString name;
                                if (val!=MusECore::CTRL_VAL_UNKNOWN)
                                    name = instr->getPatchName(mt->outChannel(), val, mt->isDrumTrack(), true); // Include default.
                                else
                                    name = tr("<unknown>");

                                p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, name);
                            }
                        }
                    }
                    break;
                }
            }
            x += header->sectionSize(section);
        }
        p.setPen(MusEGlobal::config.trackSectionDividerColor);
        p.drawLine(x1, yy, x2, yy);
    }
    p.drawLine(x1, yy, x2, yy);

    if (mode == DRAG) {
        int yy = curY - dragYoff;
        p.setPen(Qt::green);
        p.drawLine(x1, yy, x2, yy);
        p.drawLine(x1, yy + dragHeight, x2, yy+dragHeight);
    }

    //---------------------------------------------------
    //    draw vertical lines
    //---------------------------------------------------

    int n = header->count();
    int xpos = -1;
    p.setPen(MusEGlobal::config.trackSectionDividerColor);
    for (int index = 0; index < n; index++) {
        int section = header->logicalIndex(index);
        xpos += header->sectionSize(section);
        p.drawLine(xpos, 0, xpos, height());
    }
}

void TList::maybeUpdateVolatileCustomColumns()
{
    MusECore::TrackList* l = MusEGlobal::song->tracks();
    int idx = 0;
    int yy  = -ypos;
    for (MusECore::iTrack i = l->begin(); i != l->end(); ++idx, yy += (*i)->height(), ++i)
    {
        MusECore::Track* track = *i;
        int trackHeight = track->height();
        if (trackHeight==0) // not visible
            continue;


        int x = 0;
        for (int index = 0; index < header->count(); ++index)
        {
            int section = header->logicalIndex(index);

            if (section>=COL_CUSTOM_MIDICTRL_OFFSET && track->isMidiTrack() &&
                    (Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                     Arranger::custom_col_t::AFFECT_CPOS) )
            {
                int w   = header->sectionSize(section);
                QRect r = QRect(x+2, yy, w-4, trackHeight);

                int ctrl_no = Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;

                MusECore::MidiTrack* mt=(MusECore::MidiTrack*)track;
                MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                int new_val = mp->hwCtrlState(mt->outChannel(), ctrl_no);

                if (new_val != old_ctrl_hw_states[track][section])
                    update(r);
            }

            x += header->sectionSize(section);
        }
    }
}

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void TList::returnPressed()
{
    if(editTrack)
    {
        if(editor && editor->isVisible())
        {
            //editor->hide();
            if (editor->text() != editTrack->name()) {
                MusECore::TrackList* tl = MusEGlobal::song->tracks();
                for (MusECore::iTrack i = tl->begin(); i != tl->end(); ++i) {
                    if ((*i)->name() == editor->text()) {
                        editTrack = nullptr;
                        editor->blockSignals(true);
                        editor->hide();
                        editor->blockSignals(false);
                        QMessageBox::critical(this,
                                              tr("MusE: bad trackname"),
                                              tr("Please choose a unique track name"),
                                              QMessageBox::Ok,
                                              Qt::NoButton,
                                              Qt::NoButton);
                        setFocus();
                        return;
                    }
                }

                MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackName,
                                                                  editTrack,
                                                                  editTrack->name(),
                                                                  editor->text()));
            }
        }

        editTrack = nullptr;
    }

    editMode = false;
    editJustFinished = true;
    if(editor && editor->isVisible())
    {
        editor->blockSignals(true);
        editor->hide();
        editor->blockSignals(false);
    }
    setFocus();
}

void TList::chanValueFinished()
{
    if(editTrack)
    {
        // Default to track port if -1 and track channel if -1.
        const int channel = chan_edit->value() - (editTrack->isMidiTrack() ? 1 : 0); // Subtract 1 from midi channel display.
        setTrackChannel(editTrack, false, channel, 0);
        editTrack = nullptr;
    }

    editMode = false;
    editJustFinished=true;
    if(chan_edit->isVisible())
    {
        chan_edit->blockSignals(true);
        chan_edit->hide();
        chan_edit->blockSignals(false);
    }
    setFocus();
}

void TList::ctrlValueFinished()
{
    if(editTrack && editTrack->isMidiTrack())
    {
        MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editTrack);
        //if (mt && mt->type() != MusECore::Track::DRUM)
        // Default to track port if -1 and track channel if -1.
        // TODO TEST: Why was DRUM excluded? I want to say just "if(mt)", but will it break something with dynamic columns?  // REMOVE Tim.
        if (mt)
        {
            int val = ctrl_edit->value();
            MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
            const int chan = mt->outChannel();
            MusECore::MidiController* mctl = mp->midiController(ctrl_num, chan);

            if (val==ctrl_edit->minimum())
                val=MusECore::CTRL_VAL_UNKNOWN;
            else
                val+=mctl->bias();

            if (val!=MusECore::CTRL_VAL_UNKNOWN)
            {
                record_controller_change_and_maybe_send(ctrl_at_tick, ctrl_num, val, mt);
            }
            else
            {
                MusECore::Undo operations;
                for (MusECore::iPart p = mt->parts()->begin(); p!=mt->parts()->end(); p++)
                {
                    if (p->second->tick()==0)
                    {
                        for (MusECore::ciEvent ev=p->second->events().begin(); ev!=p->second->events().end(); ev++)
                        {
                            if (ev->second.tick()!=0) break;
                            else if (ev->second.type()==MusECore::Controller && ev->second.dataA()==ctrl_num)
                            {
                                using MusECore::UndoOp;
                                operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, p->second, false, false));
                                break;
                            }
                        }
                    }
                }
                MusEGlobal::song->applyOperationGroup(operations);
            }
        }

        editTrack = nullptr;
    }

    editMode = false;
    editJustFinished=true;
    if(ctrl_edit->isVisible())
    {
        ctrl_edit->blockSignals(true);
        ctrl_edit->hide();
        ctrl_edit->blockSignals(false);
    }
    setFocus();
}

//---------------------------------------------------------
//   adjustScrollbar
//---------------------------------------------------------

void TList::adjustScrollbar()
{
    int h = 0;
    MusECore::TrackList* l = MusEGlobal::song->tracks();
    for (MusECore::iTrack it = l->begin(); it != l->end(); ++it)
        h += (*it)->height();
    _scroll->setMaximum(h +30);
    redraw();
}

//---------------------------------------------------------
//   y2Track
//---------------------------------------------------------

MusECore::Track* TList::y2Track(int y) const
{
    MusECore::TrackList* l = MusEGlobal::song->tracks();
    int ty = 0;
    for (MusECore::iTrack it = l->begin(); it != l->end(); ++it) {
        int h = (*it)->height();
        if (y >= ty && y < ty + h)
            return *it;
        ty += h;
    }
    return 0;
}

void TList::muteSelectedTracksSlot()
{
    bool stateDefined=false;
    bool setTo;
    MusECore::PendingOperationList operations;
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t)
    {
        if ((*t)->selected()){
            if (!stateDefined)
            {
                setTo = !(*t)->isMute();
                stateDefined = true;
            }
            operations.add(MusECore::PendingOperationItem((*t), setTo, MusECore::PendingOperationItem::SetTrackMute));
        }
    }
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
    update();
}

void TList::soloSelectedTracksSlot()
{
    bool stateDefined=false;
    bool setTo;
    MusECore::PendingOperationList operations;
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t)
    {
        if ((*t)->selected()){
            if (!stateDefined)
            {
                setTo = !(*t)->soloMode();
                stateDefined = true;
            }
            operations.add(MusECore::PendingOperationItem((*t), setTo, MusECore::PendingOperationItem::SetTrackSolo));
        }
    }
    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
    update();
}

void TList::incrementController(MusECore::Track* t, int controllerType, int incrementValue)
{
    MusECore::MidiTrack* midiTrack = static_cast<MusECore::MidiTrack*>(t);
    const int channel = midiTrack->outChannel();

    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[midiTrack->outPort()];
    MusECore::MidiCtrlValListList* mcvll = mp->controller();
    MusECore::ciMidiCtrlValList imcvl = mcvll->find(channel, controllerType);
    MusECore::MidiCtrlValList* mcvl = imcvl->second;

    MusECore::MidiController* mc = mp->midiController(controllerType, channel, false);

    int value = mcvl->lastValidHWVal();
    int max = 127;
    int min = 0;
    int bias = 0;
    if(mc)
    {
        max = mc->maxVal();
        min = mc->minVal();
        bias = mc->bias();

        if (value == MusECore::CTRL_VAL_UNKNOWN)
        {
            value = mc->initVal() + bias;
            //printf("Controller not yet set, resetting to default (%d)\n", value);
        }
    }

    // checking ranges seems to need bias removed
    value -= bias;
    value += incrementValue;

    if (value > max)
        value = max;
    if (value < min)
        value = min;

    value += bias;

    mp->putControllerValue(midiTrack->outPort(), channel, controllerType, value, false);
}

void TList::volumeSelectedTracksSlot(int incrementValue)
{
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for (auto t: *tracks)
    {
        if (!t->selected())
            continue;

        if (t->type() == MusECore::Track::MIDI || t->type() == MusECore::Track::DRUM)
        {
            incrementController(t, MusECore::CTRL_VOLUME, incrementValue*2);
        }
        else
        {
            MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
            float vol = at->volume();
            float dbVol = muse_val2dbr(vol);
            float newVolume = dbVol + float(incrementValue)/2;
            if (newVolume < MusEGlobal::config.minSlider)
                newVolume = MusEGlobal::config.minSlider;
            if (newVolume > 10.0)
                newVolume = 10.0;
            at->setVolume(muse_db2val(newVolume));
        }
    }
}

void TList::panSelectedTracksSlot(int incrementValue)
{
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for (auto t : *tracks)
    {
        if (!t->selected())
            continue;

        if (t->type() == MusECore::Track::MIDI || t->type() == MusECore::Track::DRUM)
        {
            incrementController(t, MusECore::CTRL_PANPOT, incrementValue);
        }
        else
        {
            MusECore::AudioTrack* at = static_cast<MusECore::AudioTrack*>(t);
            float newPan = at->pan() + 0.01 * incrementValue;
            if (newPan < -1.0)
                newPan = -1.0;
            if (newPan > 1.0)
                newPan = 1.0;
            at->setPan(newPan);
        }
    }
}


void TList::editTrackNameSlot()
{
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    if (tracks->countSelected() == 1) {
        for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t)
            if ((*t)->selected()){
                editTrackName(*t);
                break;
            }
    }
}

void TList::editTrackName(MusECore::Track *t)
{
    int colx = header->sectionPosition(COL_NAME);
    int colw = header->sectionSize(COL_NAME);
    int coly = t->y() - ypos;
    int colh = t->height();
    editTrack = t;
    if (editor == nullptr) {
        editor = new QLineEdit(this);
        editor->setFrame(false);
        connect(editor, SIGNAL(editingFinished()), SLOT(returnPressed()));
    }
    editor->setText(editTrack->name());
    editor->selectAll();
    editor->setGeometry(colx, coly, colw, colh);
    editMode = true;
    editor->show();
    editor->setFocus();
}

void TList::setTrackChannel(MusECore::Track *track, bool isDelta, int channel, int delta, bool doAllTracks)
{
    MusECore::Undo operations;
    if(track->isMidiTrack())
    {
        MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(track);

        if(!doAllTracks && !track->selected())
        {
            if(isDelta) {
                channel = mt->outChannel() + delta;
                if(channel >= MusECore::MUSE_MIDI_CHANNELS)
                    channel = 0;
                else if(channel < 0)
                    channel = MusECore::MUSE_MIDI_CHANNELS - 1;
            }

            channel = qMin(channel, MusECore::MUSE_MIDI_CHANNELS - 1);
            channel = qMax(channel, 0);

            if(channel != mt->outChannel())
                operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackChannel, mt, mt->outChannel(), channel));
        }
        else
        {
            for (const auto& t : *MusEGlobal::song->midis())
            {
                if (doAllTracks && (t->type() != mt->type()))
                    continue; // only tracks of the same type

                if(isDelta) {
                    channel = t->outChannel() + delta;
                    if(channel >= MusECore::MUSE_MIDI_CHANNELS)
                        channel = 0;
                    else if (channel < 0)
                        channel = MusECore::MUSE_MIDI_CHANNELS - 1;
                }

                channel = qMin(channel, MusECore::MUSE_MIDI_CHANNELS - 1);
                channel = qMax(channel, 0);

                if(channel != t->outChannel() && (doAllTracks || t->selected()))
                    operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackChannel, t, t->outChannel(), channel));
            }
        }

        if(!operations.empty())
            MusEGlobal::song->applyOperationGroup(operations);
    }
    else
    {
        if(track->type() != MusECore::Track::AUDIO_SOFTSYNTH)
        {
            if(!doAllTracks && !track->selected())
            {
                if(isDelta) {
                    channel = track->channels() + delta;
                    if(channel > MusECore::MAX_CHANNELS)
                        channel = 1;
                    else if(channel < 1)
                        channel = MusECore::MAX_CHANNELS;
                }

                channel = qMin(channel, MusECore::MAX_CHANNELS);
                channel = qMax(channel, 1);

                if(channel != track->channels())
                    operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackChannel, track, track->channels(), channel));
            }
            else
            {
                for(const auto& t : *MusEGlobal::song->tracks())
                {
                    if(t->isMidiTrack())
                        continue;
                    if (doAllTracks && (t->type() != track->type()))
                        continue; // only tracks of the same type

                    if(isDelta) {
                        channel = t->channels() + delta;
                        if(channel > MusECore::MAX_CHANNELS)
                            channel = 1;
                        else if(channel < 1)
                            channel = MusECore::MAX_CHANNELS;
                    }

                    channel = qMin(channel, MusECore::MAX_CHANNELS);
                    channel = qMax(channel, 1);

                    if(channel != t->channels() && (doAllTracks || t->selected()))
                        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackChannel, t, t->channels(), channel));
                }
            }

            if(!operations.empty())
                MusEGlobal::song->applyOperationGroup(operations);
        }
    }
}

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void TList::mouseDoubleClickEvent(QMouseEvent* ev)
{
    if((editor && (editor->isVisible() || editor->hasFocus())) ||
            (chan_edit && (chan_edit->isVisible() || chan_edit->hasFocus())) ||
            (ctrl_edit && (ctrl_edit->isVisible() || ctrl_edit->hasFocus())))
    {
        ev->accept();
        return;
    }

    int button  = ev->button();
    if(button != Qt::LeftButton) {
        ev->accept();
        return;
    }

    int x       = ev->x();
    int section = header->logicalIndexAt(x);
    if (section == -1)
    {
        ev->accept();
        return;
    }

    MusECore::Track* t = y2Track(ev->y() + ypos);
    if(t == nullptr)
    {
        ev->accept();
        return;
    }

    const bool shift = ((QInputEvent*)ev)->modifiers() & Qt::ShiftModifier;

    int colx = header->sectionPosition(section);
    int colw = header->sectionSize(section);
    int coly = t->y() - ypos;
    int colh = t->height();

    if (t) {
        if (section == COL_NAME) {
            editTrackName(t);
        }
        else if (section == COL_OPORT || section == COL_CLASS) {
            if (t->isSynthTrack() || t->isMidiTrack())
                openSynthGui(t);
        }
        else if (section == COL_TRACK_IDX) {
            if (button == Qt::LeftButton) {
                if (shift) {
                    // Select all tracks of the same type
                    MusEGlobal::song->selectAllTracks(false);
                    MusECore::TrackList* all_tl = MusEGlobal::song->tracks();
                    for (const auto tit : *all_tl) {
                        if (tit->type() == t->type())
                            tit->setSelected(true);
                    }
                } else {
                    MusEGlobal::song->selectAllTracks(true);
                }
            }
            MusEGlobal::song->update(SC_TRACK_SELECTION);
        }
        else if (section == COL_OCHANNEL) {
            // Enabled for audio tracks. And synth channels cannot be changed ATM.
            // Default to track port if -1 and track channel if -1.
            if(t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
            {
                ev->accept();
                return;
            }

            // A disabled spinbox up or down button will pass the event to the parent! Causes pseudo 'wrapping'. Eat it up.
            editTrack=t;
            if (!chan_edit)
            {
                chan_edit=new QSpinBox(this);
                chan_edit->setFrame(false);
                chan_edit->setMinimum(1);
                connect(chan_edit, SIGNAL(editingFinished()), SLOT(chanValueFinished()));
            }
            if (t->isMidiTrack())
            {
                chan_edit->setMaximum(MusECore::MUSE_MIDI_CHANNELS);
                chan_edit->setValue(((MusECore::MidiTrack*)editTrack)->outChannel()+1);
            }
            else // if(t->type() != MusECore::Track::AUDIO_SOFTSYNTH)
            {
                chan_edit->setMaximum(MusECore::MAX_CHANNELS);
                chan_edit->setValue(((MusECore::AudioTrack*)editTrack)->channels());
            }
            int w=colw;
            if (w < chan_edit->sizeHint().width()) w=chan_edit->sizeHint().width();
            chan_edit->setGeometry(colx, coly, w, colh);
            chan_edit->selectAll();
            editMode = true;
            chan_edit->show();
            chan_edit->setFocus();
        }
        else if (section >= COL_CUSTOM_MIDICTRL_OFFSET)
        {
            if (t->isMidiTrack())
            {
                editTrack=t;

                ctrl_num=Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;

                MusECore::MidiTrack* mt=(MusECore::MidiTrack*)t;
                MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                const int chan = mt->outChannel();
                MusECore::MidiController* mctl = mp->midiController(ctrl_num, chan);

                if (ctrl_num!=MusECore::CTRL_PROGRAM)
                {
                    if (Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                            Arranger::custom_col_t::AFFECT_BEGIN)
                        ctrl_at_tick=0;
                    else
                        ctrl_at_tick=MusEGlobal::song->cpos();

                    if (ctrl_edit==nullptr)
                    {
                        ctrl_edit=new QSpinBox(this);
                        ctrl_edit->setSpecialValueText(tr("off"));
                        connect(ctrl_edit, SIGNAL(editingFinished()), SLOT(ctrlValueFinished()));
                    }

                    ctrl_edit->setMinimum(mctl->minVal()-1); // -1 because of the specialValueText
                    ctrl_edit->setMaximum(mctl->maxVal());
                    ctrl_edit->setValue(((MusECore::MidiTrack*)editTrack)->getControllerChangeAtTick(0,ctrl_num)-mctl->bias());
                    int w=colw;
                    if (w < ctrl_edit->sizeHint().width()) w=ctrl_edit->sizeHint().width();
                    ctrl_edit->setGeometry(colx, coly, w, colh);
                    editMode = true;
                    ctrl_edit->show();
                    ctrl_edit->setFocus();
                }
            }
        }
    }
    ev->accept();
}

//---------------------------------------------------------
//   synthGUI context menu
//---------------------------------------------------------

void TList::showMidiClassPopupMenu(MusECore::Track* t, int x, int y)
{
    if (t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
    {
        MusECore::SynthI* synth = static_cast<MusECore::SynthI*>(t);
        PopupMenu* p = new PopupMenu;

        QAction* cact = p->addAction(*MusEGui::ankerSVGIcon, tr("Configure MIDI Ports/Soft Synths..."));
        p->addSeparator();

        if(!synth->synth())
            p->addAction(tr("SYNTH IS UNAVAILABLE!"));

        QAction* gact = p->addAction(tr("Show Generic Synth GUI"));
        gact->setCheckable(true);
        gact->setEnabled(synth->hasGui());
        gact->setChecked(synth->guiVisible());

        QAction* nact = p->addAction(tr("Show Native Synth GUI"));
        nact->setCheckable(true);
        nact->setEnabled(synth->hasNativeGui());
        nact->setChecked(synth->nativeGuiVisible());

#ifdef LV2_SUPPORT
        PopupMenu *mSubPresets = nullptr;
        //show presets submenu for lv2 synths
        if(synth->synth() && synth->synth()->synthType() == MusECore::Synth::LV2_SYNTH)
        {
            mSubPresets = new PopupMenu(tr("Presets"));
            p->addMenu(mSubPresets);
            static_cast<MusECore::LV2SynthIF *>(synth->sif())->populatePresetsMenu(mSubPresets);
        }
#endif

        // If it has a gui but we don't have OSC, disable the action.
#ifndef OSC_SUPPORT
#ifdef DSSI_SUPPORT
        if(synth->synth() && synth->synth()->synthType() == MusECore::Synth::DSSI_SYNTH)
        {
            nact->setChecked(false);
            nact->setEnabled(false);
        }
#endif
#endif

        QAction* ract = p->exec(mapToGlobal(QPoint(x, y)), nullptr);
        if (ract == gact) {
            bool show = !synth->guiVisible();
            synth->showGui(show);
        }
        else if (ract == nact) {
            bool show = !synth->nativeGuiVisible();
            synth->showNativeGui(show);
        }
        else if (ract == cact) {
            MusEGlobal::muse->configMidiPorts();
        }

#ifdef LV2_SUPPORT
        else if (mSubPresets != nullptr && ract != nullptr && ract->data().canConvert<void *>()) {
            static_cast<MusECore::LV2SynthIF *>(synth->sif())->applyPreset(ract->data().value<void *>());
        }
#endif
        delete p;
        return;
    }


    // MIDI tracks
    if (t->type() != MusECore::Track::MIDI && t->type() != MusECore::Track::DRUM)
        return;

    int oPort = static_cast<MusECore::MidiTrack*>(t)->outPort();
    MusECore::MidiPort* port = &MusEGlobal::midiPorts[oPort];

    PopupMenu* p = new PopupMenu;

    QAction *switchact = nullptr, *gact = nullptr, *nact = nullptr;

#ifdef LV2_SUPPORT
    PopupMenu *mSubPresets = nullptr;
#endif

    if (t->type() == MusECore::Track::MIDI)
        switchact = p->addAction(*drumeditSVGIcon, tr("Convert MIDI to Drum Track"));
    else
        switchact = p->addAction(*pianorollSVGIcon, tr("Convert Drum to MIDI Track"));


    if(port->device() && port->device()->isSynti())
    {
        MusECore::SynthI* synth = static_cast<MusECore::SynthI*>(port->device());
        if(synth->synth()) {

            p->addSeparator();

            gact = p->addAction(tr("Show Generic Synth GUI"));
            gact->setCheckable(true);
            gact->setEnabled(port->hasGui());
            gact->setChecked(port->guiVisible());

            nact = p->addAction(tr("Show Native Synth GUI"));
            nact->setCheckable(true);
            nact->setEnabled(port->hasNativeGui());
            nact->setChecked(port->nativeGuiVisible());

            // If it has a gui but we don't have OSC, disable the action.
#ifndef OSC_SUPPORT
#ifdef DSSI_SUPPORT
            if(port->device() && port->device()->isSynti())
            {
                MusECore::SynthI* synth = static_cast<MusECore::SynthI*>(port->device());
                if(synth->synth() && synth->synth()->synthType() == MusECore::Synth::DSSI_SYNTH)
                {
                    nact->setChecked(false);
                    nact->setEnabled(false);
                }
            }
#endif
#endif

#ifdef LV2_SUPPORT
            if(port->device() && port->device()->isSynti())
            {
                MusECore::SynthI* synth = static_cast<MusECore::SynthI*>(port->device());
                //show presets submenu for lv2 synths
                if(synth->synth() && synth->synth()->synthType() == MusECore::Synth::LV2_SYNTH)
                {
                    mSubPresets = new PopupMenu(tr("Presets"));
                    p->addMenu(mSubPresets);
                    static_cast<MusECore::LV2SynthIF *>(synth->sif())->populatePresetsMenu(mSubPresets);
                }
            }
#endif
        }
    }

    QAction* ract = p->exec(mapToGlobal(QPoint(x, y)), nullptr);
    if (ract == gact) {
        port->showGui(!port->guiVisible());
    }
    else if (ract == nact) {
        port->showNativeGui(!port->nativeGuiVisible());
    }
    else if (ract == switchact) {
        if (!t->selected())
            changeTrackToType(t, t->type() == MusECore::Track::MIDI ? MusECore::Track::DRUM : MusECore::Track::MIDI);
        else
        {
            MusECore::Track::TrackType curType = t->type();
            for (auto const myt : *MusEGlobal::song->tracks())
            {
                if (myt->selected() && myt->type() == curType)
                    changeTrackToType(myt, myt->type() == MusECore::Track::MIDI ? MusECore::Track::DRUM : MusECore::Track::MIDI);
            } // track for-loop
        }
    }


#ifdef LV2_SUPPORT
    else if (mSubPresets != nullptr && ract != nullptr && ract->data().canConvert<void *>())
    {
        if (port->device() && port->device()->isSynti())
        {
            MusECore::SynthI* synth = static_cast<MusECore::SynthI*>(port->device());
            static_cast<MusECore::LV2SynthIF *>(synth->sif())->applyPreset(ract->data().value<void *>());
        }
    }
#endif

    delete p;
}

//---------------------------------------------------------
//   audio output context menu
//---------------------------------------------------------

void TList::showAudioOutPopupMenu(MusECore::Track* t, int x, int y)
{
    if (t->type() != MusECore::Track::AUDIO_OUTPUT)
        return;

    PopupMenu* p = new PopupMenu;

    QAction* actTrack = p->addAction(*MusEGui::downmixTrackSVGIcon, tr("Render Downmix to Selected Wave Track"));
    actTrack->setEnabled(!MusEGlobal::audio->bounce());
    QAction* actFile = p->addAction(*MusEGui::downmixOnSVGIcon, tr("Render Downmix to a File..."));
    actFile->setEnabled(!MusEGlobal::audio->bounce());

    QAction* ract = p->exec(mapToGlobal(QPoint(x, y)), nullptr);
    if (ract == actFile)
        MusEGlobal::muse->bounceToFile(static_cast<MusECore::AudioOutput*>(t));
    else if (ract == actTrack)
        MusEGlobal::muse->bounceToTrack(static_cast<MusECore::AudioOutput*>(t));

    delete p;
}

//---------------------------------------------------------
//   tracklistChanged
//---------------------------------------------------------

void TList::tracklistChanged()
{
    redraw();
}

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void TList::keyPressEvent(QKeyEvent* e)
{
    if ( e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        e->accept();
        return;
    }

    if (editMode)
    {
        if ( e->key() == Qt::Key_Escape )
        {
            if(editor && editor->isVisible())
            {
                editor->blockSignals(true);
                editor->hide();
                editor->blockSignals(false);
            }
            if(chan_edit && chan_edit->isVisible())
            {
                chan_edit->blockSignals(true);
                chan_edit->hide();
                chan_edit->blockSignals(false);
            }
            if(ctrl_edit && ctrl_edit->isVisible())
            {
                ctrl_edit->blockSignals(true);
                ctrl_edit->hide();
                ctrl_edit->blockSignals(false);
            }
            editTrack = nullptr;
            editMode = false;
            setFocus();
            return;
        }
        return;
    }
    else if (!editJustFinished)
    {
        emit keyPressExt(e); //redirect keypress events to main app. don't call this when confirming an editor
    }
    else
        editJustFinished=false;

    emit keyPressExt(e); //redirect keypress events to main app
}

//---------------------------------------------------------
//   moveSelection
//---------------------------------------------------------

void TList::moveSelection(int n)
{
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();

    int nselect = tracks->countSelected();
    if (nselect > 1) {
      // remove selection for all but the first track
      MusECore::Track* selTrack = nullptr;
      for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {

        if (selTrack == nullptr) {
          if ((*t)->selected())
            selTrack = *t;
        } else {
          if ((*t)->selected())
            (*t)->setSelected(false);
        }
      }
    }

    MusECore::Track* selTrack = nullptr;
    for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
        MusECore::iTrack s = t;
        if ((*t)->selected()) {
            if (n > 0) { // Move down
                while (n--) {
                    ++t;
                    if (t == tracks->end()) {
                        --t;
                        break;
                    }
                    // skip over hidden tracks
                    if (!(*t)->isVisible()) {
                        n++;
                        continue;
                    }
                    selTrack = *t;
                    break;
                }
            }
            else { // Move up
                while (n++ != 0) {
                    if (t == tracks->begin())
                        break;
                    --t;
                    // skip over hidden tracks
                    if (!(*t)->isVisible()) {
                        n--;
                        continue;
                    }
                    selTrack = *t;
                    break;
                }
            }
            if(selTrack)
            {
                (*s)->setSelected(false);
                selTrack->setSelected(true);

                // if selected track is outside of view, enforce scrolling
                if (selTrack->y() > this->height()+ypos-20)
                {
                    emit verticalScrollSetYpos(ypos+selTrack->height());
                }
                else if (selTrack->y() < ypos)
                {
                    emit verticalScrollSetYpos(selTrack->y());
                }

                // rec enable track if expected
                MusECore::TrackList recd = getRecEnabledTracks();

                if (!MusEGlobal::audio->isRecording() &&
                        recd.size() == 1 &&
                        MusEGlobal::config.moveArmedCheckBox) { // one rec enabled track, move rec enabled with selection
                    MusEGlobal::song->setRecordFlag((MusECore::Track*)recd.front(),false);
                    MusEGlobal::song->setRecordFlag((selTrack),true);
                }

                if (editTrack && editTrack != selTrack)
                    returnPressed();
                redraw();
            }
            break;
        }
    }
    if(selTrack)
        MusEGlobal::song->update(SC_TRACK_SELECTION);
}

MusECore::TrackList TList::getRecEnabledTracks()
{
    MusECore::TrackList recEnabled;
    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
        if ((*t)->recordFlag())
            recEnabled.push_back(*t);
    }
    return recEnabled;
}

//---------------------------------------------------------
//   changeAutomation
//---------------------------------------------------------

void TList::changeAutomation(QAction* act)
{
    if(!editAutomation || editAutomation->isMidiTrack())
        return;

    if (act->data().toInt() == AUTO_INVALID)
        return;

    bool setRead = false;

    if (act->data().toInt() == AUTO_SHOW_ALL) {
        MusECore::CtrlListList* cll = static_cast<MusECore::AudioTrack*>(editAutomation)->controller();
        for (auto& it : *cll) {
            MusECore::CtrlList *cl = it.second;
            if (!cl->dontShow() && !cl->isVisible() && cl->size() > 0) {
                cl->setVisible(true);
                setRead = true;
            }
        }
    }
    else if (act->data().toInt() == AUTO_HIDE_ALL) {
        MusECore::CtrlListList* cll = static_cast<MusECore::AudioTrack*>(editAutomation)->controller();
        for (auto& it : *cll) {
            MusECore::CtrlList *cl = it.second;
            if (cl->isVisible())
                cl->setVisible(false);
        }
    }
    else {
        int colindex = act->data().toInt() & 0xff;
        int id = (act->data().toInt() & 0x00ffffff) >> 8;
        // Is it the midi control action or clear action item?
        if (colindex == AUTO_CLEAR_MIDI || colindex == AUTO_MIDI_ASSIGN)
            return;

        if (colindex < 100)
            return; // this was meant for changeAutomationColor
        // one of these days I'll rewrite this so it's understandable
        // this is just to get it up and running...

        MusECore::CtrlListList* cll = static_cast<MusECore::AudioTrack*>(editAutomation)->controller();
        for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
            MusECore::CtrlList *cl = icll->second;
            if (id == cl->id())  // got it, change state
                cl->setVisible(act->isChecked());
        }
        setRead = true;
    }

    // if automation is OFF for the track we change it to READ as a convenience
    // hopefully this confuses users far less than not understanding why the
    // automation does not do anything.
    if (setRead && static_cast<MusECore::AudioTrack*>(editAutomation)->automationType() == MusECore::AUTO_OFF)
    {
        MusEGlobal::audio->msgSetTrackAutomationType(static_cast<MusECore::AudioTrack*>(editAutomation), MusECore::AUTO_READ);
        if (MusEGlobal::debugMsg)
            printf("Changing automation from OFF to READ\n");
    }

    MusEGlobal::song->update(SC_TRACK_MODIFIED|SC_AUTOMATION);
}

//---------------------------------------------------------
//   changeAutomation
//---------------------------------------------------------
void TList::changeAutomationColor(QAction* act)
{
    if(!editAutomation || editAutomation->isMidiTrack())
        return;
    if(act->data().toInt() == AUTO_INVALID)
        return;
    int colindex = act->data().toInt() & 0xff;
    int id = (act->data().toInt() & 0x00ffffff) >> 8;

    // Is it the clear automation action item?
    // (As commented below, we should rewrite this to make it easier to understand..)
    if (colindex == AUTO_CLEAR_AUTO)
    {
        if(QMessageBox::question(MusEGlobal::muse, QString("Muse"),
                                 tr("Clear all controller events?"), tr("&Ok"), tr("&Cancel"),
                                 QString(), 0, 1 ) == 0)
        {
            MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
            MusEGlobal::audio->msgClearControllerEvents(track, id);

        }
    }


    // Is it the clear midi control action item?
    if(colindex == AUTO_CLEAR_MIDI)
    {
        MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
        MusECore::MidiAudioCtrlMap* macp = track->controller()->midiControls();
        MusECore::AudioMidiCtrlStructMap amcs;
        macp->find_audio_ctrl_structs(id, &amcs);
        if(!amcs.empty())
            MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
        for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
            macp->erase(*iamcs);
        if(!amcs.empty())
            MusEGlobal::audio->msgIdle(false);

        // Hm, need to remove the 'clear' item, and the status lines below it. Try this:
        QActionGroup* midi_actgrp = act->actionGroup();
        if(midi_actgrp)
        {
            QList<QAction*> act_list = midi_actgrp->actions();
            int sz = act_list.size();
            for(int i = 0; i < sz; ++i)
            {
                QAction* list_act = act_list.at(i);
                ///midi_actgrp->removeAction(list_act);
                // list_act has no parent now.
                ///delete list_act;
                list_act->setVisible(false); // HACK Cannot delete any actions! Causes crash with our PopupMenu due to recent fixes.
            }
        }
        return;
    }

    // Is it the midi control action item?
    if(colindex == AUTO_MIDI_ASSIGN)
    {
        MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
        MusECore::MidiAudioCtrlMap* macm = track->controller()->midiControls();
        MusECore::AudioMidiCtrlStructMap amcs;
        macm->find_audio_ctrl_structs(id, &amcs);

        int port = -1, chan = 0, ctrl = 0;
        for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
        {
            macm->hash_values((*iamcs)->first, &port, &chan, &ctrl);
            break; // Only a single item for now, thanks!
        }

        MidiAudioControl* pup = new MidiAudioControl(port, chan, ctrl);

        if(pup->exec() == QDialog::Accepted)
        {
            MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
            // Erase all for now.
            for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
                macm->erase(*iamcs);

            port = pup->port(); chan = pup->chan(); ctrl = pup->ctrl();
            if(port >= 0 && chan >=0 && ctrl >= 0)
                // Add will replace if found.
                macm->add_ctrl_struct(port, chan, ctrl, MusECore::MidiAudioCtrlStruct(id));

            MusEGlobal::audio->msgIdle(false);
        }

        delete pup;
        return;
    }

    if (colindex > 100)
        return; // this was meant for changeAutomation
    // one of these days I'll rewrite this so it's understandable
    // this is just to get it up and running...

    MusECore::CtrlListList* cll = ((MusECore::AudioTrack*)editAutomation)->controller();
    for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
        MusECore::CtrlList *cl = icll->second;
        if (cl->id() == id) { // got it, change color and enable
            cl->setColor(collist[colindex]);
            cl->setVisible(true);
        }
    }
    MusEGlobal::song->update(SC_TRACK_MODIFIED);
}

//---------------------------------------------------------
//   colorMenu
//---------------------------------------------------------
PopupMenu* TList::colorMenu(QColor c, int id, QWidget* parent)
{
    PopupMenu * m = new PopupMenu(parent, true);

    QActionGroup* col_actgrp = new QActionGroup(m);
    m->addAction(new MusEGui::MenuTitleItem(tr("Change color"), m));
    col_actgrp->setExclusive(true);
    for (int i = 0; i< 6; i++) {
        QPixmap pix(10,10);
        QPainter p(&pix);
        p.fillRect(0,0,10,10,collist[i]);
        QIcon icon(pix);
        QAction *act = col_actgrp->addAction(icon,colnames[i]);
        act->setCheckable(true);
        if (c == collist[i])
            act->setChecked(true);
        act->setData((id<<8) + i); // Shift 8 bits. Color in the bottom 8 bits.
    }
    m->addActions(col_actgrp->actions());

    //m->addSeparator();
    m->addAction(new MenuTitleItem(tr("Midi control"), m));

    if(editAutomation && !editAutomation->isMidiTrack())
    {
        QAction *act = m->addAction(tr("Assign"));
        act->setCheckable(false);
        act->setData((id<<8) + AUTO_MIDI_ASSIGN); // Shift 8 bits. Make midi menu the last item at 255.

        MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
        MusECore::MidiAudioCtrlMap* macm = track->controller()->midiControls();
        MusECore::AudioMidiCtrlStructMap amcs;
        macm->find_audio_ctrl_structs(id, &amcs);

        // Group only the clear and status items so they can both be easily removed when clear is clicked.
        if(!amcs.empty())
        {
            QActionGroup* midi_actgrp = new QActionGroup(m);
            QAction *cact = midi_actgrp->addAction(tr("Clear"));
            cact->setData((id<<8) + AUTO_CLEAR_MIDI); // Shift 8 bits. Make clear the second-last item at 254
            for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
            {
                int port, chan, mctrl;
                macm->hash_values((*iamcs)->first, &port, &chan, &mctrl);
                //QString s = QString("Port:%1 Chan:%2 Ctl:%3-%4").arg(port + 1)
                QString s = QString("Port:%1 Chan:%2 Ctl:%3").arg(port + 1)
                        .arg(chan + 1)
                        //.arg((mctrl >> 8) & 0xff)
                        //.arg(mctrl & 0xff);
                        .arg(MusECore::midiCtrlName(mctrl, true));
                QAction *mact = midi_actgrp->addAction(s);
                mact->setEnabled(false);
                mact->setData(AUTO_INVALID); // Not used
            }
            m->addActions(midi_actgrp->actions());
        }
    }
    m->addAction(new MenuTitleItem(tr("Other"), m));
    QAction *act = m->addAction(tr("Clear automation"));
    act->setCheckable(false);
    act->setData((id<<8) + AUTO_CLEAR_AUTO); // Shift 8 bits. Make clear menu item 253

    connect(m, SIGNAL(triggered(QAction*)), SLOT(changeAutomationColor(QAction*)));
    return m;

}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------
void TList::mousePressEvent(QMouseEvent* ev)
{
    if((editor && (editor->isVisible() || editor->hasFocus())) ||
            (chan_edit && (chan_edit->isVisible() || chan_edit->hasFocus())) ||
            (ctrl_edit && (ctrl_edit->isVisible() || ctrl_edit->hasFocus())))
    {
        ev->accept();
        return;
    }

    const int x       = ev->x();
    const int y       = ev->y();
    const int button  = ev->button();
    const bool ctrl   = ((QInputEvent*)ev)->modifiers() & Qt::ControlModifier;
    const bool shift  = ((QInputEvent*)ev)->modifiers() & Qt::ShiftModifier;

    MusECore::Track* t    = y2Track(y + ypos);

    TrackColumn col = TrackColumn(header->logicalIndexAt(x));
    if (t == nullptr) {
        if (button == Qt::RightButton) {

            // Show the menu
            QAction* act = addTrackMenu->exec(ev->globalPos(), nullptr);

            // Valid click?
            if(act)
            {
                t = MusEGlobal::song->addNewTrack(act);  // Add at end of list.
                if(t && t->isVisible())
                {
                    MusEGlobal::song->selectAllTracks(false);
                    t->setSelected(true);
                    MusEGlobal::song->update(SC_TRACK_SELECTION);
                    adjustScrollbar();
                }
            }

        }
        return;
    }

    MusECore::TrackList* tracks = MusEGlobal::song->tracks();
    dragYoff = y - (t->y() - ypos);
    startY   = y;

    if (resizeFlag) {
        mode = RESIZE;

        int y  = ev->y();
        int ty = -ypos;
        sTrack = 0;
        for (MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it, ++sTrack) {
            int h = (*it)->height();
            ty += h;
            if (y >= (ty-2)) {

                if ( (*it) == tracks->back() && y > ty ) { // DELETETHIS, only retain if(foo) break;?
                    //printf("tracks->back() && y > ty\n");
                }
                else if ( y > (ty+2) ) {
                    //printf(" y > (ty+2) \n");
                }
                else {
                    //printf("ogga ogga\n");
                    break;
                }
                //&& y < (ty)) DELETETHIS
                //     break;
            }
        }

        return;
    }

    mode = NORMAL;

    if (button == Qt::LeftButton && col != COL_INPUT_MONITOR && col != COL_RECORD && col != COL_MUTE && col != COL_SOLO)
    {
        mode = START_DRAG;  // Allow a track drag to start.

        if (ctrl) {
            if (tracks->countSelected() == 1 && tracks->currentSelection() == t)
                return;

            t->setSelected(!t->selected());
        }

        else if (shift) {
            if (tracks->countSelected() == 1 && tracks->currentSelection() == t)
                return;

            else if (tracks->countSelected() == 0)
                t->setSelected(true);

            else {
                int indexRange = 0, i1 = -1, i2 = -1;
                int indexClicked = tracks->index(t);
                for (MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it) {
                    if ( (*it)->selected() ) {
                        indexRange = tracks->index(*it);
                        if (indexRange < indexClicked) {
                            i1 = indexRange;
                            i2 = indexClicked;
                            break;
                        }
                    }
                }

                if (i1 == -1) {
                    i1 = indexClicked;
                    i2 = indexRange;
                }

                MusEGlobal::song->selectAllTracks(false);
                for (MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it) {
                    if (tracks->index(*it) >= i1 && tracks->index(*it) <= i2 && (*it) != t)
                        (*it)->setSelected(true);
                }
                t->setSelected(true);
            }

        } else {

            MusEGlobal::song->selectAllTracks(false);
            t->setSelected(true);

            // rec enable track if expected
            MusECore::TrackList recd = getRecEnabledTracks();
            if (!MusEGlobal::audio->isRecording() &&
                    recd.size() == 1 &&
                    MusEGlobal::config.moveArmedCheckBox) { // one rec enabled track, move rec enabled with selection
                MusEGlobal::song->setRecordFlag((MusECore::Track*)recd.front(),false);
                MusEGlobal::song->setRecordFlag(t,true);
            }
        }


        MusEGlobal::song->update(SC_TRACK_SELECTION);
    }
    else
    {
        switch (col) {

        case COL_TRACK_IDX:
            break;

        case COL_CLEF:
        {
            if (button == Qt::RightButton && t->isMidiTrack() && t->type() == MusECore::Track::MIDI) {
                QMenu* p = new QMenu;
                p->addAction(tr("Treble clef"))->setData(0);
                p->addAction(tr("Bass clef"))->setData(1);
                p->addAction(tr("Grand Staff"))->setData(2);

                // Show the menu
                QAction* act = p->exec(ev->globalPos(), nullptr);
                if (act) {
                    switch (act->data().toInt()) {
                    case 0:
                        ((MusECore::MidiTrack*)t)->setClef(trebleClef);
                        break;
                    case 1:
                        ((MusECore::MidiTrack*)t)->setClef(bassClef);
                        break;
                    case 2:
                        ((MusECore::MidiTrack*)t)->setClef(grandStaff);
                        break;
                    default:
                        break;
                    }
                }
                delete p;
            }
            break;
        }

        case COL_AUTOMATION:
        {
            if (button == Qt::RightButton && !t->isMidiTrack()) {
                editAutomation = t;
                PopupMenu* p = new PopupMenu(true);
                p->disconnect();
                p->clear();
                p->setTitle(tr("Viewable automation"));
                MusECore::CtrlListList* cll = static_cast<MusECore::AudioTrack*>(t)->controller();
                QAction* act = nullptr;
                int last_rackpos = -1;
                bool internalHeaderDone = false;
                bool synthHeaderDone = false;

                p->addAction(new MusEGui::MenuTitleItem(tr("Automation Display"), p));
                act = p->addAction(*dummySVGIcon, tr("Show All with Events"));
                act->setData(AUTO_SHOW_ALL);
                act = p->addAction(tr("Hide All"));
                act->setData(AUTO_HIDE_ALL);

                QList<const MusECore::CtrlList*> tmpList;

                for (const auto& icll : *cll) {
                    MusECore::CtrlList *cl = icll.second;
                    if (cl->dontShow())
                        continue;

                    int ctrl = cl->id();

                    if(ctrl < AC_PLUGIN_CTL_BASE)
                    {
                        if(!internalHeaderDone) {
                            p->addAction(new MusEGui::MenuTitleItem(tr("Internal"), p));
                            internalHeaderDone = true;
                        }
                        addAutoMenuAction(p, cl);
                    }
                    else
                    {
                        if (ctrl < static_cast<int>(MusECore::genACnum(MusECore::MAX_PLUGINS, 0)))  // The beginning of the special dssi synth controller block.
                        {
                            int rackpos = (ctrl - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
                            if (rackpos < MusECore::PipelineDepth)
                            {
                                if(rackpos != last_rackpos)
                                {
                                    outputAutoMenuSorted(p, tmpList);

                                    QString s = static_cast<MusECore::AudioTrack*>(t)->efxPipe() ?
                                                static_cast<MusECore::AudioTrack*>(t)->efxPipe()->name(rackpos) : QString();
                                    p->addAction(new MusEGui::MenuTitleItem(s, p));
                                    last_rackpos = rackpos;
                                }
                            }
                            tmpList.append(cl);
                        }
                        else
                        {
                            if (t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
                            {
                                if(!synthHeaderDone) {
                                    outputAutoMenuSorted(p, tmpList);
                                    p->addAction(new MusEGui::MenuTitleItem(tr("Synth"), p));
                                    synthHeaderDone = true;
                                }
                                tmpList.append(cl);
                            }
                        }
                    }
                }

                outputAutoMenuSorted(p, tmpList);

                connect(p, SIGNAL(triggered(QAction*)), SLOT(changeAutomation(QAction*)));
                p->exec(QCursor::pos());

                delete p;
            }
            break;
        }

        case COL_INPUT_MONITOR:
        {
            if(!t->canRecordMonitor())
                break;

            const bool val = !(t->recMonitor());

            if (button == Qt::LeftButton)
            {
                // apply to selected tracks
                if (t->selected() && tracks->countSelected() > 1) {
                    MusECore::Undo operations;
                    MusECore::TrackList* tl = MusEGlobal::song->tracks();
                    for (const auto tit : *tl)
                    {
                        if (tit->selected() && tit->canRecordMonitor())
                            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackRecMonitor, tit, val));
                    }
                    if (!operations.empty())
                        MusEGlobal::song->applyOperationGroup(operations);
                }
                else
                {
                // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
                MusECore::PendingOperationList operations;
                operations.add(MusECore::PendingOperationItem(t, val, MusECore::PendingOperationItem::SetTrackRecMonitor));
                MusEGlobal::audio->msgExecutePendingOperations(operations, true);
                }
            }
            else if (button == Qt::RightButton)
            {
                // enable or disable ALL tracks of this type
                // This is a major operation not easily manually undoable. Let's make it undoable.
                MusECore::Undo operations;
                MusECore::TrackList* all_tl = MusEGlobal::song->tracks();
                foreach (MusECore::Track *other_t, *all_tl)
                {
                    if(other_t->type() != t->type())
                        continue;
                    operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackRecMonitor, other_t, val));
                }
                if(!operations.empty())
                {
                    MusEGlobal::song->applyOperationGroup(operations);
                    // Not required if undoable.
                    //MusEGlobal::song->update(SC_TRACK_REC_MONITOR);
                }
            }
            break;
        }

        case COL_RECORD:
        {
            if (!t->canRecord())
                break;

            bool val = !(t->recordFlag());

            if (button == Qt::LeftButton) {
                if (t->type() == MusECore::Track::AUDIO_OUTPUT)
                {
                    if (val && !t->recordFlag())
                        MusEGlobal::muse->bounceToFile((MusECore::AudioOutput*)t);

                    break;
                }

                // apply to selected tracks
                if (t->selected() && tracks->countSelected() > 1) {
                    MusECore::Undo operations;
                    MusECore::TrackList* tl = MusEGlobal::song->tracks();
                    for (const auto tit : *tl)
                    {
                        if (tit->selected() && tit->canRecord() && tit->type() != MusECore::Track::AUDIO_OUTPUT)
                            MusEGlobal::song->setRecordFlag(tit, val, &operations);
                    }
                    if (!operations.empty())
                        MusEGlobal::song->applyOperationGroup(operations);
                }
                else {
                    // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
                    if (!t->setRecordFlag1(val))
                        break;
                    MusECore::PendingOperationList operations;
                    operations.add(MusECore::PendingOperationItem(t, val, MusECore::PendingOperationItem::SetTrackRecord));
                    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
                }
            }
            else if (button == Qt::RightButton)
            {
                // enable or disable ALL tracks of this type
                // This is a major operation not easily manually undoable. Let's make it undoable.
                MusECore::Undo operations;
                if (!t->isMidiTrack()) {
                    if (t->type() == MusECore::Track::AUDIO_OUTPUT) {
                        return;
                    }
                    MusECore::WaveTrackList* wtl = MusEGlobal::song->waves();
                    foreach (MusECore::WaveTrack *wt, *wtl) {
                        MusEGlobal::song->setRecordFlag(wt, val, &operations);
                    }
                }
                else {
                    MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();
                    foreach (MusECore::MidiTrack *mt, *mtl) {
                        if (mt->type() == t->type())
                            MusEGlobal::song->setRecordFlag(mt, val, &operations);
                    }
                }
                if (!operations.empty())
                {
                    MusEGlobal::song->applyOperationGroup(operations);
                    // Not required if undoable.
                    //MusEGlobal::song->update(SC_RECFLAG | SC_TRACK_REC_MONITOR);
                }
            }
            break;
        }

        case COL_NONE:
            mode = START_DRAG;
            break;

        case COL_CLASS:
        {
            if (button == Qt::RightButton) {
                if (t->isMidiTrack() || t->isSynthTrack())
                    showMidiClassPopupMenu(t, x, t->y() - ypos);
                else if (t->type() == MusECore::Track::AUDIO_OUTPUT)
                    showAudioOutPopupMenu(t, x, t->y() - ypos);
            }

            break;
        }

        case COL_OPORT:
        {
            if (button == Qt::RightButton) {
                if (t->isSynthTrack())
                    showMidiClassPopupMenu(t, x, t->y() - ypos);
                else if (t->isMidiTrack())
                    MusEGui::midiPortsPopupMenu(t, x, t->y() - ypos, ctrl, this);
            }

            break;
        }

        case COL_MUTE:
        {
            bool turnOff = (button == Qt::RightButton) || shift;
            bool state = turnOff ? !t->off() : !t->mute();

            if (((t->selected() && tracks->countSelected() > 1) || ctrl) && t->type() != MusECore::Track::AUDIO_OUTPUT)
            {
                // These are major operations not easily manually undoable. Let's make them undoable.
                MusECore::Undo operations;
                if (t->selected() && tracks->countSelected() > 1) // toggle all selected tracks
                {
                    for (const auto it : *tracks) {
                        if (it->selected() && it->type() != MusECore::Track::AUDIO_OUTPUT)
                            setMute(operations, it, turnOff, state);
                    }
                }
                else if (ctrl) // toggle ALL tracks
                {
                    for (const auto it : *tracks) {
                        if (it->type() != MusECore::Track::AUDIO_OUTPUT)
                            setMute(operations, it, turnOff, state);
                    }
                }
                if (!operations.empty())
                {
                    MusEGlobal::song->applyOperationGroup(operations);
                    // Not required if undoable.
                    //MusEGlobal::song->update(SC_MUTE);
                }
            }
            else { // toggle the clicked track
                // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
                MusECore::PendingOperationList operations;
                if (turnOff)
                    operations.add(MusECore::PendingOperationItem(t, !t->off(), MusECore::PendingOperationItem::SetTrackOff));
                else if (t->off())
                    operations.add(MusECore::PendingOperationItem(t, false, MusECore::PendingOperationItem::SetTrackOff));
                else
                    operations.add(MusECore::PendingOperationItem(t, !t->mute(), MusECore::PendingOperationItem::SetTrackMute));

                MusEGlobal::audio->msgExecutePendingOperations(operations, true);
            }

            break;
        }

        case COL_SOLO:
        {
            bool state = !t->solo();

            if (((t->selected() && tracks->countSelected() > 1) || ctrl) && t->type() != MusECore::Track::AUDIO_OUTPUT)
            {
                // These are major operations not easily manually undoable. Let's make them undoable.
                MusECore::Undo operations;
                if (t->selected() && tracks->countSelected() > 1) // toggle all selected tracks
                {
                    for (const auto it : *tracks) {
                        if (it->selected() && it->type() != MusECore::Track::AUDIO_OUTPUT)
                            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackSolo, it, state));
                    }
                }
                else if (ctrl) // toggle ALL tracks
                {
                    for (const auto it : *tracks) {
                        if (it->type() != MusECore::Track::AUDIO_OUTPUT)
                            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackSolo, it, state));
                    }
                }
                if(!operations.empty())
                {
                    MusEGlobal::song->applyOperationGroup(operations);
                    // Not required if undoable.
                    //MusEGlobal::song->update(SC_SOLO);
                }
            }
            else // toggle the clicked track
            {
                // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
                MusECore::PendingOperationList operations;
                operations.add(MusECore::PendingOperationItem(t, state, MusECore::PendingOperationItem::SetTrackSolo));
                MusEGlobal::audio->msgExecutePendingOperations(operations, true);
            }
            break;
        }

        case COL_NAME:
        {
            if (button == Qt::RightButton) {
                mode = NORMAL;
                QMenu* p = new QMenu;
                // Leave room for normal track IDs - base these at AUDIO_SOFTSYNTH.
                int selCnt = MusEGlobal::song->countSelectedTracks();

                QAction *a;
                a = p->addAction(*duplSelTrackSVGIcon, tr("Duplicate Track"));
                a->setData(1005);
                a = p->addAction(*minusSVGIcon, tr("Delete Track"));
                a->setData(1001);

                if (selCnt > 1){
                    p->addSeparator();
                    a = p->addAction(*duplSelTracksSVGIcon, tr("Duplicate Selected"));
                    a->setData(1004);
                    a->setShortcut(shortcuts[SHRT_DUPLICATE_TRACK].key);
                    a = p->addAction(*delSelTracksSVGIcon, tr("Delete Selected"));
                    a->setData(1003);
                }

                if (selCnt > 0) {
                    p->addSeparator();
                    a = p->addAction(tr("Move Selected Up"));
                    a->setData(1006);
                    a->setShortcut(shortcuts[SHRT_MOVEUP_TRACK].key);
                    a = p->addAction(tr("Move Selected Down"));
                    a->setData(1007);
                    a->setShortcut(shortcuts[SHRT_MOVEDOWN_TRACK].key);
                    a = p->addAction(tr("Move Selected to Top"));
                    a->setData(1008);
                    a->setShortcut(shortcuts[SHRT_MOVETOP_TRACK].key);
                    a = p->addAction(*dummySVGIcon, tr("Move Selected to Bottom"));
                    a->setData(1009);
                    a->setShortcut(shortcuts[SHRT_MOVEBOTTOM_TRACK].key);
                }

                p->addSeparator();
                a = p->addAction(tr("Track Comment..."));
                a->setData(1002);

                p->addSeparator();
                a = p->addAction(tr("Set Track Color..."));
                a->setData(1020);
                a = p->addAction(tr("Reset Track Color to Default"));
                a->setData(1021);
                p->addSeparator();

                if (t->type()==MusECore::Track::DRUM)
                {
                    a=p->addAction(tr("Save Track's Drumlist"));
                    a->setData(1010);
                    a->setEnabled(!static_cast<MusECore::MidiTrack*>(t)->workingDrumMap()->empty());
                    a=p->addAction(tr("Load Track's Drumlist"));
                    a->setData(1012);
                    a=p->addAction(tr("Reset Track's Drumlist"));
                    a->setData(1013);
                    a->setEnabled(!static_cast<MusECore::MidiTrack*>(t)->workingDrumMap()->empty());
                    a=p->addAction(tr("Reset Track's Drumlist Ordering"));
                    a->setData(1016);
                    a->setEnabled(!((MusECore::MidiTrack*)t)->drummap_ordering_tied_to_patch());
                    a=p->addAction(tr("Copy Track's Drumlist to All Selected Tracks"));
                    a->setData(1014);
                    a->setEnabled(!static_cast<MusECore::MidiTrack*>(t)->workingDrumMap()->empty());
                    // 1016 is occupied.
                    p->addSeparator();
                }
                insertTrackMenu->setTitle(tr("Insert Track"));
                p->addMenu(insertTrackMenu);

                QAction* act = p->exec(ev->globalPos(), nullptr);
                if (act) {
                    //fprintf(stderr, "TList::mousePressEvent act:%p\n", act);
                    int n = act->data().toInt();
                    if(n >= 1000 && n < MENU_ADD_SYNTH_ID_BASE)
                    {
                        //fprintf(stderr, "   n:%d\n", n);
                        switch (n) {
                        case 1001:     // delete track
                            MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(t), t));
                            break;
                        case 1003:     // delete track(s)
                            MusEGlobal::audio->msgRemoveTracks();
                            break;
                        case 1002:     // show track comment
                        {
                            TrackComment* tc = new TrackComment(t, nullptr);
                            tc->show();
                        }
                            break;
                        case 1004:
                            MusEGlobal::song->duplicateTracks();
                            break;
                        case 1005:
                            MusEGlobal::song->duplicateTracks(t);
                            break;
                        case 1006:
                            moveSelectedTracks(true, false);
                            break;
                        case 1007:
                            moveSelectedTracks(false, false);
                            break;
                        case 1008:
                            moveSelectedTracks(true, true);
                            break;
                        case 1009:
                            moveSelectedTracks(false, true);
                            break;
                        case 1020:
                        {
                            QColor c = QColorDialog::getColor(t->color());
                            if (c.isValid()) {
                                if (t->selected())
                                    for (const auto& it : *tracks) {
                                        if (it->selected())
                                            it->setColor(c);
                                    }
                                else
                                    t->setColor(c);
                                MusEGlobal::song->update(SC_TRACK_MODIFIED);
                            }
                        }
                            break;
                        case 1021:
                        {
                            if (t->selected())
                                for (const auto& it : *tracks) {
                                    if (it->selected())
                                        it->resetColor();
                                }
                            else
                                t->resetColor();
                            MusEGlobal::song->update(SC_TRACK_MODIFIED);
                        }
                            break;

                        case 1010:
                            saveTrackDrummap((MusECore::MidiTrack*)t, true);
                            break;

                        case 1012:
                            loadTrackDrummap((MusECore::MidiTrack*)t);
                            break;

                        case 1013:
                            if (QMessageBox::warning(this, tr("Drum map"),
                                                     tr("Reset the track's drum map with instrument defaults?"),
                                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
                            {
                                // The allocated WorkingDrumMapPatchList wdmpl will become the new list and the
                                //  original lists will be deleted, in the operation following.
                                MusECore::PendingOperationList operations;
                                // Completely blank replacement list.
                                MusECore::WorkingDrumMapPatchList* new_wdmpl = new MusECore::WorkingDrumMapPatchList();
                                MusECore::DrumMapTrackPatchReplaceOperation* dmop = new MusECore::DrumMapTrackPatchReplaceOperation;
                                dmop->_isInstrumentMod = false; // Not instrument operation.
                                dmop->_workingItemPatchList = new_wdmpl;
                                dmop->_track = static_cast<MusECore::MidiTrack*>(t);
                                operations.add(MusECore::PendingOperationItem(dmop, MusECore::PendingOperationItem::ReplaceTrackDrumMapPatchList));
                                MusEGlobal::audio->msgExecutePendingOperations(operations, true);
                            }
                            break;

                        case 1016:
                            if (QMessageBox::warning(this, tr("Drum map"),
                                                     tr("Reset the track's drum map ordering?"),
                                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
                            {
                                ((MusECore::MidiTrack*)t)->set_drummap_ordering_tied_to_patch(true);
                                MusEGlobal::song->update(SC_DRUMMAP);
                            }
                            break;

                        case 1014:
                            copyTrackDrummap((MusECore::MidiTrack*)t, true);
                            break;

                        default:
                            printf("action %d\n", n);
                            break;
                        }
                    }
                    else
                    {
                        t = MusEGlobal::song->addNewTrack(act, t);  // Let addNewTrack handle it. Insert before clicked-on track 't'.
                        //fprintf(stderr, "   addNewTrack: track:%p\n", t);
                        if(t)
                        {
                            MusEGlobal::song->selectAllTracks(false);
                            t->setSelected(true);
                            MusEGlobal::song->update(SC_TRACK_SELECTION);
                            adjustScrollbar();
                        }
                    }
                }
                delete p;
            }
            break;
        }

//        case COL_TIMELOCK:
//        {
//            if(!t->isMidiTrack())
//            {
//                mode = START_DRAG;  // Allow a track drag to start.
//                break;
//            }
//            t->setLocked(!t->locked());

//            break;
//        }

        case COL_OCHANNEL:
        {
            int delta = 0;
            if (button == Qt::MidButton || (button == Qt::RightButton && shift))
                delta = -1;
            else if (button == Qt::RightButton)
                delta = 1;

            setTrackChannel(t, true, 0, delta, ctrl);

            break;
        }

        default:
        {
            mode = START_DRAG;
            if (col>=COL_CUSTOM_MIDICTRL_OFFSET && t->isMidiTrack())
            {
                if (Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                        Arranger::custom_col_t::AFFECT_BEGIN)
                    ctrl_at_tick=0;
                else
                    ctrl_at_tick=MusEGlobal::song->cpos();

                int delta = 0;
                if (button == Qt::RightButton)
                    delta = 1;
                else if (button == Qt::MidButton)
                    delta = -1;

                if (delta!=0)
                {
                    MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(t);
                    if (mt == nullptr)
                        break;

                    int ctrl_num = Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;

                    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                    const int chan = mt->outChannel();
                    MusECore::MidiController* mctl = mp->midiController(ctrl_num, chan);

                    int minval=mctl->minVal()+mctl->bias();
                    int maxval=mctl->maxVal()+mctl->bias();

                    int val = mt->getControllerChangeAtTick(0,ctrl_num);
                    int oldval=val;

                    if (ctrl_num!=MusECore::CTRL_PROGRAM)
                    {
                        val += delta;
                        if(val > maxval)
                            val = maxval;
                        if(val < minval-1) // "-1" because of "off"
                            val = minval-1;
                    }
                    else
                    {
                        MusECore::MidiInstrument* instr = mp->instrument();
                        if (delta>0) val=instr->getNextPatch(mt->outChannel(), val, false);
                        else if (delta<0) val=instr->getPrevPatch(mt->outChannel(), val, false);
                    }

                    if (val != oldval)
                    {
                        if (val!=minval-1)
                        {
                            record_controller_change_and_maybe_send(ctrl_at_tick, ctrl_num, val, mt);
                        }
                        else
                        {
                            MusECore::Undo operations;
                            for (MusECore::iPart p = mt->parts()->begin(); p!=mt->parts()->end(); p++)
                            {
                                if (p->second->tick()==0)
                                {
                                    for (MusECore::ciEvent ev=p->second->events().begin(); ev!=p->second->events().end(); ev++)
                                    {
                                        if (ev->second.tick()!=0) break;
                                        else if (ev->second.type()==MusECore::Controller && ev->second.dataA()==ctrl_num)
                                        {
                                            using MusECore::UndoOp;
                                            operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, p->second, false, false));
                                            break;
                                        }
                                    }
                                }
                            }
                            MusEGlobal::song->applyOperationGroup(operations);
                        }
                    }
                }
                else // if (delta==0)
                {
                    ctrl_num=Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;

                    if (ctrl_num==MusECore::CTRL_PROGRAM)
                    {
                        editTrack=t;

                        MusECore::MidiTrack* mt=(MusECore::MidiTrack*)t;
                        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                        MusECore::MidiInstrument* instr = mp->instrument();

                        PopupMenu* pup = new PopupMenu(true);
                        instr->populatePatchPopup(pup, mt->outChannel(), mt->isDrumTrack());

                        if(pup->actions().count() == 0)
                        {
                            delete pup;
                            return;
                        }

                        connect(pup, SIGNAL(triggered(QAction*)), SLOT(instrPopupActivated(QAction*)));

                        QAction *act = pup->exec(ev->globalPos());
                        if(act)
                        {
                            int val = act->data().toInt();
                            if(val != -1)
                                record_controller_change_and_maybe_send(ctrl_at_tick, MusECore::CTRL_PROGRAM, val, mt);
                        }

                        delete pup;
                    }
                }
            }
        }
        } //end of "switch"

    }

    redraw();
}

void TList::addAutoMenuAction(PopupMenu* p, const MusECore::CtrlList *cl) {
    QAction *act = p->addAction(cl->name());
    act->setCheckable(true);
    act->setChecked(cl->isVisible());

    QPixmap pix(10, 10);
    QPainter qp(&pix);
    qp.fillRect(0,0,10,10, cl->color());
    if (cl->size() > 0) {
        if (cl->color() == Qt::black)
            qp.fillRect(2, 2, 6, 6, Qt::gray);
        else
            qp.fillRect(2, 2, 6, 6, Qt::black);
    }
    QIcon icon(pix);
    act->setIcon(icon);

    int ctrl = cl->id();
    int data = ctrl<<8; // shift 8 bits
    data += 150; // illegal color > 100
    act->setData(data);
    PopupMenu *m = colorMenu(cl->color(), cl->id(), p);
    act->setMenu(m);
}

void TList::outputAutoMenuSorted(PopupMenu* p, QList<const MusECore::CtrlList*> &tmpList) {

    if (!tmpList.isEmpty()) {

        std::sort(tmpList.begin(), tmpList.end(),
                  [](const MusECore::CtrlList* a, const MusECore::CtrlList* b) -> bool { return a->name() < b->name(); });

        for (const auto& it : tmpList)
            addAutoMenuAction(p, it);

        tmpList.clear();
    }
}

    void TList::setMute(MusECore::Undo& operations, MusECore::Track *t, bool turnOff, bool state)
{
    if (turnOff)
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackOff, t, state));
    else if (t->off())
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackOff, t, false));
    else
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SetTrackMute, t, state));
}

void TList::loadTrackDrummap(MusECore::MidiTrack* t, const char* fn_)
{
    QString fn;

    if (fn_==nullptr)
        fn=MusEGui::getOpenFileName("drummaps", MusEGlobal::drum_map_file_pattern,
                                    this, tr("Muse: Load Track's Drum Map"), 0);
    else
        fn=QString(fn_);

    if (fn.isEmpty())
    {
        printf("ERROR: TList::loadTrackDrummap(): empty filename\n");
        return;
    }

    bool popenFlag;
    FILE* f = MusEGui::fileOpen(this, fn, QString(".map"), "r", popenFlag, true);
    if (f == 0)
    {
        printf("ERROR: TList::loadTrackDrummap() could not open file %s!\n", fn.toLatin1().data());
        return;
    }

    MusECore::Xml xml(f);
    loadTrackDrummapFromXML(t, xml);

    if (popenFlag)
        pclose(f);
    else
        fclose(f);

    MusEGlobal::song->update(SC_DRUMMAP);
}

void TList::loadTrackDrummapFromXML(MusECore::MidiTrack *t, MusECore::Xml &xml)
{
    MusECore::PendingOperationList operations;
    MusECore::WorkingDrumMapPatchList* wdmpl = 0;

    for (;;) {
        MusECore::Xml::Token token = xml.parse();
        const QString& tag = xml.s1();
        switch (token) {
        case MusECore::Xml::Error:
        case MusECore::Xml::End:
            if(wdmpl)
                delete wdmpl;
            return;
        case MusECore::Xml::TagStart:
            if (tag == "muse")
            {
            }
            else if (tag == "our_drummap" ||  // OBSOLETE. Support old files.
                     tag == "drummap" ||      // OBSOLETE. Support old files.
                     tag == "drumMapPatch")
            {
                if(!wdmpl)
                    wdmpl = new MusECore::WorkingDrumMapPatchList();
                // false = Do not fill in unused items.
                wdmpl->read(xml, false);
            }

            else
                xml.unknown("TList::loadTrackDrummap");
            break;
        case MusECore::Xml::Attribut:
            break;
        case MusECore::Xml::TagEnd:
            if (tag == "muse")
            {
                if(wdmpl)
                {
                    // The allocated WorkingDrumMapPatchList wdmpl will become the new list and the
                    //  original lists will be deleted, in the operation following.
                    MusECore::DrumMapTrackPatchReplaceOperation* dmop = new MusECore::DrumMapTrackPatchReplaceOperation;
                    dmop->_isInstrumentMod = false; // Not instrument operation.
                    dmop->_workingItemPatchList = wdmpl;
                    dmop->_track = t;

                    operations.add(MusECore::PendingOperationItem(dmop, MusECore::PendingOperationItem::ReplaceTrackDrumMapPatchList));
                    MusEGlobal::audio->msgExecutePendingOperations(operations, true);
                }
                goto ende;
            }
        default:
            break;
        }
    }
ende:
    return;
}

void TList::saveTrackDrummap(MusECore::MidiTrack* t, bool /*full*/, const char* fn_)
{
    QString fn;
    if (fn_==nullptr)
        fn = MusEGui::getSaveFileName(QString("drummaps"), MusEGlobal::drum_map_file_save_pattern,
                                      this, tr("MusE: Store Track's Drum Map"));
    else
        fn = QString(fn_);

    if (fn.isEmpty())
        return;

    bool popenFlag;
    FILE* f = MusEGui::fileOpen(this, fn, QString(".map"), "w", popenFlag, false, true);
    if (f == 0)
        return;

    MusECore::Xml xml(f);
    xml.header();
    xml.tag(0, "muse version=\"1.0\"");

    t->workingDrumMap()->write(1, xml);

    xml.tag(0, "/muse");

    if (popenFlag)
        pclose(f);
    else
        fclose(f);
}

void TList::copyTrackDrummap(MusECore::MidiTrack* t, bool /*full*/)
{
    MusECore::PendingOperationList operations;
    MusECore::WorkingDrumMapPatchList* new_wdmpl;

    MusECore::WorkingDrumMapPatchList* wdmpl = t->workingDrumMap();
    MusECore::MidiTrack* mt;
    for(MusECore::iMidiTrack it = MusEGlobal::song->midis()->begin(); it != MusEGlobal::song->midis()->end(); ++it)
    {
        mt = *it;
        if(mt == t || !mt->selected() || mt->type() != MusECore::Track::DRUM)
            continue;

        // The allocated WorkingDrumMapPatchList wdmpl will become the new list and the
        //  original lists will be deleted, in the operation following.
        new_wdmpl = new MusECore::WorkingDrumMapPatchList();
        *new_wdmpl = *wdmpl;
        MusECore::DrumMapTrackPatchReplaceOperation* dmop = new MusECore::DrumMapTrackPatchReplaceOperation;
        dmop->_isInstrumentMod = false; // Not instrument operation.
        dmop->_workingItemPatchList = new_wdmpl;
        dmop->_track = mt;
        operations.add(MusECore::PendingOperationItem(dmop, MusECore::PendingOperationItem::ReplaceTrackDrumMapPatchList));
    }

    if(!operations.empty())
        MusEGlobal::audio->msgExecutePendingOperations(operations, true);
}

//---------------------------------------------------------
//   selectTrack
//---------------------------------------------------------
void TList::selectTrack(MusECore::Track* tr, bool /*deselect*/)
{
    MusEGlobal::song->selectAllTracks(false);

    if (tr) {
        tr->setSelected(true);

        // rec enable track if expected
        MusECore::TrackList recd = getRecEnabledTracks();
        if (!MusEGlobal::audio->isRecording() &&
                recd.size() == 1 &&
                MusEGlobal::config.moveArmedCheckBox) { // one rec enabled track, move rec enabled with selection
            MusEGlobal::song->setRecordFlag((MusECore::Track*)recd.front(),false);
            MusEGlobal::song->setRecordFlag(tr,true);
        }
    }

    // SC_TRACK_SELECTION will cause update anyway, no harm ...
    update();
    MusEGlobal::song->update(SC_TRACK_SELECTION);
}

//---------------------------------------------------------
//   selectTrackAbove
//---------------------------------------------------------
void TList::selectTrackAbove()
{
    moveSelection(-1);
}
//---------------------------------------------------------
//   selectTrackBelow
//---------------------------------------------------------
void TList::selectTrackBelow()
{
    moveSelection(1);
}

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void TList::mouseMoveEvent(QMouseEvent* ev)
{
    if((editor && (editor->isVisible() || editor->hasFocus())) ||
            (chan_edit && (chan_edit->isVisible() || chan_edit->hasFocus())) ||
            (ctrl_edit && (ctrl_edit->isVisible() || ctrl_edit->hasFocus())))
    {
        ev->accept();
        return;
    }

    if (ev->buttons() == 0) {
//        if ((((QInputEvent*)ev)->modifiers() | ev->buttons()) == 0) {
        int y = ev->y();
        int ty = -ypos;
        MusECore::TrackList* tracks = MusEGlobal::song->tracks();
        MusECore::iTrack it;
        for (it = tracks->begin(); it != tracks->end(); ++it) {
            int h = (*it)->height();
            ty += h;
            if (y >= (ty-2)) {
                if ( (*it) == tracks->back() && y >= ty ) { // DELETETHIS and cleanup
                    // outside last track don't change to splitVCursor
                }
                else if ( y > (ty+2) ) {
                    //printf(" y > (ty+2) \n");
                }
                else {
                    if (!resizeFlag) {
                        resizeFlag = true;
                        setCursor(QCursor(Qt::SplitVCursor));
                        MusEGlobal::muse->setStatusBarText(tr("Draw to change the track height. Hold CTRL for all tracks, SHIFT for selected tracks."));
                    }
                    break;
                }
            }
        }
        if (it == tracks->end() && resizeFlag) {
            setCursor(QCursor(Qt::ArrowCursor));
            resizeFlag = false;
            MusEGlobal::muse->clearStatusBarText();
        }
        return;
    }

    curY      = ev->y();
    int delta = curY - startY;
    switch (mode) {
    case START_DRAG:
        if (delta < 0)
            delta = -delta;
        if (delta <= 2)
            break;
    {
        MusECore::Track* t = y2Track(startY + ypos);
        if (t == nullptr)
            mode = NORMAL;
        else {
            mode = DRAG;
            dragHeight = t->height();
            sTrack     = MusEGlobal::song->tracks()->index(t);
            setCursor(QCursor(Qt::SizeVerCursor));
            redraw();
        }
    }
        break;
    case NORMAL:
        break;
    case DRAG:
        redraw();
        break;
    case RESIZE:
    {
        if (sTrack >= 0 && (unsigned) sTrack < MusEGlobal::song->tracks()->size())
        {
            bool shift = ((QInputEvent*)ev)->modifiers() & Qt::SHIFT;
            bool ctrl = ((QInputEvent*)ev)->modifiers() & Qt::CTRL;
            if (ctrl | shift) {
                bool done = false;
                for (const auto& it : *MusEGlobal::song->tracks()) {
                    if (shift && !it->selected())
                        continue;
                    int h  = it->height() + delta;
                    h = qMax(h, MIN_TRACKHEIGHT);
                    it->setHeight(h);
                    done = true;
                }
                if (done) {
                    startY = curY;
                    update();
                    MusEGlobal::song->update(SC_TRACK_RESIZED);
                }
            }
            else {
                MusECore::Track* t = MusEGlobal::song->tracks()->index(sTrack);
                if (t) {
                    int h  = t->height() + delta;
                    startY = curY;
                    if (h < MIN_TRACKHEIGHT)
                        h = MIN_TRACKHEIGHT;
                    t->setHeight(h);
                    update();
                    MusEGlobal::song->update(SC_TRACK_RESIZED);
                }
            }
        }
    }
        break;
    }
}

void TList::moveSelectedTracks(bool up, bool full) {

    // Move only visible? But the other track functions also work on hidden tracks
    // (duplicate, delete...), so this seems more consistent (kybos)

    MusECore::TrackList *tracks = MusEGlobal::song->tracks();

    if (tracks->size() < 2 || tracks->countSelected() == 0
            || (!up && tracks->back()->selected())
            || (up && tracks->front()->selected()))
        return;

    if (MusEGlobal::audio->isPlaying()) {
        MusEGlobal::muse->setStatusBarText(tr("Operation not available while playing"), 5000);
        return;
    }

    MusECore::TrackList tracksTmp = *tracks;

    unsigned delta = 1;
    bool deltaFound = false;

    if (up) {
        for (const auto it : *tracks) {
            if (!it->selected())
                continue;

            if (full & !deltaFound) {
                delta = static_cast<unsigned>(tracks->index(it));
                deltaFound = true;
            }

            unsigned sidx = static_cast<unsigned>(tracks->index(it));
            unsigned i = delta;
            while (i--) {
                std::swap(tracksTmp[sidx], tracksTmp[sidx-1]);
                sidx--;
            }
        }
    } else {
        for (auto it = tracks->rbegin(); it != tracks->rend(); it++) {
            if (!(*it)->selected())
                continue;

            unsigned sidx = static_cast<unsigned>(tracks->index(*it));

            if (full && !deltaFound) {
                delta = static_cast<unsigned>(tracks->size()) - sidx - 1;
                deltaFound = true;
            }

            unsigned i = delta;
            while (i--) {
                std::swap(tracksTmp[sidx], tracksTmp[sidx+1]);
                sidx++;
            }
        }
    }

    MusEGlobal::song->tracks()->swap(tracksTmp);
    MusEGlobal::song->update(SC_TRACK_MOVED);
}

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void TList::mouseReleaseEvent(QMouseEvent* ev)
{
    if((editor && (editor->isVisible() || editor->hasFocus())) ||
            (chan_edit && (chan_edit->isVisible() || chan_edit->hasFocus())) ||
            (ctrl_edit && (ctrl_edit->isVisible() || ctrl_edit->hasFocus())))
    {
        ev->accept();
        return;
    }

    if (mode == DRAG) {
        MusECore::Track* t = y2Track(ev->y() + ypos);
        if (t) {
            int dTrack = MusEGlobal::song->tracks()->index(t);
            if (sTrack >= 0 && dTrack >= 0)   // sanity check
            {
                const int tracks_sz = MusEGlobal::song->tracks()->size();
                if (sTrack < tracks_sz && dTrack < tracks_sz)   // sanity check
                    MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::MoveTrack, sTrack, dTrack));
            }

            // inconsistent, better keep the original aux knob order?
            // - if aux track is moved in the mixer, the aux knobs are also NOT adjusted
            // - if the drag action is undone, the aux knobs remain in the wrong order

//            MusECore::TrackList *tracks = MusEGlobal::song->tracks();
//            if ( tracks->at(dTrack)->type() == MusECore::Track::AUDIO_AUX) {

//                MusECore::AuxList auxCopy; // = *MusEGlobal::song->auxs();
//                //MusEGlobal::song->auxs()->clear();
//                std::vector<int> oldAuxIndex;

//                for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
//                    if ((*t)->type() == MusECore::Track::AUDIO_AUX) {
//                        MusECore::AudioAux *ax = (MusECore::AudioAux*)*t;
//                        auxCopy.push_back(ax);
//                        oldAuxIndex.push_back(MusEGlobal::song->auxs()->index(ax)); // store old index
//                    }
//                }
//                // loop through all tracks and set the levels for all tracks
//                for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
//                    MusECore::AudioTrack *trk = (MusECore::AudioTrack*)*t;

//                    if (!trk->isMidiTrack() && trk->hasAuxSend())
//                    {
//                        std::vector<double> oldAuxValue;
//                        for (unsigned i = 0 ; i < auxCopy.size(); i++)
//                            oldAuxValue.push_back(trk->auxSend(i));
//                        for (unsigned i = 0 ; i < auxCopy.size(); i++)
//                            trk->setAuxSend(i, oldAuxValue[oldAuxIndex[i]] );
//                    }
//                    MusEGlobal::song->auxs()->clear();
//                    for (MusECore::iAudioAux t = auxCopy.begin(); t != auxCopy.end(); ++t) {
//                        MusEGlobal::song->auxs()->push_back(*t);
//                    }
//                }

//                MusEGlobal::song->update(SC_EVERYTHING);

//            }
        }
    }
    if (mode != NORMAL) {
        mode = NORMAL;
        setCursor(QCursor(Qt::ArrowCursor));
        redraw();
    }
    if (editTrack && editor && editor->isVisible())
        editor->setFocus();
    //else // DELETETHIS or add the same for ctrl_edit!
    //if (editTrack && chan_edit && chan_edit->isVisible())  // p4.0.46 DELETETHIS?
    //      chan_edit->setFocus();
    adjustScrollbar();
}

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void TList::wheelEvent(QWheelEvent* ev)
{
    emit redirectWheelEvent(ev);
}


QSize TList::sizeHint() const { return QSize(250, 100); }
QSize TList::minimumSizeHint() const { return QSize(100, 100); }

//---------------------------------------------------------
//   setYPos
//---------------------------------------------------------

void TList::setYPos(int y)
{
    int delta  = ypos - y;         // -  -> shift up
    ypos  = y;

    scroll(0, delta);
}

void TList::changeTrackToType(MusECore::Track *t, MusECore::Track::TrackType trackType)
{
    // MIDI -> NEW_DRUM or vice versa. added by flo.
    MusEGlobal::audio->msgIdle(true);
    t->setType(trackType);
    MusEGlobal::audio->msgIdle(false);
    MusEGlobal::song->update(SC_TRACK_MODIFIED);
}

void TList::instrPopupActivated(QAction* act)
{
    MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editTrack);
    if(act && mt)
    {
        int val = act->data().toInt();
        if(val != -1)
            record_controller_change_and_maybe_send(ctrl_at_tick, MusECore::CTRL_PROGRAM, val, mt);
    }
}


void TList::setHeader(Header* h)
{
    header=h;
    redraw();
}

void TList::populateAddTrack()
{
    addTrackMenu = new QMenu;
    MusEGui::populateAddTrack(addTrackMenu, false, false, true);

    insertTrackMenu = new QMenu;
    MusEGui::populateAddTrack(insertTrackMenu, false, true);
}

} // namespace MusEGui

