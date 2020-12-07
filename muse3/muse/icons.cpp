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

// NOTICE: Although it is tempting to use multi-icons (addPixmap, addFile etc.),
//          certain styles do not support it, such as QtCurve.
//         Therefore the separate icons must be manually set upon each state.

#include <QPixmap>
#include <QIcon>
#include <QCursor>
#include <QDir>
#include <QApplication>

#include "xpm/track_comment.xpm"
#include "xpm/audio_bounce_to_file.xpm"
#include "xpm/audio_bounce_to_track.xpm"
#include "xpm/audio_restartaudio.xpm"
#include "xpm/automation_clear_data.xpm"
#include "xpm/automation_mixer.xpm"
#include "xpm/automation_take_snapshot.xpm"
#include "xpm/midi_edit_instrument.xpm"
#include "xpm/midi_init_instr.xpm"
#include "xpm/midi_inputplugins_midi_input_filter.xpm"
#include "xpm/midi_inputplugins_midi_input_transform.xpm"
#include "xpm/midi_inputplugins_remote_control.xpm"
#include "xpm/midi_inputplugins_transpose.xpm"
#include "xpm/midi_local_off.xpm"
#include "xpm/midi_reset_instr.xpm"
#include "xpm/settings_appearance_settings.xpm"
#include "xpm/settings_midifileexport.xpm"
#include "xpm/settings_midisync.xpm"
#include "xpm/view_marker.xpm"

#include "xpm/delete.xpm"
#include "xpm/midi_ctrl_graph_merge_erase.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_inclusive.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_wysiwyg.xpm"

#include "xpm/record1.xpm"
#include "xpm/dot.xpm"
#include "xpm/doth.xpm"
#include "xpm/dot1.xpm"
#include "xpm/cursor.xpm"

#include "xpm/routing_input_button_slim_4.xpm"
#include "xpm/routing_output_button_slim_4.xpm"
#include "xpm/routing_midi_input_button_slim.xpm"
#include "xpm/routing_midi_output_button_slim.xpm"

#include "xpm/eye.xpm"
#include "xpm/eye_gray.xpm"
#include "xpm/eye_crossed.xpm"

#include "xpm/up.xpm"
#include "xpm/down.xpm"
#include "xpm/lock.xpm"

#include "xpm/editcutS.xpm"
#include "xpm/editcopyS.xpm"
#include "xpm/editpasteS.xpm"
#include "xpm/editmuteS.xpm"
#include "xpm/editpastecloneS.xpm"
#include "xpm/editpaste2trackS.xpm"
#include "xpm/editpasteclone2trackS.xpm"

#include "xpm/toggle_small.xpm"
#include "xpm/greendot.xpm"
#include "xpm/greendot12x12.xpm"
#include "xpm/reddot.xpm"
#include "xpm/darkgreendot.xpm"
#include "xpm/bluedot.xpm"
#include "xpm/bluedot12x12.xpm"
#include "xpm/graydot12x12.xpm"
#include "xpm/orangedot.xpm"
#include "xpm/orangedot12x12.xpm"

#include "xpm/cliplistS.xpm"
#include "xpm/delta_on.xpm"
#include "xpm/delta_off.xpm"
#include "xpm/velo_all.xpm"
#include "xpm/velo_per_note.xpm"

#include "xpm/edit_track_add.xpm"
#include "xpm/midi_transform.xpm"
#include "xpm/select.xpm"
#include "xpm/select_all.xpm"
#include "xpm/select_all_parts_on_track.xpm"
#include "xpm/select_deselect_all.xpm"
#include "xpm/select_inside_loop.xpm"
#include "xpm/select_invert_selection.xpm"
#include "xpm/select_outside_loop.xpm"

#include "xpm/muse_icon.xpm"
#include "xpm/about_muse.xpm"
#include "xpm/muse_leftside_logo.xpm"

#include "xpm/global.xpm"
#include "xpm/project.xpm"
#include "xpm/user.xpm"

#include "xpm/pianoNew.xpm"
#include "xpm/presetsNew.xpm"

#include "icons.h"


namespace MusEGui {

QPixmap* track_commentIcon;
QPixmap* cliplistSIcon;
QPixmap* deltaOnIcon;
QPixmap* deltaOffIcon;
QPixmap* veloPerNote_OnIcon;
QPixmap* veloPerNote_OffIcon;

QPixmap* routesInIcon;
QPixmap* routesOutIcon;
QPixmap* routesMidiInIcon;
QPixmap* routesMidiOutIcon;

QPixmap* deleteIcon;
QPixmap* midiCtrlMergeEraseIcon;
QPixmap* midiCtrlMergeEraseInclusiveIcon;
QPixmap* midiCtrlMergeEraseWysiwygIcon;

QPixmap* record1_Icon;
QPixmap* dotIcon;
QPixmap* dothIcon;
QPixmap* dot1Icon;
QPixmap* cursorIcon;
QPixmap* muteIcon;
QPixmap* eyeIcon;
QPixmap* eyeCrossedIcon;
QPixmap* eyeGrayIcon;
QPixmap* upIcon;
QPixmap* downIcon;
QPixmap* lockIcon;

QIcon* editcutIconSet;
QIcon* editcopyIconSet;
QIcon* editpasteIconSet;
QIcon* editpaste2TrackIconSet;
QIcon* editpasteCloneIconSet;
QIcon* editpasteClone2TrackIconSet;

QPixmap* editpasteSIcon;
QPixmap* editpasteCloneSIcon;

QPixmap* toggle_small_Icon;
QPixmap* greendotIcon;
QPixmap* greendot12x12Icon;
QPixmap* reddotIcon;
QPixmap* darkgreendotIcon;
QPixmap* graydot12x12Icon;
QPixmap* bluedotIcon;
QPixmap* bluedot12x12Icon;
QPixmap* orangedotIcon;
QPixmap* orangedot12x12Icon;

QIcon* ledGreenIcon;
QIcon* ledDarkGreenIcon;

QPixmap* edit_track_addIcon;
QPixmap* midi_transformIcon;
QPixmap* selectIcon;
QPixmap* select_allIcon;
QPixmap* select_all_parts_on_trackIcon;
QPixmap* select_deselect_allIcon;
QIcon*   icon_select_deselect_all;
QPixmap* select_inside_loopIcon;
QPixmap* select_invert_selectionIcon;
QPixmap* select_outside_loopIcon;

QPixmap* audio_bounce_to_fileIcon;
QPixmap* audio_bounce_to_trackIcon;
QPixmap* audio_restartaudioIcon;
QPixmap* automation_clear_dataIcon;
QPixmap* automation_mixerIcon;
QPixmap* automation_take_snapshotIcon;
QPixmap* midi_edit_instrumentIcon;
QPixmap* midi_init_instrIcon;
QPixmap* midi_inputplugins_midi_input_filterIcon;
QPixmap* midi_inputplugins_midi_input_transformIcon;
QPixmap* midi_inputplugins_remote_controlIcon;
QPixmap* midi_inputplugins_transposeIcon;
QPixmap* midi_local_offIcon;
QPixmap* midi_reset_instrIcon;
QPixmap* settings_appearance_settingsIcon;
QPixmap* settings_midifileexportIcon;
QPixmap* settings_midisyncIcon;
QPixmap* view_markerIcon;

QPixmap* museIcon;
QPixmap* aboutMuseImage;
QPixmap* museLeftSideLogo;

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
QIcon* midiinSVGIcon;
QIcon* steprecSVGIcon;
QIcon* speakerSVGIcon;
QIcon* speakerSingleNoteSVGIcon;
QIcon* speakerChordsSVGIcon;
QIcon* filenewSVGIcon;
QIcon* filetemplateSVGIcon;
QIcon* fileopenSVGIcon;
QIcon* filesaveSVGIcon;
QIcon* filesaveasSVGIcon;
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
QIcon* mixerstripSVGIcon;
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

QIcon* routeAutoAdjustSVGIcon;
QIcon* routeSelSourceSVGIcon;
QIcon* routeSelDestSVGIcon;
QIcon* routeSourceSVGIcon;
QIcon* routeDestSVGIcon;

QIcon* gridOnSVGIcon;

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

    track_commentIcon = new QPixmap(track_comment_xpm);
    deleteIcon        = new QPixmap(delete_xpm);
    midiCtrlMergeEraseIcon          = new QPixmap(midi_ctrl_graph_merge_erase_xpm);
    midiCtrlMergeEraseInclusiveIcon = new QPixmap(midi_ctrl_graph_merge_erase_inclusive_xpm);
    midiCtrlMergeEraseWysiwygIcon   = new QPixmap(midi_ctrl_graph_merge_erase_wysiwyg_xpm);

    record1_Icon = new QPixmap(record1_xpm);
    dotIcon      = new QPixmap(dot_xpm);
    dothIcon     = new QPixmap(doth_xpm);
    dot1Icon     = new QPixmap(dot1_xpm);
    cursorIcon   = new QPixmap(cursor_xpm);
    muteIcon     = new QPixmap(editmuteS_xpm);
    eyeIcon      = new QPixmap(eye_xpm);
    eyeCrossedIcon = new QPixmap(eye_crossed_xpm);
    eyeGrayIcon  = new QPixmap(eye_gray_xpm);
    upIcon       = new QPixmap(up_xpm);
    downIcon     = new QPixmap(down_xpm);
    lockIcon     = new QPixmap(lock_xpm);

    editcutIconSet       = new QIcon(QPixmap(editcutS_xpm)); // ddskrjo
    editcopyIconSet      = new QIcon(QPixmap(editcopyS_xpm));
    editpasteIconSet     = new QIcon(QPixmap(editpasteS_xpm));
    editpaste2TrackIconSet = new QIcon(QPixmap(editpaste2trackS_xpm));
    editpasteCloneIconSet  = new QIcon(QPixmap(editpastecloneS_xpm));
    editpasteClone2TrackIconSet = new QIcon(QPixmap(editpasteclone2trackS_xpm)); // ..
    editpasteSIcon      = new QPixmap(editpasteS_xpm);
    editpasteCloneSIcon = new QPixmap(editpastecloneS_xpm);

    routesInIcon         = new QPixmap(routing_input_button_slim_4_xpm);
    routesOutIcon        = new QPixmap(routing_output_button_slim_4_xpm);
    routesMidiInIcon     = new QPixmap(routing_midi_input_button_slim_xpm);
    routesMidiOutIcon    = new QPixmap(routing_midi_output_button_slim_xpm);

    toggle_small_Icon    = new QPixmap(toggle_small_xpm);

    greendotIcon         = new QPixmap(greendot_xpm);
    greendot12x12Icon    = new QPixmap(greendot12x12_xpm);
    reddotIcon           = new QPixmap(reddot_xpm);
    darkgreendotIcon     = new QPixmap(darkgreendot_xpm);

    ledGreenIcon         = new QIcon(*greendotIcon);
    ledDarkGreenIcon     = new QIcon(*darkgreendotIcon);

    bluedotIcon          = new QPixmap(bluedot_xpm);
    bluedot12x12Icon     = new QPixmap(bluedot12x12_xpm);
    graydot12x12Icon     = new QPixmap(graydot12x12_xpm);
    orangedotIcon        = new QPixmap(orangedot_xpm);
    orangedot12x12Icon   = new QPixmap(orangedot12x12_xpm);

    cliplistSIcon        = new QPixmap(cliplistS_xpm);
    deltaOnIcon          = new QPixmap(delta_on_xpm);
    deltaOffIcon         = new QPixmap(delta_off_xpm);
    veloPerNote_OnIcon   = new QPixmap(velo_per_note_xpm);
    veloPerNote_OffIcon  = new QPixmap(velo_all_xpm);

    edit_track_addIcon            = new QPixmap(edit_track_add_xpm);
    midi_transformIcon            = new QPixmap(midi_transform_xpm);
    selectIcon                    = new QPixmap(select_xpm);
    select_allIcon                = new QPixmap(select_all_xpm);
    select_all_parts_on_trackIcon = new QPixmap(select_all_parts_on_track_xpm);
    select_deselect_allIcon       = new QPixmap(select_deselect_all);
    icon_select_deselect_all      = new QIcon(*select_deselect_allIcon);
    select_inside_loopIcon        = new QPixmap(select_inside_loop_xpm);
    select_invert_selectionIcon   = new QPixmap(select_invert_selection);
    select_outside_loopIcon       = new QPixmap(select_outside_loop_xpm);

    audio_bounce_to_fileIcon                      = new QPixmap(audio_bounce_to_file_xpm);
    audio_bounce_to_trackIcon                     = new QPixmap(audio_bounce_to_track_xpm);
    audio_restartaudioIcon                        = new QPixmap(audio_restartaudio_xpm);
    automation_clear_dataIcon                     = new QPixmap(automation_clear_data_xpm);
    automation_mixerIcon                          = new QPixmap(automation_mixer_xpm);
    automation_take_snapshotIcon                  = new QPixmap(automation_take_snapshot_xpm);
    midi_edit_instrumentIcon                      = new QPixmap(midi_edit_instrument_xpm);
    midi_init_instrIcon                           = new QPixmap(midi_init_instr_xpm);
    midi_inputplugins_midi_input_filterIcon       = new QPixmap(midi_inputplugins_midi_input_filter_xpm);
    midi_inputplugins_midi_input_transformIcon    = new QPixmap(midi_inputplugins_midi_input_transform_xpm);
    midi_inputplugins_remote_controlIcon          = new QPixmap(midi_inputplugins_remote_control_xpm);
    midi_inputplugins_transposeIcon               = new QPixmap(midi_inputplugins_transpose_xpm);
    midi_local_offIcon                            = new QPixmap(midi_local_off_xpm);
    midi_reset_instrIcon                          = new QPixmap(midi_reset_instr_xpm);
    settings_appearance_settingsIcon              = new QPixmap(settings_appearance_settings_xpm);
    settings_midifileexportIcon                   = new QPixmap(settings_midifileexport_xpm);
    settings_midisyncIcon                         = new QPixmap(settings_midisync_xpm);
    view_markerIcon                               = new QPixmap(view_marker_xpm);

    museIcon                                      = new QPixmap(muse_icon_xpm);
    aboutMuseImage                                = new QPixmap(about_muse_xpm);
    museLeftSideLogo                              = new QPixmap(muse_leftside_logo_xpm);

    globalIcon                                    = new QIcon(QPixmap(global_xpm));
    userIcon                                      = new QIcon(QPixmap(user_xpm));
    projectIcon                                   = new QIcon(QPixmap(project_xpm));

    pianoNewIcon                                  = new QIcon(QPixmap(pianoNew_xpm));
    presetsNewIcon                                = new QIcon(QPixmap(presetsNew_xpm));

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
    midiinSVGIcon     = icons.getSVG("midiin.svg");
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
    mixerstripSVGIcon = icons.getSVG("mixerstrip.svg");

    filenewSVGIcon     = icons.getSVG("filenew.svg");
    filetemplateSVGIcon = icons.getSVG("filefromtemplate.svg");
    fileopenSVGIcon    = icons.getSVG("fileopen.svg");
    filesaveSVGIcon    = icons.getSVG("filesave.svg");
    filesaveasSVGIcon  = icons.getSVG("filesaveas.svg");
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

    //----------------------------------
    // Cursors
    //----------------------------------

    editpasteSCursor = new QCursor(QPixmap(*editpasteSIcon).scaled(qRound(dpr * cursorSize), qRound(dpr * cursorSize)));
    editpasteCloneSCursor = new QCursor(QPixmap(*editpasteCloneSIcon).scaled(qRound(dpr * cursorSize), qRound(dpr * cursorSize)));

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
    delete track_commentIcon;
    delete deleteIcon;
    delete midiCtrlMergeEraseIcon;
    delete midiCtrlMergeEraseInclusiveIcon;
    delete midiCtrlMergeEraseWysiwygIcon;

    delete record1_Icon;
    delete dotIcon;
    delete dothIcon;
    delete dot1Icon;
    delete cursorIcon;
    delete muteIcon;
    delete upIcon;
    delete downIcon;
    delete lockIcon;

    delete editcutIconSet;
    delete editcopyIconSet;
    delete editpasteIconSet;
    delete editpaste2TrackIconSet;
    delete editpasteCloneIconSet;
    delete editpasteClone2TrackIconSet;

    delete editpasteSIcon;
    delete editpasteCloneSIcon;

    delete routesInIcon;
    delete routesOutIcon;
    delete routesMidiInIcon;
    delete routesMidiOutIcon;

    delete toggle_small_Icon;

    delete ledGreenIcon;
    delete ledDarkGreenIcon;

    delete greendotIcon;
    delete reddotIcon;
    delete darkgreendotIcon;
    delete bluedotIcon;
    delete orangedotIcon;

    delete cliplistSIcon;
    delete deltaOnIcon;
    delete deltaOffIcon;
    delete veloPerNote_OnIcon;
    delete veloPerNote_OffIcon;

    delete edit_track_addIcon;
    delete midi_transformIcon;
    delete selectIcon;
    delete select_allIcon;
    delete select_all_parts_on_trackIcon;
    delete select_deselect_allIcon;
    delete icon_select_deselect_all;
    delete select_inside_loopIcon;
    delete select_invert_selectionIcon;
    delete select_outside_loopIcon;

    delete audio_bounce_to_fileIcon;
    delete audio_bounce_to_trackIcon;
    delete audio_restartaudioIcon;
    delete automation_clear_dataIcon;
    delete automation_mixerIcon;
    delete automation_take_snapshotIcon;
    delete midi_edit_instrumentIcon;
    delete midi_init_instrIcon;
    delete midi_inputplugins_midi_input_filterIcon;
    delete midi_inputplugins_midi_input_transformIcon;
    delete midi_inputplugins_remote_controlIcon;
    delete midi_inputplugins_transposeIcon;
    delete midi_local_offIcon;
    delete midi_reset_instrIcon;
    delete settings_appearance_settingsIcon;
    delete settings_midifileexportIcon;
    delete settings_midisyncIcon;
    delete view_markerIcon;

    delete museIcon;
    delete aboutMuseImage;
    delete museLeftSideLogo;
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
    delete midiinSVGIcon;
    delete steprecSVGIcon;
    delete speakerSVGIcon;
    delete speakerSingleNoteSVGIcon;
    delete speakerChordsSVGIcon;

    delete filenewSVGIcon;
    delete filetemplateSVGIcon;
    delete fileopenSVGIcon;
    delete filesaveSVGIcon;
    delete filesaveasSVGIcon;
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
    delete mixerstripSVGIcon;

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
