//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: icons.cpp,v 1.13.2.8 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <QPixmap>
#include <QIcon>
#include <QCursor>
#include <QDir>
#include <QApplication>

#include "xpm/midi_ctrl_graph_merge_erase.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_inclusive.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_wysiwyg.xpm"

#include "xpm/muse_icon.xpm"
#include "xpm/about_muse.xpm"

#include "xpm/global.xpm"
#include "xpm/project.xpm"
#include "xpm/user.xpm"

#include "xpm/pianoNew.xpm"
#include "xpm/presetsNew.xpm"

//#include "icons.h"


namespace MusEGui {

QPixmap* midiCtrlMergeEraseIcon;
QPixmap* midiCtrlMergeEraseInclusiveIcon;
QPixmap* midiCtrlMergeEraseWysiwygIcon;

QPixmap* museIcon;
QPixmap* aboutMuseImage;

QIcon* globalIcon;
QIcon* projectIcon;
QIcon* userIcon;

QIcon* pianoNewIcon;
QIcon* presetsNewIcon;

//----------------------------------
//   SVG...
//----------------------------------

QIcon* dropDownTriangleSVGIcon;
QIcon* expandLeftRightSVGIcon;

QIcon* routingInputSVGIcon;
QIcon* routingOutputSVGIcon;
QIcon* routingInputUnconnectedSVGIcon;
QIcon* routingOutputUnconnectedSVGIcon;

QIcon* headphonesOffSVGIcon;
QIcon* headphonesOnSVGIcon;

QIcon* muteOffSVGIcon;
QIcon* muteOnSVGIcon;
QIcon* muteOnXSVGIcon;
QIcon* muteProxyOnSVGIcon;
QIcon* muteAndProxyOnSVGIcon;
QIcon* muteStateSVGIcon;

QIcon* soloOffSVGIcon;
QIcon* soloOnSVGIcon;
QIcon* soloOnAloneSVGIcon;
QIcon* soloProxyOnSVGIcon;
QIcon* soloProxyOnAloneSVGIcon;
QIcon* soloAndProxyOnSVGIcon;
QIcon* soloStateSVGIcon;

QIcon* trackOffSVGIcon;
QIcon* trackOnSVGIcon;

QIcon* stereoOffSVGIcon;
QIcon* stereoOnSVGIcon;

QIcon* preFaderOffSVGIcon;
QIcon* preFaderOnSVGIcon;

QIcon* recArmOffSVGIcon;
QIcon* recArmOnSVGIcon;
QIcon* recArmStateSVGIcon;

QIcon* monitorOffSVGIcon;
QIcon* monitorOnSVGIcon;
QIcon* monitorStateSVGIcon;

QIcon* velocityPerNoteSVGIcon;
QIcon* midiControllerNewSVGIcon;
QIcon* midiControllerSelectSVGIcon;
QIcon* midiControllerRemoveSVGIcon;

//QIcon* soloSVGIcon;
//QIcon* soloProxySVGIcon;
QIcon* muteSVGIcon;
//QIcon* trackEnableSVGIcon;
//QIcon* recArmSVGIcon;
QIcon* recMasterSVGIcon;

QIcon* stopSVGIcon;
QIcon* playSVGIcon;
QIcon* fastForwardSVGIcon;
QIcon* rewindSVGIcon;
QIcon* rewindToStartSVGIcon;

QIcon* externSyncOffSVGIcon;
QIcon* externSyncOnSVGIcon;

QIcon* masterTrackOffSVGIcon;
QIcon* masterTrackOnSVGIcon;

QIcon* jackTransportOffSVGIcon;
QIcon* jackTransportOnSVGIcon;

QIcon* timebaseMasterOffSVGIcon;
QIcon* timebaseMasterOnSVGIcon;

QIcon* metronomeOffSVGIcon;
QIcon* metronomeOnSVGIcon;

QIcon* fixedSpeedSVGIcon;
QIcon* transportAffectsLatencySVGIcon;
QIcon* overrideLatencySVGIcon;

QIcon* panicSVGIcon;
QIcon* loopSVGIcon;
QIcon* punchinSVGIcon;
QIcon* punchoutSVGIcon;
QIcon* undoSVGIcon;
QIcon* redoSVGIcon;
QIcon* midiInSVGIcon;
QIcon* steprecSVGIcon;
QIcon* speakerSVGIcon;
QIcon* speakerSingleNoteSVGIcon;
QIcon* speakerChordsSVGIcon;
QIcon* filenewSVGIcon;
QIcon* filetemplateSVGIcon;
QIcon* fileopenSVGIcon;
QIcon* filesaveSVGIcon;
QIcon* filesaveasSVGIcon;
QIcon* filesaveProjectSVGIcon;
QIcon* filesaveTemplateSVGIcon;
QIcon* filesaveRevisionSVGIcon;
QIcon* filecloseSVGIcon;
QIcon* appexitSVGIcon;
QIcon* whatsthisSVGIcon;
QIcon* infoSVGIcon;
QIcon* showFieldsSVGIcon;
QIcon* exitSVGIcon;
QIcon* noteSVGIcon;
QIcon* metaSVGIcon;
QIcon* ctrlSVGIcon;
QIcon* sysexSVGIcon;
QIcon* tracktypeSVGIcon;
QIcon* pianorollSVGIcon;
QIcon* arrangerSVGIcon;
QIcon* waveeditorSVGIcon;
QIcon* scoreeditSVGIcon;
QIcon* mastereditSVGIcon;
QIcon* drumeditSVGIcon;
QIcon* listeditSVGIcon;
QIcon* synthSVGIcon;
QIcon* trackInputSVGIcon;
QIcon* trackOutputSVGIcon;
QIcon* trackAuxSVGIcon;
QIcon* trackGroupVGIcon;

// tool icons
QIcon* pencilIconSVG;
QIcon* glueIconSVG;
QIcon* cutterIconSVG;
QIcon* zoomIconSVG;
QIcon* zoomAtIconSVG;
QIcon* deleteIconSVG;
QIcon* drawIconSVG;
QIcon* pointerIconSVG;
QIcon* mutePartsIconSVG;
QIcon* handIconSVG;
QIcon* closedHandIconSVG;
QIcon* cursorIconSVG;
//QIcon* magnetIconSVG;
//QIcon* customMoveIconSVG;
QIcon* pencilMove4WayIconSVG;
QIcon* pencilMoveHorizIconSVG;
QIcon* pencilMoveVertIconSVG;
QIcon* audioStretchIconSVG;
QIcon* audioResampleIconSVG;

QIcon* noscaleSVGIcon[3];
QIcon* ankerSVGIcon;
QIcon* settingsSVGIcon;
QIcon* transportSVGIcon;
QIcon* bigtimeSVGIcon;
QIcon* mixerSVGIcon;
QIcon* plusSVGIcon;
QIcon* minusSVGIcon;
QIcon* keySVGIcon;
QIcon* delSelTracksSVGIcon;
QIcon* duplSelTrackSVGIcon;
QIcon* duplSelTracksSVGIcon;

QIcon* routeAutoAdjustSVGIcon;
QIcon* routeSelSourceSVGIcon;
QIcon* routeSelDestSVGIcon;
QIcon* routeSourceSVGIcon;
QIcon* routeDestSVGIcon;

QIcon* gridOnSVGIcon;
QIcon* rangeToSelectionSVGIcon;
QIcon* quantizeSVGIcon;
QIcon* clearSVGIcon;
QIcon* downmixOffSVGIcon;
QIcon* downmixOnSVGIcon;
QIcon* downmixTrackSVGIcon;
QIcon* downmixStateSVGIcon;
QIcon* restartSVGIcon;
QIcon* snapshotSVGIcon;
QIcon* emptyBarSVGIcon;
QIcon* lockSVGIcon;

QIcon* midiResetSVGIcon;
QIcon* midiInitSVGIcon;
QIcon* midiLocalOffSVGIcon;
QIcon* midiTransformSVGIcon;
QIcon* midiInputTransformSVGIcon;
QIcon* midiInputTransposeSVGIcon;
QIcon* midiInputFilterSVGIcon;
QIcon* midiInputRemoteSVGIcon;
QIcon* midiSyncSVGIcon;
QIcon* midiExportImportSVGIcon;

QIcon* cutSVGIcon;
QIcon* copySVGIcon;
QIcon* pasteSVGIcon;
QIcon* pasteDialogSVGIcon;
QIcon* pasteCloneSVGIcon;
QIcon* copyRangeSVGIcon;
QIcon* deleteSVGIcon;
QIcon* pasteSelectedTrackSVGIcon;
QIcon* pasteCloneSelectedTrackSVGIcon;

QIcon* eyeSVGIcon;
QIcon* eyeCrossedSVGIcon;
QIcon* eyeGreySVGIcon;

QIcon* nextPartSVGIcon;
QIcon* lastPartSVGIcon;

QIcon* selectAllSVGIcon;
QIcon* selectAllTrackSVGIcon;
QIcon* selectInsideLoopSVGIcon;
QIcon* selectOutsideLoopSVGIcon;
QIcon* selectInvertSVGIcon;
QIcon* deselectAllSVGIcon;

QIcon* ledGreenSVGIcon;
QIcon* ledGreenDarkSVGIcon;
QIcon* ledRedSVGIcon;
QIcon* ledBlueSVGIcon;
QIcon* ledYellowSVGIcon;
QIcon* ledOffSVGIcon;

QIcon* routeInSVGIcon;
QIcon* routeOutSVGIcon;
QIcon* routeInMidiSVGIcon;
QIcon* routeOutMidiSVGIcon;
QIcon* midiPortSVGIcon;
QIcon* routerSVGIcon;

QIcon* appearanceSVGIcon;
QIcon* editInstrumentSVGIcon;
QIcon* deltaSVGIcon;

QIcon* dummySVGIcon;

//----------------------------------
// Cursors
//----------------------------------

QCursor* editpasteSCursor;
QCursor* editpasteCloneSCursor;

QCursor* pencilCursor;
QCursor* glueCursor;
QCursor* cutterCursor;
QCursor* zoomCursor;
QCursor* zoomAtCursor;
QCursor* deleteCursor;
QCursor* drawCursor;
QCursor* mutePartsCursor;
QCursor* handCursor;
QCursor* closedHandCursor;
//QCursor* magnetCursor;
//QCursor* customMoveCursor;
QCursor* pencilMove4WayCursor;
QCursor* pencilMoveHorizCursor;
QCursor* pencilMoveVertCursor;


//---------------------------------------------------------
//   class Icons
//---------------------------------------------------------

class Icons {
    QStringList _global, _user;
    QString _path_global, _path_user;
    bool _global_on, _user_on;

public:
    Icons(const QString & path_global, const QString & path_user)
        : _path_global(path_global),
          _path_user(path_user)
    {
        QDir dir(path_global, "*.svg");
        _global = dir.entryList(QDir::Files);
        _global_on = !_global.isEmpty();

        dir.setPath(path_user);
        _user = dir.entryList(QDir::Files);
        _user_on = !_user.isEmpty();
    }

    QIcon* getSVG(const QString & filename)
    {
        if (_user_on && _user.contains(filename))
            return new QIcon(_path_user + "/" + filename);

        if (_global_on && _global.contains(filename))
            return new QIcon(_path_global + "/" + filename);

        return new QIcon(":/svg/" + filename);
    }

    void addSVG(QIcon *icon, const QString & filename,
                  QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::On)
    {
        if (!icon)
            return;

        if (_user_on && _user.contains(filename)) {
            icon->addFile(_path_user + "/" + filename, QSize(), mode, state);
            return;
        }

        if (_global_on && _global.contains(filename)) {
            icon->addFile(_path_global + "/" + filename, QSize(), mode, state);
            return;
        }

        icon->addFile(":/svg/" + filename, QSize(), mode, state);
    }
};


//---------------------------------------------------------
//   initIcons
//---------------------------------------------------------

void initIcons(int cursorSize, const QString& gpath, const QString& upath)
{
    const qreal dpr = qApp->devicePixelRatio();

    Icons icons(gpath, upath);

    midiCtrlMergeEraseIcon          = new QPixmap(midi_ctrl_graph_merge_erase_xpm);
    midiCtrlMergeEraseInclusiveIcon = new QPixmap(midi_ctrl_graph_merge_erase_inclusive_xpm);
    midiCtrlMergeEraseWysiwygIcon   = new QPixmap(midi_ctrl_graph_merge_erase_wysiwyg_xpm);

    museIcon                         = new QPixmap(muse_icon_xpm);
    aboutMuseImage                   = new QPixmap(about_muse_xpm);

    globalIcon                       = new QIcon(QPixmap(global_xpm));
    userIcon                         = new QIcon(QPixmap(user_xpm));
    projectIcon                      = new QIcon(QPixmap(project_xpm));

    pianoNewIcon                     = new QIcon(QPixmap(pianoNew_xpm));
    presetsNewIcon                   = new QIcon(QPixmap(presetsNew_xpm));

    //----------------------------------
    //   SVG...
    //----------------------------------

    dropDownTriangleSVGIcon  = icons.getSVG("drop_down_triangle.svg");
    expandLeftRightSVGIcon  = icons.getSVG("expand_left_right.svg");

    routingInputSVGIcon = icons.getSVG("routing_input.svg");
    routingOutputSVGIcon = icons.getSVG("routing_output.svg");
    routingInputUnconnectedSVGIcon = icons.getSVG("routing_input_unconnected.svg");
    routingOutputUnconnectedSVGIcon = icons.getSVG("routing_output_unconnected.svg");

    headphonesOffSVGIcon = icons.getSVG("headphones_off.svg");
    headphonesOnSVGIcon = icons.getSVG("headphones_on.svg");

    muteOffSVGIcon = icons.getSVG("mute_off.svg");
    muteOnSVGIcon = icons.getSVG("mute_on.svg");
    muteOnXSVGIcon = icons.getSVG("mute_on_X.svg");
    muteProxyOnSVGIcon = icons.getSVG("mute_proxy_on.svg");
    muteAndProxyOnSVGIcon = icons.getSVG("mute_and_proxy_on.svg");

    muteStateSVGIcon = icons.getSVG("mute_off.svg");
    icons.addSVG(muteStateSVGIcon, "mute_on.svg");

    soloOffSVGIcon = icons.getSVG("solo_spotlight_off.svg");
    soloOnSVGIcon = icons.getSVG("solo_spotlight_on.svg");
    soloOnAloneSVGIcon = icons.getSVG("solo_spotlight_on_alone.svg");
    soloProxyOnSVGIcon = icons.getSVG("solo_proxy_spotlight_on.svg");
    soloProxyOnAloneSVGIcon = icons.getSVG("solo_proxy_spotlight_on_alone.svg");
    soloAndProxyOnSVGIcon = icons.getSVG("solo_and_proxy_spotlight_on.svg");

    soloStateSVGIcon = icons.getSVG("solo_spotlight_off.svg");
    icons.addSVG(soloStateSVGIcon, "solo_spotlight_on_alone.svg");

    trackOffSVGIcon  = icons.getSVG("track_off.svg");
    trackOnSVGIcon = icons.getSVG("track_on.svg");
    icons.addSVG(trackOnSVGIcon, "track_off.svg");

    stereoOffSVGIcon  = icons.getSVG("stereo_off.svg");
    stereoOnSVGIcon = icons.getSVG("stereo_on.svg");

    preFaderOffSVGIcon  = icons.getSVG("pre_fader_off.svg");
    preFaderOnSVGIcon = icons.getSVG("pre_fader_on.svg");

    recArmOffSVGIcon = icons.getSVG("rec_arm_off_default_col.svg");
    recArmOnSVGIcon = icons.getSVG("rec_arm_on.svg");

    recArmStateSVGIcon = icons.getSVG("rec_arm_off_default_col.svg");
    icons.addSVG(recArmStateSVGIcon, "rec_arm_on.svg");

    monitorOffSVGIcon = icons.getSVG("monitor_off_default_col.svg");
    monitorOnSVGIcon = icons.getSVG("monitor_on.svg");

    monitorStateSVGIcon = icons.getSVG("monitor_off_default_col.svg");
    icons.addSVG(monitorStateSVGIcon, "monitor_on.svg");

    velocityPerNoteSVGIcon = icons.getSVG("velocity_all_notes.svg");
    velocityPerNoteSVGIcon->addFile(":/svg/velocity_per_note.svg", QSize(), QIcon::Normal, QIcon::On);

    midiControllerNewSVGIcon = icons.getSVG("midi_controller_new.svg");
    midiControllerSelectSVGIcon = icons.getSVG("midi_controller_select.svg");
    midiControllerRemoveSVGIcon = icons.getSVG("midi_controller_remove.svg");


//    soloSVGIcon = icons.getSVG("headphones_off.svg");
//    soloSVGIcon->addFile(":/svg/headphones_on.svg", QSize(), QIcon::Normal, QIcon::On);
    // TODO
//    soloProxySVGIcon = icons.getSVG("headphones_off.svg");
//    soloProxySVGIcon->addFile(":/svg/headphones_on.svg", QSize(), QIcon::Normal, QIcon::On);

    muteSVGIcon = icons.getSVG("mute_off.svg");
    muteSVGIcon->addFile(":/svg/mute_on.svg", QSize(), QIcon::Normal, QIcon::On);

//    trackEnableSVGIcon = icons.getSVG("track_on.svg");
//    trackEnableSVGIcon->addFile(":/svg/track_off.svg", QSize(), QIcon::Normal, QIcon::On);

    //recArmSVGIcon = icons.getSVG("rec_arm_off_default_col.svg");
//    recArmSVGIcon = icons.getSVG("rec_arm_off.svg");
//    recArmSVGIcon->addFile(":/svg/rec_arm_on.svg", QSize(), QIcon::Normal, QIcon::On);

    //recMasterSVGIcon = icons.getSVG("rec_arm_off_default_col.svg");
    recMasterSVGIcon = icons.getSVG("rec_arm_off.svg");
    recMasterSVGIcon->addFile(":/svg/rec_arm_on.svg", QSize(), QIcon::Normal, QIcon::On);


    stopSVGIcon = icons.getSVG("stop.svg");

    playSVGIcon = icons.getSVG("play_off.svg");
    playSVGIcon->addFile(":/svg/play_on.svg", QSize(), QIcon::Normal, QIcon::On);

    fastForwardSVGIcon = icons.getSVG("fast_forward.svg");

    rewindSVGIcon = icons.getSVG("rewind.svg");

    rewindToStartSVGIcon = icons.getSVG("rewind_to_start.svg");

    externSyncOffSVGIcon = icons.getSVG("extern_sync_off.svg");
    externSyncOnSVGIcon = icons.getSVG("extern_sync_on.svg");

    masterTrackOffSVGIcon = icons.getSVG("master_track_off.svg");
    masterTrackOnSVGIcon = icons.getSVG("master_track_on.svg");

    jackTransportOffSVGIcon = icons.getSVG("jack_transport_off.svg");
    jackTransportOnSVGIcon = icons.getSVG("jack_transport_on.svg");

    timebaseMasterOffSVGIcon = icons.getSVG("timebase_master_off.svg");
    timebaseMasterOnSVGIcon = icons.getSVG("timebase_master_on.svg");

    metronomeOffSVGIcon = icons.getSVG("metronome_off.svg");
    metronomeOnSVGIcon = icons.getSVG("metronome_on.svg");

    fixedSpeedSVGIcon = icons.getSVG("speed_off.svg");
    fixedSpeedSVGIcon->addFile(":/svg/speed_on.svg", QSize(), QIcon::Normal, QIcon::On);
    transportAffectsLatencySVGIcon = icons.getSVG("transport_affects_latency_off.svg");
    transportAffectsLatencySVGIcon->addFile(":/svg/transport_affects_latency_on.svg", QSize(), QIcon::Normal, QIcon::On);
    overrideLatencySVGIcon = icons.getSVG("override_latency_off.svg");
    overrideLatencySVGIcon->addFile(":/svg/override_latency_on.svg", QSize(), QIcon::Normal, QIcon::On);

    panicSVGIcon      = icons.getSVG("panic.svg");
    loopSVGIcon       = icons.getSVG("loop.svg");
    punchinSVGIcon    = icons.getSVG("punchin.svg");
    punchoutSVGIcon   = icons.getSVG("punchout.svg");
    undoSVGIcon       = icons.getSVG("undo.svg");
    redoSVGIcon       = icons.getSVG("redo.svg");
    midiInSVGIcon     = icons.getSVG("midiin.svg");
    steprecSVGIcon    = icons.getSVG("steprec.svg");
    speakerSVGIcon    = icons.getSVG("speaker.svg");
    speakerSingleNoteSVGIcon = icons.getSVG("speaker_single_note.svg");
    speakerChordsSVGIcon     = icons.getSVG("speaker_chords.svg");
    whatsthisSVGIcon  = icons.getSVG("whatsthis.svg");
    infoSVGIcon       = icons.getSVG("info.svg");
    showFieldsSVGIcon = icons.getSVG("show_fields.svg");
    exitSVGIcon       = icons.getSVG("exit.svg");
    noteSVGIcon       = icons.getSVG("note.svg");
    metaSVGIcon       = icons.getSVG("meta.svg");
    ctrlSVGIcon       = icons.getSVG("ctrl.svg");
    sysexSVGIcon      = icons.getSVG("sysex.svg");
    tracktypeSVGIcon  = icons.getSVG("tracktype.svg");

    filenewSVGIcon     = icons.getSVG("filenew.svg");
    filetemplateSVGIcon = icons.getSVG("filefromtemplate.svg");
    fileopenSVGIcon    = icons.getSVG("fileopen.svg");
    filesaveSVGIcon    = icons.getSVG("filesave.svg");
    filesaveasSVGIcon  = icons.getSVG("filesaveas.svg");
    filesaveRevisionSVGIcon = icons.getSVG("filesave_revision.svg");
    filesaveTemplateSVGIcon = icons.getSVG("filesave_template.svg");
    filesaveProjectSVGIcon  = icons.getSVG("filesave_project.svg");
    filecloseSVGIcon   = icons.getSVG("fileclose.svg");
    appexitSVGIcon     = icons.getSVG("appexit.svg");

    pianorollSVGIcon   = icons.getSVG("pianoroll.svg");
    arrangerSVGIcon    = icons.getSVG("arranger.svg");
    waveeditorSVGIcon  = icons.getSVG("waveeditor.svg");
    scoreeditSVGIcon   = icons.getSVG("scoreedit.svg");
    mastereditSVGIcon  = icons.getSVG("masteredit.svg");
    drumeditSVGIcon    = icons.getSVG("drumedit.svg");
    listeditSVGIcon    = icons.getSVG("listedit.svg");

    synthSVGIcon       = icons.getSVG("synth.svg");
    trackInputSVGIcon  = icons.getSVG("track_input.svg");
    trackOutputSVGIcon = icons.getSVG("track_output.svg");
    trackAuxSVGIcon    = icons.getSVG("track_aux.svg");
    trackGroupVGIcon   = icons.getSVG("track_group.svg");

    ankerSVGIcon       = icons.getSVG("anker.svg");
    settingsSVGIcon    = icons.getSVG("settings.svg");
    transportSVGIcon   = icons.getSVG("transport.svg");
    bigtimeSVGIcon     = icons.getSVG("bigtime.svg");
    mixerSVGIcon       = icons.getSVG("mixer.svg");
    plusSVGIcon        = icons.getSVG("plus.svg");
    minusSVGIcon       = icons.getSVG("minus.svg");
    keySVGIcon         = icons.getSVG("key.svg");
    delSelTracksSVGIcon = icons.getSVG("delete_sel_tracks.svg");
    duplSelTrackSVGIcon = icons.getSVG("duplicate_sel_track.svg");
    duplSelTracksSVGIcon = icons.getSVG("duplicate_sel_tracks.svg");

    // tool icons
    pencilIconSVG     = icons.getSVG("pencil.svg");
    glueIconSVG       = icons.getSVG("glue.svg");
    cutterIconSVG     = icons.getSVG("cutter.svg");
    zoomIconSVG       = icons.getSVG("zoom.svg");
    zoomAtIconSVG     = icons.getSVG("zoomAt.svg");
    deleteIconSVG     = icons.getSVG("eraser.svg");
    drawIconSVG       = icons.getSVG("draw.svg");
    pointerIconSVG    = icons.getSVG("pointer.svg");
    mutePartsIconSVG  = icons.getSVG("mute_parts.svg");
    handIconSVG       = icons.getSVG("hand.svg");
    closedHandIconSVG = icons.getSVG("closed_hand.svg");
    cursorIconSVG     = icons.getSVG("cursor.svg");
    //magnetIconSVG     = icons.getSVG("magnet.svg");
    //customMoveIconSVG = icons.getSVG("cursor_move.svg");
    pencilMove4WayIconSVG = icons.getSVG("pencil_move_4_way.svg");
    pencilMoveHorizIconSVG = icons.getSVG("pencil_move_horiz.svg");
    pencilMoveVertIconSVG = icons.getSVG("pencil_move_vert.svg");
    audioStretchIconSVG = icons.getSVG("audio_stretch.svg");
    audioResampleIconSVG = icons.getSVG("audio_resample.svg");

    noscaleSVGIcon[0] = icons.getSVG("noscale1.svg");
    noscaleSVGIcon[1] = icons.getSVG("noscale2.svg");
    noscaleSVGIcon[2] = icons.getSVG("noscale3.svg");

    routeAutoAdjustSVGIcon = icons.getSVG("route_auto_adjust.svg");
    routeSelSourceSVGIcon = icons.getSVG("route_sel_source.svg");
    routeSelDestSVGIcon = icons.getSVG("route_sel_dest.svg");
    routeSourceSVGIcon = icons.getSVG("route_sources.svg");
    routeDestSVGIcon = icons.getSVG("route_destinations.svg");

    gridOnSVGIcon = icons.getSVG("grid_on.svg");
    rangeToSelectionSVGIcon = icons.getSVG("range_to_selection.svg");
    quantizeSVGIcon = icons.getSVG("quantize.svg");
    clearSVGIcon = icons.getSVG("clear.svg");
    downmixOffSVGIcon = icons.getSVG("downmix_off.svg");
    downmixOnSVGIcon = icons.getSVG("downmix_on.svg");
    downmixTrackSVGIcon = icons.getSVG("downmix_track.svg");
    downmixStateSVGIcon = icons.getSVG("downmix_off.svg");
    icons.addSVG(downmixStateSVGIcon, "downmix_on.svg");
    restartSVGIcon = icons.getSVG("restart.svg");
    snapshotSVGIcon = icons.getSVG("snapshot.svg");
    emptyBarSVGIcon = icons.getSVG("empty_bar.svg");
    lockSVGIcon = icons.getSVG("lock.svg");

    midiResetSVGIcon = icons.getSVG("midi_reset.svg");
    midiInitSVGIcon = icons.getSVG("midi_init.svg");
    midiLocalOffSVGIcon = icons.getSVG("midi_local_off.svg");
    midiTransformSVGIcon = icons.getSVG("midi_transform.svg");
    midiInputTransformSVGIcon = icons.getSVG("midi_input_transform.svg");
    midiInputTransposeSVGIcon = icons.getSVG("midi_input_transpose.svg");
    midiInputFilterSVGIcon = icons.getSVG("midi_input_filter.svg");
    midiInputRemoteSVGIcon = icons.getSVG("midi_input_remote.svg");
    midiSyncSVGIcon = icons.getSVG("midi_sync.svg");
    midiExportImportSVGIcon = icons.getSVG("midi_import_export.svg");

    cutSVGIcon = icons.getSVG("cut.svg");
    copySVGIcon = icons.getSVG("copy.svg");
    pasteSVGIcon = icons.getSVG("paste.svg");
    pasteDialogSVGIcon = icons.getSVG("paste_dialog.svg");
    pasteCloneSVGIcon = icons.getSVG("paste_clone.svg");
    copyRangeSVGIcon = icons.getSVG("copy_range.svg");
    deleteSVGIcon = icons.getSVG("delete.svg");
    pasteSelectedTrackSVGIcon = icons.getSVG("paste_selected_track.svg");
    pasteCloneSelectedTrackSVGIcon = icons.getSVG("paste_clone_selected_track.svg");

    eyeSVGIcon = icons.getSVG("eye.svg");
    eyeCrossedSVGIcon = icons.getSVG("eye_crossed.svg");
    eyeGreySVGIcon = icons.getSVG("eye_grey.svg");

    nextPartSVGIcon = icons.getSVG("next_part.svg");
    lastPartSVGIcon = icons.getSVG("last_part.svg");

    selectAllSVGIcon = icons.getSVG("select_all.svg");
    selectAllTrackSVGIcon = icons.getSVG("select_all_track.svg");
    selectInsideLoopSVGIcon = icons.getSVG("select_inside_loop.svg");
    selectOutsideLoopSVGIcon = icons.getSVG("select_outside_loop.svg");
    selectInvertSVGIcon = icons.getSVG("select_invert.svg");
    deselectAllSVGIcon = icons.getSVG("deselect_all.svg");

    ledGreenSVGIcon = icons.getSVG("led_green.svg");
    ledGreenDarkSVGIcon = icons.getSVG("led_green_dark.svg");
    ledRedSVGIcon = icons.getSVG("led_red.svg");
    ledBlueSVGIcon = icons.getSVG("led_blue.svg");
    ledYellowSVGIcon = icons.getSVG("led_yellow.svg");
    ledOffSVGIcon = icons.getSVG("led_off.svg");

    routeInSVGIcon = icons.getSVG("route_in.svg");
    routeOutSVGIcon = icons.getSVG("route_out.svg");
    routeInMidiSVGIcon = icons.getSVG("route_in_midi.svg");
    routeOutMidiSVGIcon = icons.getSVG("route_out_midi.svg");
    midiPortSVGIcon = icons.getSVG("midi_port.svg");
    routerSVGIcon = icons.getSVG("router.svg");

    appearanceSVGIcon = icons.getSVG("appearance.svg");
    editInstrumentSVGIcon = icons.getSVG("edit_instrument.svg");
    deltaSVGIcon = icons.getSVG("delta.svg");

    QPixmap px(10,10);
    px.fill(Qt::transparent);
    dummySVGIcon = new QIcon(px);

    //----------------------------------
    // Cursors
    //----------------------------------

    editpasteSCursor = new QCursor(pasteSVGIcon->pixmap(QSize(cursorSize, cursorSize)));
    editpasteCloneSCursor = new QCursor(copySVGIcon->pixmap(QSize(cursorSize, cursorSize)));

    // tool cursors
    pencilCursor     = new QCursor(pencilIconSVG->pixmap(QSize(cursorSize, cursorSize)), 0, qRound(dpr * (cursorSize - 1)));
    glueCursor       = new QCursor(glueIconSVG->pixmap(QSize(cursorSize, cursorSize)),  0, qRound(dpr * (cursorSize - 1)));
    cutterCursor     = new QCursor(cutterIconSVG->pixmap(QSize(cursorSize, cursorSize)),  0, qRound(dpr * (cursorSize - 1)));
    zoomCursor       = new QCursor(zoomIconSVG->pixmap(QSize(cursorSize, cursorSize)));
    zoomAtCursor     = new QCursor(zoomAtIconSVG->pixmap(QSize(cursorSize, cursorSize)));
    deleteCursor     = new QCursor(deleteIconSVG->pixmap(QSize(cursorSize, cursorSize)), qRound(dpr * (cursorSize / 6)), qRound(dpr * (cursorSize - 1)));
    drawCursor       = new QCursor(drawIconSVG->pixmap(QSize(cursorSize, cursorSize)), 0, qRound(dpr * (cursorSize - 1)));
    mutePartsCursor  = new QCursor(mutePartsIconSVG->pixmap(QSize(cursorSize, cursorSize)));
    handCursor       = new QCursor(handIconSVG->pixmap(QSize(cursorSize, cursorSize)));
    closedHandCursor = new QCursor(closedHandIconSVG->pixmap(QSize(cursorSize, cursorSize)));
    //magnetCursor     = new QCursor(magnetIconSVG->pixmap(QSize(cursorSize, cursorSize)), -1, qRound(dpr * 15));
    // This one needs to be bigger to contain drum notes.
    //customMoveCursor = new QCursor(customMoveIconSVG->pixmap(QSize(2 * cursorSize, 2 * cursorSize)));
    pencilMove4WayCursor =
            new QCursor(pencilMove4WayIconSVG->pixmap(QSize(cursorSize, cursorSize)), 0, qRound(dpr * (cursorSize - 1)));
    pencilMoveHorizCursor =
            new QCursor(pencilMoveHorizIconSVG->pixmap(QSize(cursorSize, cursorSize)), 0, qRound(dpr * (cursorSize - 1)));
    pencilMoveVertCursor =
            new QCursor(pencilMoveVertIconSVG->pixmap(QSize(cursorSize, cursorSize)), 0, qRound(dpr * (cursorSize - 1)));
}

//---------------------------------------------------------
//   deleteIcons
//---------------------------------------------------------

void deleteIcons()
{
    delete midiCtrlMergeEraseIcon;
    delete midiCtrlMergeEraseInclusiveIcon;
    delete midiCtrlMergeEraseWysiwygIcon;

    delete museIcon;
    delete aboutMuseImage;
    delete globalIcon;
    delete userIcon;
    delete projectIcon;

    //----------------------------------
    //   SVG...
    //----------------------------------

    delete dropDownTriangleSVGIcon;
    delete expandLeftRightSVGIcon;

    delete routingInputSVGIcon;
    delete routingOutputSVGIcon;
    delete routingInputUnconnectedSVGIcon;
    delete routingOutputUnconnectedSVGIcon;

    delete headphonesOffSVGIcon;
    delete headphonesOnSVGIcon;

    delete muteOffSVGIcon;
    delete muteOnSVGIcon;
    delete muteOnXSVGIcon;
    delete muteProxyOnSVGIcon;
    delete muteAndProxyOnSVGIcon;
    delete muteStateSVGIcon;

    delete soloOffSVGIcon;
    delete soloOnSVGIcon;
    delete soloOnAloneSVGIcon;
    delete soloProxyOnSVGIcon;
    delete soloProxyOnAloneSVGIcon;
    delete soloAndProxyOnSVGIcon;
    delete soloStateSVGIcon;

    delete trackOffSVGIcon;
    delete trackOnSVGIcon;

    delete stereoOffSVGIcon;
    delete stereoOnSVGIcon;

    delete preFaderOffSVGIcon;
    delete preFaderOnSVGIcon;

    delete recArmOffSVGIcon;
    delete recArmOnSVGIcon;
    delete recArmStateSVGIcon;

    delete monitorOffSVGIcon;
    delete monitorOnSVGIcon;
    delete monitorStateSVGIcon;

    delete velocityPerNoteSVGIcon;

    delete midiControllerNewSVGIcon;
    delete midiControllerSelectSVGIcon;
    delete midiControllerRemoveSVGIcon;


//    delete soloSVGIcon;
//    delete soloProxySVGIcon;
    delete muteSVGIcon;
//    delete trackEnableSVGIcon;
//    delete recArmSVGIcon;
    delete recMasterSVGIcon;

    delete stopSVGIcon;
    delete playSVGIcon;
    delete fastForwardSVGIcon;
    delete rewindSVGIcon;
    delete rewindToStartSVGIcon;

    delete externSyncOffSVGIcon;
    delete externSyncOnSVGIcon;

    delete masterTrackOffSVGIcon;
    delete masterTrackOnSVGIcon;

    delete jackTransportOffSVGIcon;
    delete jackTransportOnSVGIcon;

    delete timebaseMasterOffSVGIcon;
    delete timebaseMasterOnSVGIcon;

    delete metronomeOffSVGIcon;
    delete metronomeOnSVGIcon;

    delete fixedSpeedSVGIcon;
    delete transportAffectsLatencySVGIcon;
    delete overrideLatencySVGIcon;

    delete panicSVGIcon;
    delete loopSVGIcon;
    delete punchinSVGIcon;
    delete punchoutSVGIcon;
    delete undoSVGIcon;
    delete redoSVGIcon;
    delete midiInSVGIcon;
    delete steprecSVGIcon;
    delete speakerSVGIcon;
    delete speakerSingleNoteSVGIcon;
    delete speakerChordsSVGIcon;

    delete filenewSVGIcon;
    delete filetemplateSVGIcon;
    delete fileopenSVGIcon;
    delete filesaveSVGIcon;
    delete filesaveasSVGIcon;
    delete filesaveProjectSVGIcon;
    delete filesaveTemplateSVGIcon;
    delete filesaveRevisionSVGIcon;
    delete filecloseSVGIcon;
    delete appexitSVGIcon;
    delete whatsthisSVGIcon;
    delete infoSVGIcon;
    delete showFieldsSVGIcon;
    delete exitSVGIcon;
    delete noteSVGIcon;
    delete metaSVGIcon;
    delete ctrlSVGIcon;
    delete sysexSVGIcon;
    delete tracktypeSVGIcon;

    delete pencilIconSVG;
    delete glueIconSVG;
    delete cutterIconSVG;
    delete zoomIconSVG;
    delete zoomAtIconSVG;
    delete deleteIconSVG;
    delete drawIconSVG;
    delete pointerIconSVG;
    delete mutePartsIconSVG;
    delete handIconSVG;
    delete closedHandIconSVG;
    delete cursorIconSVG;
    //delete magnetIconSVG;
    //delete customMoveIconSVG;
    delete pencilMove4WayIconSVG;
    delete pencilMoveHorizIconSVG;
    delete pencilMoveVertIconSVG;
    delete audioStretchIconSVG;
    delete audioResampleIconSVG;
    delete ankerSVGIcon;
    delete settingsSVGIcon;
    delete transportSVGIcon;
    delete bigtimeSVGIcon;
    delete mixerSVGIcon;
    delete plusSVGIcon;
    delete minusSVGIcon;
    delete keySVGIcon;
    delete delSelTracksSVGIcon;
    delete duplSelTrackSVGIcon;
    delete duplSelTracksSVGIcon;

    delete noscaleSVGIcon[0];
    delete noscaleSVGIcon[1];
    delete noscaleSVGIcon[2];

    delete pianorollSVGIcon;
    delete arrangerSVGIcon;
    delete waveeditorSVGIcon;
    delete scoreeditSVGIcon;
    delete mastereditSVGIcon;
    delete drumeditSVGIcon;
    delete listeditSVGIcon;

    delete synthSVGIcon;
    delete trackInputSVGIcon;
    delete trackOutputSVGIcon;
    delete trackAuxSVGIcon;
    delete trackGroupVGIcon;

    delete routeAutoAdjustSVGIcon;
    delete routeSelSourceSVGIcon;
    delete routeSelDestSVGIcon;
    delete routeSourceSVGIcon;
    delete routeDestSVGIcon;

    delete gridOnSVGIcon;
    delete rangeToSelectionSVGIcon;
    delete quantizeSVGIcon;
    delete clearSVGIcon;
    delete downmixOffSVGIcon;
    delete downmixOnSVGIcon;
    delete downmixTrackSVGIcon;
    delete downmixStateSVGIcon;
    delete restartSVGIcon;
    delete snapshotSVGIcon;
    delete emptyBarSVGIcon;
    delete lockSVGIcon;

    delete midiResetSVGIcon;
    delete midiInitSVGIcon;
    delete midiLocalOffSVGIcon;

    delete midiTransformSVGIcon;
    delete midiInputTransformSVGIcon;
    delete midiInputTransposeSVGIcon;
    delete midiInputFilterSVGIcon;
    delete midiInputRemoteSVGIcon;
    delete midiSyncSVGIcon;
    delete midiExportImportSVGIcon;

    delete cutSVGIcon;
    delete copySVGIcon;
    delete pasteSVGIcon;
    delete pasteDialogSVGIcon;
    delete pasteCloneSVGIcon;
    delete copyRangeSVGIcon;
    delete deleteSVGIcon;
    delete pasteSelectedTrackSVGIcon;
    delete pasteCloneSelectedTrackSVGIcon;

    delete eyeSVGIcon;
    delete eyeCrossedSVGIcon;
    delete eyeGreySVGIcon;

    delete nextPartSVGIcon;
    delete lastPartSVGIcon;

    delete selectAllSVGIcon;
    delete selectAllTrackSVGIcon;
    delete selectInsideLoopSVGIcon;
    delete selectOutsideLoopSVGIcon;
    delete selectInvertSVGIcon;
    delete deselectAllSVGIcon;

    delete ledGreenSVGIcon;
    delete ledGreenDarkSVGIcon;
    delete ledRedSVGIcon;
    delete ledBlueSVGIcon;
    delete ledYellowSVGIcon;
    delete ledOffSVGIcon;

    delete routeInSVGIcon;
    delete routeOutSVGIcon;
    delete routeInMidiSVGIcon;
    delete routeOutMidiSVGIcon;
    delete midiPortSVGIcon;
    delete routerSVGIcon;

    delete appearanceSVGIcon;
    delete editInstrumentSVGIcon;
    delete deltaSVGIcon;

    delete dummySVGIcon;

    //----------------------------------
    // Cursors
    //----------------------------------

    delete editpasteSCursor;
    delete editpasteCloneSCursor;

    delete pencilCursor;
    delete glueCursor;
    delete cutterCursor;
    delete zoomCursor;
    delete zoomAtCursor;
    delete deleteCursor;
    delete drawCursor;
    delete mutePartsCursor;
    delete handCursor;
    delete closedHandCursor;
    //delete magnetCursor;
    //delete customMoveCursor;
    delete pencilMove4WayCursor;
    delete pencilMoveHorizCursor;
    delete pencilMoveVertCursor;
}

} // namespace MusEGui
