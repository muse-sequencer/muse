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
#include "xpm/note1.xpm"
#include "xpm/synth.xpm"
#include "xpm/cmark.xpm"
#include "xpm/lmark.xpm"
#include "xpm/rmark.xpm"
#include "xpm/steprec.xpm"
#include "xpm/cursor.xpm"
#include "xpm/filesave.xpm"
#include "xpm/filesaveas.xpm"
#include "xpm/fileopen.xpm"
#include "xpm/filesaveS.xpm"
#include "xpm/fileopenS.xpm"
#include "xpm/filenew.xpm"
#include "xpm/fileclose.xpm"
#include "xpm/appexit.xpm"

#include "xpm/routing_input_button_slim_4.xpm"
#include "xpm/routing_output_button_slim_4.xpm"
#include "xpm/routing_midi_input_button_slim.xpm"
#include "xpm/routing_midi_output_button_slim.xpm"

#include "xpm/eye.xpm"
#include "xpm/eye_gray.xpm"
#include "xpm/eye_crossed.xpm"

#include "xpm/up.xpm"
#include "xpm/down.xpm"
#include "xpm/midiin.xpm"
#include "xpm/sysex.xpm"
#include "xpm/ctrl.xpm"
#include "xpm/meta.xpm"
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

#include "xpm/speaker.xpm"
#include "xpm/buttondown.xpm"

#include "xpm/exit.xpm"

#include "xpm/toggle_small.xpm"
#include "xpm/greendot.xpm"
#include "xpm/greendot12x12.xpm"
#include "xpm/reddot.xpm"
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

#include "xpm/sine.xpm"
#include "xpm/saw.xpm"

#include "xpm/pianoNew.xpm"
#include "xpm/presetsNew.xpm"

#include "xpm/cpu.xpm"

#include "xpm/router_filter_source.xpm"
#include "xpm/router_filter_destination.xpm"
#include "xpm/router_filter_source_routes.xpm"
#include "xpm/router_filter_destination_routes.xpm"
#include "xpm/router_view_splitter.xpm"

#include "icons.h"

#define MPNGIMG(a) QPixmap(a)
#define MSVGIMG(a) QPixmap(a)

namespace MusEGui {
  bool use_theme_icons_if_possible = true;
  
  QPixmap* MPIXMAP(const char* const* fallback, const QString& name) {
    const QPixmap& fallback_pm = QPixmap(fallback);
    if(use_theme_icons_if_possible)
      return new QPixmap(QIcon::fromTheme(name, QIcon(fallback_pm)).pixmap(fallback_pm.width(),fallback_pm.height()));
    else 
      return new QPixmap(fallback);
  }
  
  QIcon* MICON(const char* const* fallback, const QString& name) {
    const QPixmap& fallback_pm = QPixmap(fallback);
    if(use_theme_icons_if_possible)
      return new QIcon(QIcon::fromTheme(name, QIcon(fallback_pm)));
    else 
      return new QIcon(fallback_pm);
  }

  
/* Quick API reference:
   -------------------

   QPixmap MPIXMAP(const char * const[] xpm, const QString & name)
   QIcon MICON(const char * const[] xpm, const QString & name)

      xpm:  a valid XPM image data
      name: filename of a theme icon, without the extension; or NULL
*/

QPixmap* track_commentIcon;
QPixmap* mixerSIcon;
QPixmap* cliplistSIcon;
QPixmap* deltaOnIcon;
QPixmap* deltaOffIcon;
QPixmap* veloPerNote_OnIcon;
QPixmap* veloPerNote_OffIcon;

QPixmap* exitIcon;

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
QPixmap* note1Icon;
QPixmap* synthIcon;
QPixmap* markIcon[3];
QPixmap* steprecIcon;
QPixmap* cursorIcon;
QPixmap* openIcon;
QPixmap* saveIcon;
QPixmap* saveasIcon;
QPixmap* openIconS;
QPixmap* saveIconS;
QPixmap* filenewIcon;
QPixmap* filecloseIcon;
QPixmap* appexitIcon;
QPixmap* muteIcon;
QPixmap* eyeIcon;
QPixmap* eyeCrossedIcon;
QPixmap* eyeGrayIcon;
QPixmap* upIcon;
QPixmap* downIcon;
QPixmap* midiinIcon;
QPixmap* sysexIcon;
QPixmap* ctrlIcon;
QPixmap* metaIcon;
QPixmap* flagIcon;
QPixmap* flagIconS;
QPixmap* lockIcon;

QPixmap* speakerIcon;
QPixmap* buttondownIcon;

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


QPixmap* sineIcon;
QPixmap* sawIcon;

QIcon* pianoNewIcon;
QIcon* presetsNewIcon;

QIcon* cpuIcon;

QPixmap* routerFilterSourceIcon;
QPixmap* routerFilterDestinationIcon;
QPixmap* routerFilterSourceRoutesIcon;
QPixmap* routerFilterDestinationRoutesIcon;
QPixmap* routerViewSplitterIcon;


//----------------------------------
//   SVG...
//----------------------------------

QPixmap* routingInputSVGPixmap;
QPixmap* routingOutputSVGPixmap;
QPixmap* routingInputUnconnectedSVGPixmap;
QPixmap* routingOutputUnconnectedSVGPixmap;

QPixmap* headphonesOffSVGPixmap;
QPixmap* headphonesOnSVGPixmap;

QPixmap* muteOffSVGPixmap;
QPixmap* muteOnSVGPixmap;
QPixmap* muteOnXSVGPixmap;
QPixmap* muteProxyOnSVGPixmap;
QPixmap* muteAndProxyOnSVGPixmap;

QPixmap* soloOffSVGPixmap;
QPixmap* soloOnSVGPixmap;
QPixmap* soloOnAloneSVGPixmap;
QPixmap* soloProxyOnSVGPixmap;
QPixmap* soloProxyOnAloneSVGPixmap;
QPixmap* soloAndProxyOnSVGPixmap;

QPixmap* trackOffSVGPixmap;
QPixmap* trackOnSVGPixmap;

QPixmap* stereoOffSVGPixmap;
QPixmap* stereoOnSVGPixmap;

QPixmap* preFaderOffSVGPixmap;
QPixmap* preFaderOnSVGPixmap;

QPixmap* recArmOffSVGPixmap;
QPixmap* recArmOnSVGPixmap;

QPixmap* monitorOffSVGPixmap;
QPixmap* monitorOnSVGPixmap;

QPixmap* externSyncOffSVGPixmap;
QPixmap* externSyncOnSVGPixmap;

QPixmap* masterTrackOffSVGPixmap;
QPixmap* masterTrackOnSVGPixmap;

QPixmap* jackTransportOffSVGPixmap;
QPixmap* jackTransportOnSVGPixmap;


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
QIcon* magnetIconSVG;

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
QCursor* magnetCursor;

//---------------------------------------------------------
//   initIcons
//---------------------------------------------------------

void initIcons(bool useThemeIconsIfPossible)
      {
      use_theme_icons_if_possible = useThemeIconsIfPossible;

      const qreal dpr = qApp->devicePixelRatio();
        
      track_commentIcon = MPIXMAP(track_comment_xpm, nullptr);
      deleteIcon   = MPIXMAP(delete_xpm, "draw-eraser");
      midiCtrlMergeEraseIcon              = MPIXMAP(midi_ctrl_graph_merge_erase_xpm, nullptr);
      midiCtrlMergeEraseInclusiveIcon     = MPIXMAP(midi_ctrl_graph_merge_erase_inclusive_xpm, nullptr);
      midiCtrlMergeEraseWysiwygIcon       = MPIXMAP(midi_ctrl_graph_merge_erase_wysiwyg_xpm, nullptr);

      record1_Icon  = MPIXMAP(record1_xpm, nullptr);
      dotIcon      = MPIXMAP(dot_xpm, "dialog-ok-apply");
      dothIcon     = MPIXMAP(doth_xpm, "draw-circle");
      dot1Icon     = MPIXMAP(dot1_xpm, nullptr);
      note1Icon    = MPIXMAP(note1_xpm, nullptr);
      synthIcon    = MPIXMAP(synth_xpm, nullptr);
      markIcon[0]  = MPIXMAP(cmark_xpm, nullptr);
      markIcon[1]  = MPIXMAP(lmark_xpm, nullptr);
      markIcon[2]  = MPIXMAP(rmark_xpm, nullptr);
      steprecIcon  = MPIXMAP(steprec_xpm, nullptr);
      cursorIcon   = MPIXMAP(cursor_xpm, nullptr);
      saveIcon     = MPIXMAP(filesave_xpm, "document-save");
      saveasIcon     = MPIXMAP(filesaveas_xpm, "document-save-as");
      openIcon     = MPIXMAP(fileopen_xpm, "document-open");
      saveIconS     = MPIXMAP(filesaveS_xpm, "document-save");
      openIconS     = MPIXMAP(fileopenS_xpm, "document-open");
      filenewIcon  = MPIXMAP(filenew_xpm, "document-new");
      filecloseIcon  = MPIXMAP(fileclose_xpm, "document-close");
      appexitIcon  = MPIXMAP(appexit_xpm, "application-exit");
      muteIcon     = MPIXMAP(editmuteS_xpm, "audio-volume-muted");
      eyeIcon      = MPIXMAP(eye_xpm, nullptr);
      eyeCrossedIcon  = MPIXMAP(eye_crossed_xpm, nullptr);
      eyeGrayIcon  = MPIXMAP(eye_gray_xpm, nullptr);
      upIcon       = MPIXMAP(up_xpm, "go-up");
      downIcon     = MPIXMAP(down_xpm, "go-down");
      midiinIcon = MPIXMAP(midiin_xpm, nullptr);
      sysexIcon   = MPIXMAP(sysex_xpm, nullptr);
      ctrlIcon    = MPIXMAP(ctrl_xpm, nullptr);
      metaIcon    = MPIXMAP(meta_xpm, nullptr);
      flagIcon    = MPIXMAP(flag_xpm, nullptr);
      flagIconS   = MPIXMAP(flagS_xpm, nullptr);
      lockIcon    = MPIXMAP(lock_xpm, nullptr);

      speakerIcon    = MPIXMAP(speaker_xpm, nullptr);
      buttondownIcon = MPIXMAP(buttondown_xpm, "arrow-down");

      editcutIconSet       = MICON(editcutS_xpm, "edit-cut"); // ddskrjo
      editcopyIconSet      = MICON(editcopyS_xpm, "edit-copy");
      editpasteIconSet     = MICON(editpasteS_xpm, "edit-paste");
      editpaste2TrackIconSet = MICON(editpaste2trackS_xpm, nullptr);
      editpasteCloneIconSet  = MICON(editpastecloneS_xpm, nullptr);
      editpasteClone2TrackIconSet = MICON(editpasteclone2trackS_xpm, nullptr); // ..
      editpasteSIcon      = MPIXMAP(editpasteS_xpm, nullptr);
      editpasteCloneSIcon = MPIXMAP(editpastecloneS_xpm, nullptr);

      // Changed by Tim. There are IMO no suitable theme substitutes for these two so far...
      exitIcon             = MPIXMAP(exit_xpm, nullptr);

      routesInIcon         = MPIXMAP(routing_input_button_slim_4_xpm, nullptr);
      routesOutIcon        = MPIXMAP(routing_output_button_slim_4_xpm, nullptr);
      routesMidiInIcon     = MPIXMAP(routing_midi_input_button_slim_xpm, nullptr);
      routesMidiOutIcon    = MPIXMAP(routing_midi_output_button_slim_xpm, nullptr);

      toggle_small_Icon    = MPIXMAP(toggle_small_xpm, nullptr);
      
      greendotIcon         = MPIXMAP(greendot_xpm, nullptr);
      greendot12x12Icon    = MPIXMAP(greendot12x12_xpm, nullptr);
      reddotIcon           = MPIXMAP(reddot_xpm, nullptr);
      
      ledGreenIcon         = new QIcon(*greendotIcon);
      
      bluedotIcon          = MPIXMAP(bluedot_xpm, nullptr);
      bluedot12x12Icon     = MPIXMAP(bluedot12x12_xpm, nullptr);
      graydot12x12Icon     = MPIXMAP(graydot12x12_xpm, nullptr);
      orangedotIcon        = MPIXMAP(orangedot_xpm, nullptr);
      orangedot12x12Icon   = MPIXMAP(orangedot12x12_xpm, nullptr);
      
      mixerSIcon           = MPIXMAP(mixerS_xpm, nullptr);
      cliplistSIcon        = MPIXMAP(cliplistS_xpm, nullptr);
      deltaOnIcon          = MPIXMAP(delta_on_xpm, nullptr);
      deltaOffIcon         = MPIXMAP(delta_off_xpm, nullptr);
      veloPerNote_OnIcon   = MPIXMAP(velo_per_note_xpm, nullptr);
      veloPerNote_OffIcon  = MPIXMAP(velo_all_xpm, nullptr);

      addtrack_addmiditrackIcon     = MPIXMAP(addtrack_addmiditrack_2_xpm, nullptr);
      addtrack_audiogroupIcon       = MPIXMAP(addtrack_audiogroup_xpm, nullptr);
      addtrack_audioinputIcon       = MPIXMAP(addtrack_audioinput_xpm, nullptr);
      addtrack_audiooutputIcon      = MPIXMAP(addtrack_audiooutput_xpm, nullptr);
      addtrack_auxsendIcon          = MPIXMAP(addtrack_auxsend_2_xpm, nullptr);
      addtrack_drumtrackIcon        = MPIXMAP(addtrack_old_drumtrack_xpm, nullptr);
      addtrack_newDrumtrackIcon     = MPIXMAP(addtrack_drumtrack_2_xpm, nullptr);
      addtrack_wavetrackIcon        = MPIXMAP(addtrack_wavetrack_xpm, nullptr);
      edit_drummsIcon               = MPIXMAP(edit_drumms_xpm, nullptr);
      edit_listIcon                 = MPIXMAP(edit_list_xpm, nullptr);
      edit_waveIcon                 = MPIXMAP(edit_wave_xpm, nullptr);
      edit_mastertrackIcon          = MPIXMAP(edit_mastertrack_xpm, nullptr);
      edit_track_addIcon            = MPIXMAP(edit_track_add_xpm, nullptr);
      edit_track_delIcon            = MPIXMAP(edit_track_del_xpm, nullptr);
      mastertrack_graphicIcon       = MPIXMAP(mastertrack_graphic_xpm, nullptr);
      mastertrack_listIcon          = MPIXMAP(mastertrack_list_xpm, nullptr);
      midi_transformIcon            = MPIXMAP(midi_transform_xpm, nullptr);
      selectIcon                    = MPIXMAP(select_xpm, nullptr);
      select_allIcon                = MPIXMAP(select_all_xpm, nullptr);
      select_all_parts_on_trackIcon = MPIXMAP(select_all_parts_on_track_xpm, nullptr);
      select_deselect_allIcon       = MPIXMAP(select_deselect_all, nullptr);
      icon_select_deselect_all      = new QIcon(*select_deselect_allIcon);
      select_inside_loopIcon        = MPIXMAP(select_inside_loop_xpm, nullptr);
      select_invert_selectionIcon   = MPIXMAP(select_invert_selection, nullptr);
      select_outside_loopIcon       = MPIXMAP(select_outside_loop_xpm, nullptr);
      pianoIconSet                  = MICON(edit_pianoroll_xpm, nullptr);
      scoreIconSet                  = MICON(edit_score_xpm, nullptr);

      audio_bounce_to_fileIcon                      = MPIXMAP(audio_bounce_to_file_xpm, nullptr);
      audio_bounce_to_trackIcon                     = MPIXMAP(audio_bounce_to_track_xpm, nullptr);
      audio_restartaudioIcon                        = MPIXMAP(audio_restartaudio_xpm, nullptr);
      automation_clear_dataIcon                     = MPIXMAP(automation_clear_data_xpm, nullptr);
      automation_mixerIcon                          = MPIXMAP(automation_mixer_xpm, nullptr);
      automation_take_snapshotIcon                  = MPIXMAP(automation_take_snapshot_xpm, nullptr);
      midi_edit_instrumentIcon                      = MPIXMAP(midi_edit_instrument_xpm, nullptr);
      midi_init_instrIcon                           = MPIXMAP(midi_init_instr_xpm, nullptr);
      midi_inputpluginsIcon                         = MPIXMAP(midi_inputplugins_xpm, nullptr);
      midi_inputplugins_midi_input_filterIcon       = MPIXMAP(midi_inputplugins_midi_input_filter_xpm, nullptr);
      midi_inputplugins_midi_input_transformIcon    = MPIXMAP(midi_inputplugins_midi_input_transform_xpm, nullptr);
      midi_inputplugins_remote_controlIcon          = MPIXMAP(midi_inputplugins_remote_control_xpm, nullptr);
      midi_inputplugins_transposeIcon               = MPIXMAP(midi_inputplugins_transpose_xpm, nullptr);
      midi_local_offIcon                            = MPIXMAP(midi_local_off_xpm, nullptr);
      midi_reset_instrIcon                          = MPIXMAP(midi_reset_instr_xpm, nullptr);
      settings_appearance_settingsIcon              = MPIXMAP(settings_appearance_settings_xpm, nullptr);
      settings_configureshortcutsIcon               = MPIXMAP(settings_configureshortcuts_xpm, nullptr);
      settings_globalsettingsIcon                   = MPIXMAP(settings_globalsettings_xpm, nullptr);
      settings_midifileexportIcon                   = MPIXMAP(settings_midifileexport_xpm, nullptr);
      settings_midiport_softsynthsIcon              = MPIXMAP(settings_midiport_softsynths_xpm, nullptr);
      settings_midisyncIcon                         = MPIXMAP(settings_midisync_xpm, nullptr);
      view_bigtime_windowIcon                       = MPIXMAP(view_bigtime_window_xpm, nullptr);
      view_markerIcon                               = MPIXMAP(view_marker_xpm, nullptr);
      view_transport_windowIcon                     = MPIXMAP(view_transport_window_xpm, nullptr);

      museIcon                                      = MPIXMAP(muse_icon_xpm, nullptr);
      aboutMuseImage                                = MPIXMAP(about_muse_xpm, nullptr);
      museLeftSideLogo                              = MPIXMAP(muse_leftside_logo_xpm, nullptr);
      globalIcon                                    = MICON(global_xpm, "folder");
      userIcon                                      = MICON(user_xpm, "user-home");
      projectIcon                                   = MICON(project_xpm, "folder-sound");

      sineIcon                                      = MPIXMAP(sine_xpm, nullptr);
      sawIcon                                       = MPIXMAP(saw_xpm, nullptr);

      pianoNewIcon                                  = MICON(pianoNew_xpm, nullptr);
      presetsNewIcon                                = MICON(presetsNew_xpm, nullptr);

      cpuIcon                                       = MICON(cpu_xpm, nullptr);

      routerFilterSourceIcon                        = MPIXMAP(router_filter_source_xpm, nullptr);
      routerFilterDestinationIcon                   = MPIXMAP(router_filter_destination_xpm, nullptr);
      routerFilterSourceRoutesIcon                  = MPIXMAP(router_filter_source_routes_xpm, nullptr);
      routerFilterDestinationRoutesIcon             = MPIXMAP(router_filter_destination_routes_xpm, nullptr);
      routerViewSplitterIcon                        = MPIXMAP(router_view_splitter_xpm, nullptr);
      

      //----------------------------------
      //   SVG...
      //----------------------------------

      routingInputSVGPixmap = new MSVGIMG(":/svg/routing_input.svg");
      routingOutputSVGPixmap = new MSVGIMG(":/svg/routing_output.svg");
      routingInputUnconnectedSVGPixmap = new MSVGIMG(":/svg/routing_input_unconnected.svg");
      routingOutputUnconnectedSVGPixmap = new MSVGIMG(":/svg/routing_output_unconnected.svg");

      headphonesOffSVGPixmap = new MSVGIMG(":/svg/headphones_off.svg");
      headphonesOnSVGPixmap = new MSVGIMG(":/svg/headphones_on.svg");

      muteOffSVGPixmap = new MSVGIMG(":/svg/mute_off.svg");
      muteOnSVGPixmap = new MSVGIMG(":/svg/mute_on.svg");
      muteOnXSVGPixmap = new MSVGIMG(":/svg/mute_on_X.svg");
      muteProxyOnSVGPixmap = new MSVGIMG(":/svg/mute_proxy_on.svg");
      muteAndProxyOnSVGPixmap = new MSVGIMG(":/svg/mute_and_proxy_on.svg");

      soloOffSVGPixmap = new MSVGIMG(":/svg/solo_spotlight_off.svg");
      soloOnSVGPixmap = new MSVGIMG(":/svg/solo_spotlight_on.svg");
      soloOnAloneSVGPixmap = new MSVGIMG(":/svg/solo_spotlight_on_alone.svg");
      soloProxyOnSVGPixmap = new MSVGIMG(":/svg/solo_proxy_spotlight_on.svg");
      soloProxyOnAloneSVGPixmap = new MSVGIMG(":/svg/solo_proxy_spotlight_on_alone.svg");
      soloAndProxyOnSVGPixmap = new MSVGIMG(":/svg/solo_and_proxy_spotlight_on.svg");

      trackOffSVGPixmap = new MSVGIMG(":/svg/track_off.svg");
      trackOnSVGPixmap = new MSVGIMG(":/svg/track_on.svg");

      stereoOffSVGPixmap = new MSVGIMG(":/svg/stereo_off.svg");
      stereoOnSVGPixmap = new MSVGIMG(":/svg/stereo_on.svg");

      preFaderOffSVGPixmap = new MSVGIMG(":/svg/pre_fader_off.svg");
      preFaderOnSVGPixmap = new MSVGIMG(":/svg/pre_fader_on.svg");

      //recArmOffSVGPixmap = new MSVGIMG(":/svg/rec_arm_off.svg");
      recArmOffSVGPixmap = new MSVGIMG(":/svg/rec_arm_off_default_col.svg");
      recArmOnSVGPixmap = new MSVGIMG(":/svg/rec_arm_on.svg");

      //monitorOffSVGPixmap = new MSVGIMG(":/svg/monitor_off.svg");
      monitorOffSVGPixmap = new MSVGIMG(":/svg/monitor_off_default_col.svg");
      monitorOnSVGPixmap = new MSVGIMG(":/svg/monitor_on.svg");
      //monitorOffSVGPixmap = new MSVGIMG(":/svg/headphones_off.svg");
      //monitorOnSVGPixmap = new MSVGIMG(":/svg/headphones_on.svg");

      externSyncOffSVGPixmap = new MSVGIMG(":/svg/extern_sync_off.svg");
      externSyncOnSVGPixmap = new MSVGIMG(":/svg/extern_sync_on.svg");

      masterTrackOffSVGPixmap = new MSVGIMG(":/svg/master_track_off.svg");
      masterTrackOnSVGPixmap = new MSVGIMG(":/svg/master_track_on.svg");

      jackTransportOffSVGPixmap = new MSVGIMG(":/svg/jack_transport_off.svg");
      jackTransportOnSVGPixmap = new MSVGIMG(":/svg/jack_transport_on.svg");

      
      routingInputSVGIcon = new QIcon(*routingInputSVGPixmap);
      routingOutputSVGIcon = new QIcon(*routingOutputSVGPixmap);
      routingInputUnconnectedSVGIcon = new QIcon(*routingInputUnconnectedSVGPixmap);
      routingOutputUnconnectedSVGIcon = new QIcon(*routingOutputUnconnectedSVGPixmap);

      headphonesOffSVGIcon = new QIcon(*headphonesOffSVGPixmap);
      headphonesOnSVGIcon = new QIcon(*headphonesOnSVGPixmap);

      muteOffSVGIcon = new QIcon(*muteOffSVGPixmap);
      muteOnSVGIcon = new QIcon(*muteOnSVGPixmap);
      muteOnXSVGIcon = new QIcon(*muteOnXSVGPixmap);
      muteProxyOnSVGIcon = new QIcon(*muteProxyOnSVGPixmap);
      muteAndProxyOnSVGIcon = new QIcon(*muteAndProxyOnSVGPixmap);

      soloOffSVGIcon = new QIcon(*soloOffSVGPixmap);
      soloOnSVGIcon = new QIcon(*soloOnSVGPixmap);
      soloOnAloneSVGIcon = new QIcon(*soloOnAloneSVGPixmap);
      soloProxyOnSVGIcon = new QIcon(*soloProxyOnSVGPixmap);
      soloProxyOnAloneSVGIcon = new QIcon(*soloProxyOnAloneSVGPixmap);
      soloAndProxyOnSVGIcon = new QIcon(*soloAndProxyOnSVGPixmap);

      trackOffSVGIcon  = new QIcon(*trackOffSVGPixmap);
      trackOnSVGIcon = new QIcon(*trackOnSVGPixmap);

      stereoOffSVGIcon  = new QIcon(*stereoOffSVGPixmap);
      stereoOnSVGIcon = new QIcon(*stereoOnSVGPixmap);

      preFaderOffSVGIcon  = new QIcon(*preFaderOffSVGPixmap);
      preFaderOnSVGIcon = new QIcon(*preFaderOnSVGPixmap);

      recArmOffSVGIcon = new QIcon(*recArmOffSVGPixmap);
      recArmOnSVGIcon = new QIcon(*recArmOnSVGPixmap);

      monitorOffSVGIcon = new QIcon(*monitorOffSVGPixmap);
      monitorOnSVGIcon = new QIcon(*monitorOnSVGPixmap);


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
      
      externSyncOffSVGIcon = new QIcon(*externSyncOffSVGPixmap);
      externSyncOnSVGIcon = new QIcon(*externSyncOnSVGPixmap);
      
      masterTrackOffSVGIcon = new QIcon(*masterTrackOffSVGPixmap);
      masterTrackOnSVGIcon = new QIcon(*masterTrackOnSVGPixmap);
      
      jackTransportOffSVGIcon = new QIcon(*jackTransportOffSVGPixmap);
      jackTransportOnSVGIcon = new QIcon(*jackTransportOnSVGPixmap);
      
      metronomeOffSVGIcon = new QIcon(":/svg/metronome_off.svg");
      metronomeOnSVGIcon = new QIcon(":/svg/metronome_on.svg");
      
      fixedSpeedSVGIcon = new QIcon(":/svg/speed_off.svg");
      fixedSpeedSVGIcon->addFile(":/svg/speed_on.svg", QSize(), QIcon::Normal, QIcon::On);
      transportAffectsLatencySVGIcon = new QIcon(":/svg/transport_affects_latency_off.svg");
      transportAffectsLatencySVGIcon->addFile(":/svg/transport_affects_latency_on.svg", QSize(), QIcon::Normal, QIcon::On);
      overrideLatencySVGIcon = new QIcon(":/svg/override_latency_off.svg");
      overrideLatencySVGIcon->addFile(":/svg/override_latency_on.svg", QSize(), QIcon::Normal, QIcon::On);

      panicSVGIcon = new QIcon(":/svg/panic.svg");
      loopSVGIcon = new QIcon(":/svg/loop.svg");
      punchinSVGIcon = new QIcon(":/svg/punchin.svg");
      punchoutSVGIcon = new QIcon(":/svg/punchout.svg");
      undoSVGIcon = new QIcon(":/svg/undo.svg");
      redoSVGIcon = new QIcon(":/svg/redo.svg");

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
      magnetIconSVG     = new QIcon(":/svg/magnet.svg");

      noscaleSVGIcon[0] = new QIcon(":/svg/noscale1.svg");
      noscaleSVGIcon[1] = new QIcon(":/svg/noscale2.svg");
      noscaleSVGIcon[2] = new QIcon(":/svg/noscale3.svg");

      //----------------------------------
      // Cursors
      //----------------------------------

      editpasteSCursor = new QCursor(*editpasteSIcon, 8, 8);
      editpasteCloneSCursor = new QCursor(*editpasteCloneSIcon, 8, 8);

      // tool cursors
      pencilCursor     = new QCursor(pencilIconSVG->pixmap(DEFCURSIZE), qRound(dpr * 1), qRound(dpr * 17));
      glueCursor       = new QCursor(glueIconSVG->pixmap(DEFCURSIZE),  qRound(dpr * 1), qRound(dpr * 17));
      cutterCursor     = new QCursor(cutterIconSVG->pixmap(DEFCURSIZE),  qRound(dpr * 1), qRound(dpr * 17));
      zoomCursor       = new QCursor(zoomIconSVG->pixmap(DEFCURSIZE));
      zoomAtCursor     = new QCursor(zoomAtIconSVG->pixmap(DEFCURSIZE));
      deleteCursor     = new QCursor(deleteIconSVG->pixmap(DEFCURSIZE), qRound(dpr * 3), qRound(dpr * 15));
      drawCursor       = new QCursor(drawIconSVG->pixmap(DEFCURSIZE), qRound(dpr * 1), qRound(dpr * 17));
      mutePartsCursor  = new QCursor(mutePartsIconSVG->pixmap(DEFCURSIZE));
      handCursor       = new QCursor(handIconSVG->pixmap(DEFCURSIZE));
      closedHandCursor = new QCursor(closedHandIconSVG->pixmap(DEFCURSIZE));
      magnetCursor     = new QCursor(magnetIconSVG->pixmap(DEFCURSIZE), -1, qRound(dpr * 15));
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
      delete note1Icon;    
      delete synthIcon;    
      delete markIcon[0];  
      delete markIcon[1];  
      delete markIcon[2];  
      delete steprecIcon;  
      delete cursorIcon;   
      delete saveIcon;     
      delete saveasIcon;   
      delete openIcon;     
      delete saveIconS;    
      delete openIconS;    
      delete filenewIcon;  
      delete filecloseIcon;  
      delete appexitIcon;  
      delete muteIcon;
      delete upIcon;       
      delete downIcon;     
      delete midiinIcon;
      delete sysexIcon; 
      delete ctrlIcon;  
      delete metaIcon;  
      delete flagIcon;  
      delete flagIconS; 
      delete lockIcon;  

      delete speakerIcon;    
      delete buttondownIcon;

      delete editcutIconSet;     
      delete editcopyIconSet;    
      delete editpasteIconSet;   
      delete editpaste2TrackIconSet;
      delete editpasteCloneIconSet;
      delete editpasteClone2TrackIconSet;

      delete editpasteSIcon;
      delete editpasteCloneSIcon;

      delete exitIcon;             

      delete routesInIcon;         
      delete routesOutIcon;        
      delete routesMidiInIcon;     
      delete routesMidiOutIcon;    

      delete toggle_small_Icon;

      delete ledGreenIcon;
      delete ledDarkGreenIcon;

      delete greendotIcon;         
      delete reddotIcon;
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

      delete sineIcon;                                      
      delete sawIcon;                                       
      
      delete cpuIcon;

      delete routerFilterSourceIcon;
      delete routerFilterDestinationIcon;
      delete routerFilterSourceRoutesIcon;
      delete routerFilterDestinationRoutesIcon;
      delete routerViewSplitterIcon;


      //----------------------------------
      //   SVG...
      //----------------------------------

      delete routingInputSVGPixmap;
      delete routingOutputSVGPixmap;
      delete routingInputUnconnectedSVGPixmap;
      delete routingOutputUnconnectedSVGPixmap;

      delete headphonesOffSVGPixmap;
      delete headphonesOnSVGPixmap;

      delete muteOffSVGPixmap;
      delete muteOnSVGPixmap;
      delete muteOnXSVGPixmap;

      delete soloOffSVGPixmap;
      delete soloOnSVGPixmap;
      delete soloOnAloneSVGPixmap;
      delete soloProxyOnSVGPixmap;
      delete soloProxyOnAloneSVGPixmap;
      delete soloAndProxyOnSVGPixmap;

      delete trackOffSVGPixmap;
      delete trackOnSVGPixmap;

      delete stereoOffSVGPixmap;
      delete stereoOnSVGPixmap;

      delete preFaderOffSVGPixmap;
      delete preFaderOnSVGPixmap;

      delete recArmOffSVGPixmap;
      delete recArmOnSVGPixmap;

      delete monitorOffSVGPixmap;
      delete monitorOnSVGPixmap;
      
      delete externSyncOffSVGPixmap;
      delete externSyncOnSVGPixmap;

      delete masterTrackOffSVGPixmap;
      delete masterTrackOnSVGPixmap;

      delete jackTransportOffSVGPixmap;
      delete jackTransportOnSVGPixmap;


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
      delete magnetIconSVG;

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
      delete magnetCursor;
      }

} // namespace MusEGui
