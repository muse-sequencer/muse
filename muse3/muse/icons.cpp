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

#include <QIcon>
#include <QCursor>

#include "xpm/track_comment.xpm"
#include "xpm/audio_bounce_to_file.xpm"
#include "xpm/audio_bounce_to_track.xpm"
#include "xpm/audio_restartaudio.xpm"
#include "xpm/automation_clear_data.xpm"
#include "xpm/automation_mixer.xpm"
#include "xpm/automation_take_snapshot.xpm"
#include "xpm/midi_edit_instrument.xpm"
#include "xpm/midi_init_instr.xpm"
#include "xpm/midi_inputplugins.xpm"
#include "xpm/midi_inputplugins_midi_input_filter.xpm"
#include "xpm/midi_inputplugins_midi_input_transform.xpm"
#include "xpm/midi_inputplugins_remote_control.xpm"
#include "xpm/midi_inputplugins_transpose.xpm"
#include "xpm/midi_local_off.xpm"
#include "xpm/midi_reset_instr.xpm"
#include "xpm/settings_appearance_settings.xpm"
#include "xpm/settings_configureshortcuts.xpm"
#include "xpm/settings_globalsettings.xpm"
#include "xpm/settings_midifileexport.xpm"
#include "xpm/settings_midiport_softsynths.xpm"
#include "xpm/settings_midisync.xpm"
#include "xpm/view_bigtime_window.xpm"
#include "xpm/view_marker.xpm"
#include "xpm/view_transport_window.xpm"

#include "xpm/delete.xpm"
#include "xpm/midi_ctrl_graph_merge_erase.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_inclusive.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_wysiwyg.xpm"

#include "xpm/record1.xpm"
#include "xpm/dot.xpm"
#include "xpm/doth.xpm"
#include "xpm/dot1.xpm"
#include "xpm/synth.xpm"
#include "xpm/cmark.xpm"
#include "xpm/lmark.xpm"
#include "xpm/rmark.xpm"
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
#include "xpm/flag.xpm"
#include "xpm/flagS.xpm"
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

#include "xpm/mixerS.xpm"
#include "xpm/cliplistS.xpm"
#include "xpm/delta_on.xpm"
#include "xpm/delta_off.xpm"
#include "xpm/velo_all.xpm"
#include "xpm/velo_per_note.xpm"

#include "xpm/addtrack_addmiditrack_2.xpm"
#include "xpm/addtrack_audiogroup.xpm"
#include "xpm/addtrack_audioinput.xpm"
#include "xpm/addtrack_audiooutput.xpm"
#include "xpm/addtrack_auxsend_2.xpm"
#include "xpm/addtrack_old_drumtrack.xpm"
#include "xpm/addtrack_drumtrack_2.xpm"
#include "xpm/addtrack_wavetrack.xpm"
#include "xpm/edit_drumms.xpm"
#include "xpm/edit_list.xpm"
#include "xpm/edit_wave.xpm"
#include "xpm/edit_mastertrack.xpm"
#include "xpm/edit_pianoroll.xpm"
#include "xpm/edit_score.xpm"
#include "xpm/edit_track_add.xpm"
#include "xpm/edit_track_del.xpm"
#include "xpm/mastertrack_graphic.xpm"
#include "xpm/mastertrack_list.xpm"
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

#include "xpm/router_filter_source.xpm"
#include "xpm/router_filter_destination.xpm"
#include "xpm/router_filter_source_routes.xpm"
#include "xpm/router_filter_destination_routes.xpm"
#include "xpm/router_view_splitter.xpm"

#include "icons.h"


namespace MusEGui {
  
QPixmap* track_commentIcon;
QPixmap* mixerSIcon;
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
QPixmap* synthIcon;
QPixmap* markIcon[3];
QPixmap* cursorIcon;
QPixmap* muteIcon;
QPixmap* eyeIcon;
QPixmap* eyeCrossedIcon;
QPixmap* eyeGrayIcon;
QPixmap* upIcon;
QPixmap* downIcon;
QPixmap* flagIcon;
QPixmap* flagIconS;
QPixmap* lockIcon;

QIcon* pianoIconSet;
QIcon* scoreIconSet;
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

QPixmap* addtrack_addmiditrackIcon;
QPixmap* addtrack_audiogroupIcon;
QPixmap* addtrack_audioinputIcon;
QPixmap* addtrack_audiooutputIcon;
QPixmap* addtrack_auxsendIcon;
QPixmap* addtrack_drumtrackIcon;
QPixmap* addtrack_newDrumtrackIcon;
QPixmap* addtrack_wavetrackIcon;
QPixmap* edit_drummsIcon;
QPixmap* edit_listIcon;
QPixmap* edit_waveIcon;
QPixmap* edit_mastertrackIcon;
QPixmap* edit_track_addIcon;
QPixmap* edit_track_delIcon;
QPixmap* mastertrack_graphicIcon;
QPixmap* mastertrack_listIcon;
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
QPixmap* midi_inputpluginsIcon;
QPixmap* midi_inputplugins_midi_input_filterIcon;
QPixmap* midi_inputplugins_midi_input_transformIcon;
QPixmap* midi_inputplugins_remote_controlIcon;
QPixmap* midi_inputplugins_transposeIcon;
QPixmap* midi_local_offIcon;
QPixmap* midi_reset_instrIcon;
QPixmap* settings_appearance_settingsIcon;
QPixmap* settings_configureshortcutsIcon;
QPixmap* settings_globalsettingsIcon;
QPixmap* settings_midifileexportIcon;
QPixmap* settings_midiport_softsynthsIcon;
QPixmap* settings_midisyncIcon;
QPixmap* view_bigtime_windowIcon;
QPixmap* view_markerIcon;
QPixmap* view_transport_windowIcon;

QPixmap* museIcon;
QPixmap* aboutMuseImage;
QPixmap* museLeftSideLogo;

QIcon* globalIcon;
QIcon* projectIcon;
QIcon* userIcon;


QIcon* pianoNewIcon;
QIcon* presetsNewIcon;

QPixmap* routerFilterSourceIcon;
QPixmap* routerFilterDestinationIcon;
QPixmap* routerFilterSourceRoutesIcon;
QPixmap* routerFilterDestinationRoutesIcon;
QPixmap* routerViewSplitterIcon;


//----------------------------------
//   SVG...
//----------------------------------

QIcon* dropDownTriangleSVGIcon;

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

QIcon* soloOffSVGIcon;
QIcon* soloOnSVGIcon;
QIcon* soloOnAloneSVGIcon;
QIcon* soloProxyOnSVGIcon;
QIcon* soloProxyOnAloneSVGIcon;
QIcon* soloAndProxyOnSVGIcon;

QIcon* trackOffSVGIcon;
QIcon* trackOnSVGIcon;

QIcon* stereoOffSVGIcon;
QIcon* stereoOnSVGIcon;

QIcon* preFaderOffSVGIcon;
QIcon* preFaderOnSVGIcon;

QIcon* recArmOffSVGIcon;
QIcon* recArmOnSVGIcon;

QIcon* monitorOffSVGIcon;
QIcon* monitorOnSVGIcon;


QIcon* soloSVGIcon;
QIcon* soloProxySVGIcon;
QIcon* muteSVGIcon;
QIcon* trackEnableSVGIcon;
QIcon* recArmSVGIcon;
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
QIcon* exitSVGIcon;
QIcon* noteSVGIcon;
QIcon* metaSVGIcon;
QIcon* ctrlSVGIcon;
QIcon* sysexSVGIcon;
QIcon* tracktypeSVGIcon;
QIcon* mixerstripSVGIcon;

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
//   initIcons
//---------------------------------------------------------

void initIcons(int cursorSize)
      {
      const qreal dpr = qApp->devicePixelRatio();
        
      track_commentIcon = new QPixmap(track_comment_xpm);
      deleteIcon        = new QPixmap(delete_xpm);
      midiCtrlMergeEraseIcon          = new QPixmap(midi_ctrl_graph_merge_erase_xpm);
      midiCtrlMergeEraseInclusiveIcon = new QPixmap(midi_ctrl_graph_merge_erase_inclusive_xpm);
      midiCtrlMergeEraseWysiwygIcon   = new QPixmap(midi_ctrl_graph_merge_erase_wysiwyg_xpm);

      record1_Icon = new QPixmap(record1_xpm);
      dotIcon      = new QPixmap(dot_xpm);
      dothIcon     = new QPixmap(doth_xpm);
      dot1Icon     = new QPixmap(dot1_xpm);
      synthIcon    = new QPixmap(synth_xpm);
      markIcon[0]  = new QPixmap(cmark_xpm);
      markIcon[1]  = new QPixmap(lmark_xpm);
      markIcon[2]  = new QPixmap(rmark_xpm);
      cursorIcon   = new QPixmap(cursor_xpm);
      muteIcon     = new QPixmap(editmuteS_xpm);
      eyeIcon      = new QPixmap(eye_xpm);
      eyeCrossedIcon = new QPixmap(eye_crossed_xpm);
      eyeGrayIcon  = new QPixmap(eye_gray_xpm);
      upIcon       = new QPixmap(up_xpm);
      downIcon     = new QPixmap(down_xpm);
      flagIcon     = new QPixmap(flag_xpm);
      flagIconS    = new QPixmap(flagS_xpm);
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
      
      mixerSIcon           = new QPixmap(mixerS_xpm);
      cliplistSIcon        = new QPixmap(cliplistS_xpm);
      deltaOnIcon          = new QPixmap(delta_on_xpm);
      deltaOffIcon         = new QPixmap(delta_off_xpm);
      veloPerNote_OnIcon   = new QPixmap(velo_per_note_xpm);
      veloPerNote_OffIcon  = new QPixmap(velo_all_xpm);

      addtrack_addmiditrackIcon     = new QPixmap(addtrack_addmiditrack_2_xpm);
      addtrack_audiogroupIcon       = new QPixmap(addtrack_audiogroup_xpm);
      addtrack_audioinputIcon       = new QPixmap(addtrack_audioinput_xpm);
      addtrack_audiooutputIcon      = new QPixmap(addtrack_audiooutput_xpm);
      addtrack_auxsendIcon          = new QPixmap(addtrack_auxsend_2_xpm);
      addtrack_drumtrackIcon        = new QPixmap(addtrack_old_drumtrack_xpm);
      addtrack_newDrumtrackIcon     = new QPixmap(addtrack_drumtrack_2_xpm);
      addtrack_wavetrackIcon        = new QPixmap(addtrack_wavetrack_xpm);
      edit_drummsIcon               = new QPixmap(edit_drumms_xpm);
      edit_listIcon                 = new QPixmap(edit_list_xpm);
      edit_waveIcon                 = new QPixmap(edit_wave_xpm);
      edit_mastertrackIcon          = new QPixmap(edit_mastertrack_xpm);
      edit_track_addIcon            = new QPixmap(edit_track_add_xpm);
      edit_track_delIcon            = new QPixmap(edit_track_del_xpm);
      mastertrack_graphicIcon       = new QPixmap(mastertrack_graphic_xpm);
      mastertrack_listIcon          = new QPixmap(mastertrack_list_xpm);
      midi_transformIcon            = new QPixmap(midi_transform_xpm);
      selectIcon                    = new QPixmap(select_xpm);
      select_allIcon                = new QPixmap(select_all_xpm);
      select_all_parts_on_trackIcon = new QPixmap(select_all_parts_on_track_xpm);
      select_deselect_allIcon       = new QPixmap(select_deselect_all);
      icon_select_deselect_all      = new QIcon(*select_deselect_allIcon);
      select_inside_loopIcon        = new QPixmap(select_inside_loop_xpm);
      select_invert_selectionIcon   = new QPixmap(select_invert_selection);
      select_outside_loopIcon       = new QPixmap(select_outside_loop_xpm);

      pianoIconSet                  = new QIcon(QPixmap(edit_pianoroll_xpm));
      scoreIconSet                  = new QIcon(QPixmap(edit_score_xpm));

      audio_bounce_to_fileIcon                      = new QPixmap(audio_bounce_to_file_xpm);
      audio_bounce_to_trackIcon                     = new QPixmap(audio_bounce_to_track_xpm);
      audio_restartaudioIcon                        = new QPixmap(audio_restartaudio_xpm);
      automation_clear_dataIcon                     = new QPixmap(automation_clear_data_xpm);
      automation_mixerIcon                          = new QPixmap(automation_mixer_xpm);
      automation_take_snapshotIcon                  = new QPixmap(automation_take_snapshot_xpm);
      midi_edit_instrumentIcon                      = new QPixmap(midi_edit_instrument_xpm);
      midi_init_instrIcon                           = new QPixmap(midi_init_instr_xpm);
      midi_inputpluginsIcon                         = new QPixmap(midi_inputplugins_xpm);
      midi_inputplugins_midi_input_filterIcon       = new QPixmap(midi_inputplugins_midi_input_filter_xpm);
      midi_inputplugins_midi_input_transformIcon    = new QPixmap(midi_inputplugins_midi_input_transform_xpm);
      midi_inputplugins_remote_controlIcon          = new QPixmap(midi_inputplugins_remote_control_xpm);
      midi_inputplugins_transposeIcon               = new QPixmap(midi_inputplugins_transpose_xpm);
      midi_local_offIcon                            = new QPixmap(midi_local_off_xpm);
      midi_reset_instrIcon                          = new QPixmap(midi_reset_instr_xpm);
      settings_appearance_settingsIcon              = new QPixmap(settings_appearance_settings_xpm);
      settings_configureshortcutsIcon               = new QPixmap(settings_configureshortcuts_xpm);
      settings_globalsettingsIcon                   = new QPixmap(settings_globalsettings_xpm);
      settings_midifileexportIcon                   = new QPixmap(settings_midifileexport_xpm);
      settings_midiport_softsynthsIcon              = new QPixmap(settings_midiport_softsynths_xpm);
      settings_midisyncIcon                         = new QPixmap(settings_midisync_xpm);
      view_bigtime_windowIcon                       = new QPixmap(view_bigtime_window_xpm);
      view_markerIcon                               = new QPixmap(view_marker_xpm);
      view_transport_windowIcon                     = new QPixmap(view_transport_window_xpm);

      museIcon                                      = new QPixmap(muse_icon_xpm);
      aboutMuseImage                                = new QPixmap(about_muse_xpm);
      museLeftSideLogo                              = new QPixmap(muse_leftside_logo_xpm);

      globalIcon                                    = new QIcon(QPixmap(global_xpm));
      userIcon                                      = new QIcon(QPixmap(user_xpm));
      projectIcon                                   = new QIcon(QPixmap(project_xpm));

      pianoNewIcon                                  = new QIcon(QPixmap(pianoNew_xpm));
      presetsNewIcon                                = new QIcon(QPixmap(presetsNew_xpm));

      routerFilterSourceIcon                        = new QPixmap(router_filter_source_xpm);
      routerFilterDestinationIcon                   = new QPixmap(router_filter_destination_xpm);
      routerFilterSourceRoutesIcon                  = new QPixmap(router_filter_source_routes_xpm);
      routerFilterDestinationRoutesIcon             = new QPixmap(router_filter_destination_routes_xpm);
      routerViewSplitterIcon                        = new QPixmap(router_view_splitter_xpm);
      

      //----------------------------------
      //   SVG...
      //----------------------------------

      dropDownTriangleSVGIcon  = new QIcon(":/svg/drop_down_triangle.svg");
      
      routingInputSVGIcon = new QIcon(":/svg/routing_input.svg");
      routingOutputSVGIcon = new QIcon(":/svg/routing_output.svg");
      routingInputUnconnectedSVGIcon = new QIcon(":/svg/routing_input_unconnected.svg");
      routingOutputUnconnectedSVGIcon = new QIcon(":/svg/routing_output_unconnected.svg");

      headphonesOffSVGIcon = new QIcon(":/svg/headphones_off.svg");
      headphonesOnSVGIcon = new QIcon(":/svg/headphones_on.svg");

      muteOffSVGIcon = new QIcon(":/svg/mute_off.svg");
      muteOnSVGIcon = new QIcon(":/svg/mute_on.svg");
      muteOnXSVGIcon = new QIcon(":/svg/mute_on_X.svg");
      muteProxyOnSVGIcon = new QIcon(":/svg/mute_proxy_on.svg");
      muteAndProxyOnSVGIcon = new QIcon(":/svg/mute_and_proxy_on.svg");

      soloOffSVGIcon = new QIcon(":/svg/solo_spotlight_off.svg");
      soloOnSVGIcon = new QIcon(":/svg/solo_spotlight_on.svg");
      soloOnAloneSVGIcon = new QIcon(":/svg/solo_spotlight_on_alone.svg");
      soloProxyOnSVGIcon = new QIcon(":/svg/solo_proxy_spotlight_on.svg");
      soloProxyOnAloneSVGIcon = new QIcon(":/svg/solo_proxy_spotlight_on_alone.svg");
      soloAndProxyOnSVGIcon = new QIcon(":/svg/solo_and_proxy_spotlight_on.svg");

      trackOffSVGIcon  = new QIcon(":/svg/track_off.svg");
      trackOnSVGIcon = new QIcon(":/svg/track_on.svg");

      stereoOffSVGIcon  = new QIcon(":/svg/stereo_off.svg");
      stereoOnSVGIcon = new QIcon(":/svg/stereo_on.svg");

      preFaderOffSVGIcon  = new QIcon(":/svg/pre_fader_off.svg");
      preFaderOnSVGIcon = new QIcon(":/svg/pre_fader_on.svg");

      recArmOffSVGIcon = new QIcon(":/svg/rec_arm_off_default_col.svg");
      recArmOnSVGIcon = new QIcon(":/svg/rec_arm_on.svg");

      monitorOffSVGIcon = new QIcon(":/svg/monitor_off_default_col.svg");
      monitorOnSVGIcon = new QIcon(":/svg/monitor_on.svg");


      soloSVGIcon = new QIcon(":/svg/headphones_off.svg");
      soloSVGIcon->addFile(":/svg/headphones_on.svg", QSize(), QIcon::Normal, QIcon::On);
      // TODO
      soloProxySVGIcon = new QIcon(":/svg/headphones_off.svg");
      soloProxySVGIcon->addFile(":/svg/headphones_on.svg", QSize(), QIcon::Normal, QIcon::On);

      muteSVGIcon = new QIcon(":/svg/mute_off.svg");
      muteSVGIcon->addFile(":/svg/mute_on.svg", QSize(), QIcon::Normal, QIcon::On);

      trackEnableSVGIcon = new QIcon(":/svg/track_on.svg");
      trackEnableSVGIcon->addFile(":/svg/track_off.svg", QSize(), QIcon::Normal, QIcon::On);

      //recArmSVGIcon = new QIcon(":/svg/rec_arm_off_default_col.svg");
      recArmSVGIcon = new QIcon(":/svg/rec_arm_off.svg");
      recArmSVGIcon->addFile(":/svg/rec_arm_on.svg", QSize(), QIcon::Normal, QIcon::On);

      //recMasterSVGIcon = new QIcon(":/svg/rec_arm_off_default_col.svg");
      recMasterSVGIcon = new QIcon(":/svg/rec_arm_off.svg");
      recMasterSVGIcon->addFile(":/svg/rec_arm_on.svg", QSize(), QIcon::Normal, QIcon::On);


      stopSVGIcon = new QIcon(":/svg/stop.svg");

      playSVGIcon = new QIcon(":/svg/play_off.svg");
      playSVGIcon->addFile(":/svg/play_on.svg", QSize(), QIcon::Normal, QIcon::On);

      fastForwardSVGIcon = new QIcon(":/svg/fast_forward.svg");

      rewindSVGIcon = new QIcon(":/svg/rewind.svg");

      rewindToStartSVGIcon = new QIcon(":/svg/rewind_to_start.svg");
      
      externSyncOffSVGIcon = new QIcon(":/svg/extern_sync_off.svg");
      externSyncOnSVGIcon = new QIcon(":/svg/extern_sync_on.svg");
      
      masterTrackOffSVGIcon = new QIcon(":/svg/master_track_off.svg");
      masterTrackOnSVGIcon = new QIcon(":/svg/master_track_on.svg");
      
      jackTransportOffSVGIcon = new QIcon(":/svg/jack_transport_off.svg");
      jackTransportOnSVGIcon = new QIcon(":/svg/jack_transport_on.svg");
      
      timebaseMasterOffSVGIcon = new QIcon(":/svg/timebase_master_off.svg");
      timebaseMasterOnSVGIcon = new QIcon(":/svg/timebase_master_on.svg");
      
      metronomeOffSVGIcon = new QIcon(":/svg/metronome_off.svg");
      metronomeOnSVGIcon = new QIcon(":/svg/metronome_on.svg");
      
      fixedSpeedSVGIcon = new QIcon(":/svg/speed_off.svg");
      fixedSpeedSVGIcon->addFile(":/svg/speed_on.svg", QSize(), QIcon::Normal, QIcon::On);
      transportAffectsLatencySVGIcon = new QIcon(":/svg/transport_affects_latency_off.svg");
      transportAffectsLatencySVGIcon->addFile(":/svg/transport_affects_latency_on.svg", QSize(), QIcon::Normal, QIcon::On);
      overrideLatencySVGIcon = new QIcon(":/svg/override_latency_off.svg");
      overrideLatencySVGIcon->addFile(":/svg/override_latency_on.svg", QSize(), QIcon::Normal, QIcon::On);

      panicSVGIcon      = new QIcon(":/svg/panic.svg");
      loopSVGIcon       = new QIcon(":/svg/loop.svg");
      punchinSVGIcon    = new QIcon(":/svg/punchin.svg");
      punchoutSVGIcon   = new QIcon(":/svg/punchout.svg");
      undoSVGIcon       = new QIcon(":/svg/undo.svg");
      redoSVGIcon       = new QIcon(":/svg/redo.svg");
      midiinSVGIcon     = new QIcon(":/svg/midiin.svg");
      steprecSVGIcon    = new QIcon(":/svg/steprec.svg");
      speakerSVGIcon    = new QIcon(":/svg/speaker.svg");
      speakerSingleNoteSVGIcon = new QIcon(":/svg/speaker_single_note.svg");
      speakerChordsSVGIcon     = new QIcon(":/svg/speaker_chords.svg");
      whatsthisSVGIcon  = new QIcon(":/svg/whatsthis.svg");
      exitSVGIcon       = new QIcon(":/svg/exit.svg");
      noteSVGIcon       = new QIcon(":/svg/note.svg");
      metaSVGIcon       = new QIcon(":/svg/meta.svg");
      ctrlSVGIcon       = new QIcon(":/svg/ctrl.svg");
      sysexSVGIcon      = new QIcon(":/svg/sysex.svg");
      tracktypeSVGIcon  = new QIcon(":/svg/tracktype.svg");
      mixerstripSVGIcon = new QIcon(":/svg/mixerstrip.svg");

      filenewSVGIcon     = new QIcon(":/svg/filenew.svg");
      filetemplateSVGIcon = new QIcon(":/svg/filefromtemplate.svg");
      fileopenSVGIcon    = new QIcon(":/svg/fileopen.svg");
      filesaveSVGIcon    = new QIcon(":/svg/filesave.svg");
      filesaveasSVGIcon  = new QIcon(":/svg/filesaveas.svg");
      filecloseSVGIcon   = new QIcon(":/svg/fileclose.svg");
      appexitSVGIcon     = new QIcon(":/svg/appexit.svg");


      // tool icons
      pencilIconSVG     = new QIcon(":/svg/pencil.svg");
      glueIconSVG       = new QIcon(":/svg/glue.svg");
      cutterIconSVG     = new QIcon(":/svg/cutter.svg");
      zoomIconSVG       = new QIcon(":/svg/zoom.svg");
      zoomAtIconSVG     = new QIcon(":/svg/zoomAt.svg");
      deleteIconSVG     = new QIcon(":/svg/eraser.svg");
      drawIconSVG       = new QIcon(":/svg/draw.svg");
      pointerIconSVG    = new QIcon(":/svg/pointer.svg");
      mutePartsIconSVG  = new QIcon(":/svg/mute_parts.svg");
      handIconSVG       = new QIcon(":/svg/hand.svg");
      closedHandIconSVG = new QIcon(":/svg/closed_hand.svg");
      cursorIconSVG     = new QIcon(":/svg/cursor.svg");
      //magnetIconSVG     = new QIcon(":/svg/magnet.svg");
      //customMoveIconSVG = new QIcon(":/svg/cursor_move.svg");
      pencilMove4WayIconSVG = new QIcon(":/svg/pencil_move_4_way.svg");
      pencilMoveHorizIconSVG = new QIcon(":/svg/pencil_move_horiz.svg");
      pencilMoveVertIconSVG = new QIcon(":/svg/pencil_move_vert.svg");
      audioStretchIconSVG = new QIcon(":/svg/audio_stretch.svg");
      audioResampleIconSVG = new QIcon(":/svg/audio_resample.svg");

      noscaleSVGIcon[0] = new QIcon(":/svg/noscale1.svg");
      noscaleSVGIcon[1] = new QIcon(":/svg/noscale2.svg");
      noscaleSVGIcon[2] = new QIcon(":/svg/noscale3.svg");

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
      delete synthIcon;    
      delete markIcon[0];  
      delete markIcon[1];  
      delete markIcon[2];  
      delete cursorIcon;   
      delete muteIcon;
      delete upIcon;       
      delete downIcon;     
      delete flagIcon;  
      delete flagIconS; 
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

      delete mixerSIcon;           
      delete cliplistSIcon;        
      delete deltaOnIcon;          
      delete deltaOffIcon;         
      delete veloPerNote_OnIcon;          
      delete veloPerNote_OffIcon;         

      delete addtrack_addmiditrackIcon;     
      delete addtrack_audiogroupIcon;       
      delete addtrack_audioinputIcon;       
      delete addtrack_audiooutputIcon;      
      delete addtrack_auxsendIcon;          
      delete addtrack_drumtrackIcon;        
      delete addtrack_newDrumtrackIcon;
      delete addtrack_wavetrackIcon;        
      delete edit_drummsIcon;               
      delete edit_listIcon;                 
      delete edit_waveIcon;                 
      delete edit_mastertrackIcon;          
      delete edit_track_addIcon;            
      delete edit_track_delIcon;            
      delete mastertrack_graphicIcon;       
      delete mastertrack_listIcon;          
      delete midi_transformIcon;            
      delete selectIcon;                    
      delete select_allIcon;                
      delete select_all_parts_on_trackIcon; 
      delete select_deselect_allIcon;       
      delete icon_select_deselect_all;       
      delete select_inside_loopIcon;        
      delete select_invert_selectionIcon;   
      delete select_outside_loopIcon;       
      delete pianoIconSet;                  
      delete scoreIconSet;                  

      delete audio_bounce_to_fileIcon;      
      delete audio_bounce_to_trackIcon;     
      delete audio_restartaudioIcon;        
      delete automation_clear_dataIcon;     
      delete automation_mixerIcon;          
      delete automation_take_snapshotIcon;  
      delete midi_edit_instrumentIcon;      
      delete midi_init_instrIcon;           
      delete midi_inputpluginsIcon;         
      delete midi_inputplugins_midi_input_filterIcon;       
      delete midi_inputplugins_midi_input_transformIcon;    
      delete midi_inputplugins_remote_controlIcon;          
      delete midi_inputplugins_transposeIcon;               
      delete midi_local_offIcon;                            
      delete midi_reset_instrIcon;                          
      delete settings_appearance_settingsIcon;              
      delete settings_configureshortcutsIcon;               
      delete settings_globalsettingsIcon;                   
      delete settings_midifileexportIcon;                   
      delete settings_midiport_softsynthsIcon;              
      delete settings_midisyncIcon;                         
      delete view_bigtime_windowIcon;                       
      delete view_markerIcon;                               
      delete view_transport_windowIcon;                     

      delete museIcon;                                      
      delete aboutMuseImage;                                
      delete museLeftSideLogo;                              
      delete globalIcon;
      delete userIcon;
      delete projectIcon;

      delete routerFilterSourceIcon;
      delete routerFilterDestinationIcon;
      delete routerFilterSourceRoutesIcon;
      delete routerFilterDestinationRoutesIcon;
      delete routerViewSplitterIcon;


      //----------------------------------
      //   SVG...
      //----------------------------------

      delete dropDownTriangleSVGIcon;

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

      delete soloOffSVGIcon;
      delete soloOnSVGIcon;
      delete soloOnAloneSVGIcon;
      delete soloProxyOnSVGIcon;
      delete soloProxyOnAloneSVGIcon;
      delete soloAndProxyOnSVGIcon;

      delete trackOffSVGIcon;
      delete trackOnSVGIcon;

      delete stereoOffSVGIcon;
      delete stereoOnSVGIcon;

      delete preFaderOffSVGIcon;
      delete preFaderOnSVGIcon;

      delete recArmOffSVGIcon;
      delete recArmOnSVGIcon;

      delete monitorOffSVGIcon;
      delete monitorOnSVGIcon;


      delete soloSVGIcon;
      delete soloProxySVGIcon;
      delete muteSVGIcon;
      delete trackEnableSVGIcon;
      delete recArmSVGIcon;
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

      delete noscaleSVGIcon[0];
      delete noscaleSVGIcon[1];
      delete noscaleSVGIcon[2];


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
