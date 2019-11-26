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
//#include "xpm/edit_midi.xpm"
#include "xpm/midi_edit_instrument.xpm"
#include "xpm/midi_init_instr.xpm"
#include "xpm/midi_inputplugins.xpm"
#include "xpm/midi_inputplugins_midi_input_filter.xpm"
#include "xpm/midi_inputplugins_midi_input_transform.xpm"
#include "xpm/midi_inputplugins_remote_control.xpm"
#include "xpm/midi_inputplugins_transpose.xpm"
//#include "xpm/midi_inputplugins_random_rhythm_generator.xpm"
#include "xpm/midi_local_off.xpm"
#include "xpm/midi_reset_instr.xpm"
//#include "xpm/midi_thru_off5.xpm"
//#include "xpm/midi_thru_on5.xpm"
#include "xpm/settings_appearance_settings.xpm"
#include "xpm/settings_configureshortcuts.xpm"
//#include "xpm/settings_follow_song.xpm"
#include "xpm/settings_globalsettings.xpm"
#include "xpm/settings_midifileexport.xpm"
#include "xpm/settings_midiport_softsynths.xpm"
#include "xpm/settings_midisync.xpm"
#include "xpm/view_bigtime_window.xpm"
//#include "xpm/view_cliplist.xpm"
#include "xpm/view_marker.xpm"
//#include "xpm/view_mixer.xpm"
#include "xpm/view_transport_window.xpm"

#include "xpm/delete.xpm"
//#include "xpm/play.xpm"
//#include "xpm/closed_hand.xpm"
//#include "xpm/size_all.xpm"
//#include "xpm/midi_ctrl_graph_merge.xpm"
#include "xpm/midi_ctrl_graph_merge_erase.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_inclusive.xpm"
#include "xpm/midi_ctrl_graph_merge_erase_wysiwyg.xpm"
//#include "xpm/midi_ctrl_graph_merge_copy.xpm"
//#include "xpm/midi_ctrl_graph_merge_copy_erase.xpm"
//#include "xpm/midi_ctrl_graph_merge_copy_erase_inclusive.xpm"
//#include "xpm/midi_ctrl_graph_merge_copy_erase_wysiwyg.xpm"

#include "xpm/record1.xpm"
//#include "xpm/record.xpm"
//#include "xpm/record_on.xpm"
//#include "xpm/record_off.xpm"
//#include "xpm/stop.xpm"
//#include "xpm/start.xpm"
//#include "xpm/fforward.xpm"
//#include "xpm/frewind.xpm"
#include "xpm/dot.xpm"
#include "xpm/doth.xpm"
#include "xpm/dot1.xpm"
//#include "xpm/note.xpm"
#include "xpm/note1.xpm"
//#include "xpm/stick.xpm"
//#include "xpm/wave.xpm"
#include "xpm/synth.xpm"
#include "xpm/cmark.xpm"
#include "xpm/lmark.xpm"
#include "xpm/rmark.xpm"
#include "xpm/steprec.xpm"
#include "xpm/cursor.xpm"
//#include "xpm/quant.xpm"
//#include "xpm/fileprint.xpm"
#include "xpm/filesave.xpm"
#include "xpm/filesaveas.xpm"
#include "xpm/fileopen.xpm"
//#include "xpm/fileprintS.xpm"
#include "xpm/filesaveS.xpm"
//#include "xpm/filesaveasS.xpm"
#include "xpm/fileopenS.xpm"
//#include "xpm/master.xpm"
#include "xpm/filenew.xpm"
//#include "xpm/filenewS.xpm"
#include "xpm/fileclose.xpm"
#include "xpm/appexit.xpm"
//#include "xpm/home.xpm"
//#include "xpm/back.xpm"
//#include "xpm/forward.xpm"

//#include "xpm/solobutton_on.xpm"
//#include "xpm/solobutton_off.xpm"
//#include "xpm/solobutton_on_blksq.xpm"
//#include "xpm/solobutton_off_blksq.xpm"
//#include "xpm/mutebutton_on.xpm"
//#include "xpm/mutebutton_off.xpm"
//#include "xpm/rec_echo_on.xpm"
//#include "xpm/rec_echo_off.xpm"
#include "xpm/routing_input_button_slim_4.xpm"
#include "xpm/routing_output_button_slim_4.xpm"
#include "xpm/routing_midi_input_button_slim.xpm"
#include "xpm/routing_midi_output_button_slim.xpm"

#include "xpm/eye.xpm"
#include "xpm/eye_gray.xpm"
#include "xpm/eye_crossed.xpm"

#include "xpm/up.xpm"
#include "xpm/down.xpm"
//#include "xpm/bold.xpm"
//#include "xpm/italic.xpm"
//#include "xpm/underlined.xpm"
//#include "xpm/gv.xpm"
#include "xpm/midiin.xpm"
#include "xpm/sysex.xpm"
#include "xpm/ctrl.xpm"
#include "xpm/meta.xpm"
//#include "xpm/pitch.xpm"
//#include "xpm/cafter.xpm"
//#include "xpm/pafter.xpm"
#include "xpm/flag.xpm"
#include "xpm/flagS.xpm"
#include "xpm/lock.xpm"
//#include "xpm/toc.xpm"
//// #include "xpm/piano.xpm" // not used -Orcan
//// #include "xpm/pianoS.xpm"
//#include "xpm/exitS.xpm"

#include "xpm/editcutS.xpm"
#include "xpm/editcopyS.xpm"
#include "xpm/editpasteS.xpm"
//#include "xpm/editmute.xpm"
#include "xpm/editmuteS.xpm"
#include "xpm/editpastecloneS.xpm"
#include "xpm/editpaste2trackS.xpm"
#include "xpm/editpasteclone2trackS.xpm"

/* Not used - Orcan
#include "xpm/editcut.xpm"
#include "xpm/editcopy.xpm"
#include "xpm/editpaste.xpm"
#include "xpm/editpasteclone.xpm"
#include "xpm/editpaste2track.xpm"
#include "xpm/editpasteclone2track.xpm"
*/
#include "xpm/speaker.xpm"
#include "xpm/buttondown.xpm"
//#include "xpm/configure.xpm"

// next two lines will vanish soon
//#include "xpm/solobutton.xpm"
//#include "xpm/newmutebutton.xpm"
#include "xpm/exit.xpm"
//#include "xpm/exit1.xpm"

#include "xpm/toggle_small.xpm"
//#include "xpm/redled.xpm"
//#include "xpm/darkredled.xpm"
#include "xpm/greendot.xpm"
#include "xpm/greendot12x12.xpm"
#include "xpm/reddot.xpm"
//#include "xpm/darkgreendot.xpm"
#include "xpm/bluedot.xpm"
#include "xpm/bluedot12x12.xpm"
//#include "xpm/graydot.xpm"
#include "xpm/graydot12x12.xpm"
#include "xpm/orangedot.xpm"
#include "xpm/orangedot12x12.xpm"
//#include "xpm/off.xpm"
//#include "xpm/blacksquare.xpm"
//#include "xpm/blacksqcheck.xpm"
//#include "xpm/checksquare.xpm"

//#include "xpm/mastertrackS.xpm"
//#include "xpm/localoffS.xpm"
//#include "xpm/miditransformS.xpm"
//#include "xpm/midi_plugS.xpm"
//#include "xpm/miditransposeS.xpm"
#include "xpm/mixerS.xpm"
//#include "xpm/mustangS.xpm"
//#include "xpm/resetS.xpm"
//#include "xpm/track_add.xpm"
//#include "xpm/track_delete.xpm"
//#include "xpm/listS.xpm"
//#include "xpm/inputpluginS.xpm"
#include "xpm/cliplistS.xpm"
//#include "xpm/mixeraudioS.xpm"
//#include "xpm/initS.xpm"
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
//#include "xpm/midi_transpose.xpm"
#include "xpm/select.xpm"
#include "xpm/select_all.xpm"
#include "xpm/select_all_parts_on_track.xpm"
#include "xpm/select_deselect_all.xpm"
#include "xpm/select_inside_loop.xpm"
#include "xpm/select_invert_selection.xpm"
#include "xpm/select_outside_loop.xpm"

//#include "xpm/mono.xpm"
//#include "xpm/stereo.xpm"
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
//QPixmap* mastertrackSIcon;
//QPixmap* localoffSIcon;
//QPixmap* miditransformSIcon;
//QPixmap* midi_plugSIcon;
//QPixmap* miditransposeSIcon;
//QPixmap* midiThruOnIcon;
//QPixmap* midiThruOffIcon;
QPixmap* mixerSIcon;
//QPixmap* mustangSIcon;
//QPixmap* resetSIcon;
//QPixmap* track_addIcon;
//QPixmap* track_deleteIcon;
//QPixmap* listSIcon;
//QPixmap* inputpluginSIcon;
QPixmap* cliplistSIcon;
//QPixmap* mixerAudioSIcon;
//QPixmap* initSIcon;
QPixmap* deltaOnIcon;
QPixmap* deltaOffIcon;
QPixmap* veloPerNote_OnIcon;
QPixmap* veloPerNote_OffIcon;

QPixmap* exitIcon;
//QPixmap* exit1Icon;
//QPixmap* newmuteIcon;
//QPixmap* soloIcon;

QPixmap* routesInIcon;
QPixmap* routesOutIcon;
QPixmap* routesMidiInIcon;
QPixmap* routesMidiOutIcon;

QPixmap* deleteIcon;
//QPixmap* playIcon;
//QPixmap* closedHandIcon;
//QPixmap* sizeAllIcon;
//QPixmap* midiCtrlMergeIcon;
QPixmap* midiCtrlMergeEraseIcon;
QPixmap* midiCtrlMergeEraseInclusiveIcon;
QPixmap* midiCtrlMergeEraseWysiwygIcon;
//QPixmap* midiCtrlMergeCopyIcon;
//QPixmap* midiCtrlMergeCopyEraseIcon;
//QPixmap* midiCtrlMergeCopyEraseInclusiveIcon;
//QPixmap* midiCtrlMergeCopyEraseWysiwygIcon;

QPixmap* record1_Icon;
//QPixmap* record_on_Icon;
//QPixmap* record_off_Icon;
//QPixmap* recordIcon;
//QPixmap* stopIcon;
//QPixmap* startIcon;
//QPixmap* fforwardIcon;
//QPixmap* frewindIcon;
QPixmap* dotIcon;
QPixmap* dothIcon;
QPixmap* dot1Icon;
QPixmap* note1Icon;
//QPixmap* noteIcon;
//QPixmap* stickIcon;
//QPixmap* waveIcon;
QPixmap* synthIcon;
QPixmap* markIcon[3];
QPixmap* steprecIcon;
QPixmap* cursorIcon;
//QPixmap* quantIcon;
//QPixmap* printIcon;
//QPixmap* printIconS;
QPixmap* openIcon;
QPixmap* saveIcon;
QPixmap* saveasIcon;
QPixmap* openIconS;
QPixmap* saveIconS;
//QPixmap* saveasIconS;
//QPixmap* masterIcon;
QPixmap* filenewIcon;
//QPixmap* filenewIconS;
QPixmap* filecloseIcon;
QPixmap* appexitIcon;
//QPixmap* homeIcon;
//QPixmap* backIcon;
//QPixmap* forwardIcon;
QPixmap* muteIcon;
QPixmap* eyeIcon;
QPixmap* eyeCrossedIcon;
QPixmap* eyeGrayIcon;
QPixmap* upIcon;
QPixmap* downIcon;
//QPixmap* boldIcon;
//QPixmap* italicIcon;
//QPixmap* underlinedIcon;
//QPixmap* gvIcon;
QPixmap* midiinIcon;
QPixmap* sysexIcon;
QPixmap* ctrlIcon;
QPixmap* metaIcon;
//QPixmap* pitchIcon;
//QPixmap* cafterIcon;
//QPixmap* pafterIcon;
QPixmap* flagIcon;
QPixmap* flagIconS;
QPixmap* lockIcon;
//QPixmap* tocIcon;
//QPixmap* exitIconS;

QPixmap* speakerIcon;
QPixmap* buttondownIcon;
//QPixmap* configureIcon;

//QPixmap* recEchoIconOn;
//QPixmap* recEchoIconOff;
//QPixmap* muteIconOn;
//QPixmap* muteIconOff;
//QPixmap* soloIconOn;
//QPixmap* soloIconOff;
//QPixmap* soloblksqIconOn;
//QPixmap* soloblksqIconOff;
//QIcon* soloIconSet1;
//QIcon* soloIconSet2;

//QPixmap* editmuteIcon;
//QPixmap* editmuteSIcon;

QIcon* pianoIconSet;
QIcon* scoreIconSet;
QIcon* editcutIconSet;
//QIcon* editmuteIconSet;
QIcon* editcopyIconSet;
QIcon* editpasteIconSet;
QIcon* editpaste2TrackIconSet;
QIcon* editpasteCloneIconSet;
QIcon* editpasteClone2TrackIconSet;

/* Not used - Orcan
QIcon* pianoIcon;
QIcon* editcutIcon;
QIcon* editcopyIcon;
QIcon* editpasteIcon;
QIcon* editpasteCloneIcon;
QIcon* editpaste2TrackIcon;
QIcon* editpasteClone2TrackIcon;
*/
QPixmap* editpasteSIcon;
QPixmap* editpasteCloneSIcon;

QPixmap* toggle_small_Icon;
//QPixmap* redLedIcon;
//QPixmap* darkRedLedIcon;
QPixmap* greendotIcon;
QPixmap* greendot12x12Icon;
QPixmap* reddotIcon;
//QPixmap* darkgreendotIcon;
//QPixmap* graydotIcon;
QPixmap* graydot12x12Icon;
QPixmap* bluedotIcon;
QPixmap* bluedot12x12Icon;
QPixmap* orangedotIcon;
QPixmap* orangedot12x12Icon;
//QPixmap* offIcon;
//QPixmap* blacksquareIcon;
//QPixmap* blacksqcheckIcon;
//QPixmap* checkSquareIcon;

//QIcon* ledRedIcon;
//QIcon* ledDarkRedIcon;
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
//QPixmap* edit_pianorollIcon;
//QPixmap* edit_scoreIcon;
QPixmap* edit_track_addIcon;
QPixmap* edit_track_delIcon;
QPixmap* mastertrack_graphicIcon;
QPixmap* mastertrack_listIcon;
QPixmap* midi_transformIcon;
//QPixmap* midi_transposeIcon;
QPixmap* selectIcon;
//QIcon*   icon_select;
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
//QPixmap* edit_midiIcon;
QPixmap* midi_edit_instrumentIcon;
QPixmap* midi_init_instrIcon;
QPixmap* midi_inputpluginsIcon;
QPixmap* midi_inputplugins_midi_input_filterIcon;
QPixmap* midi_inputplugins_midi_input_transformIcon;
//QPixmap* midi_inputplugins_random_rhythm_generatorIcon;
QPixmap* midi_inputplugins_remote_controlIcon;
QPixmap* midi_inputplugins_transposeIcon;
QPixmap* midi_local_offIcon;
QPixmap* midi_reset_instrIcon;
QPixmap* settings_appearance_settingsIcon;
QPixmap* settings_configureshortcutsIcon;
//QPixmap* settings_follow_songIcon;
QPixmap* settings_globalsettingsIcon;
QPixmap* settings_midifileexportIcon;
QPixmap* settings_midiport_softsynthsIcon;
QPixmap* settings_midisyncIcon;
QPixmap* view_bigtime_windowIcon;
//QPixmap* view_cliplistIcon;
QPixmap* view_markerIcon;
//QPixmap* view_mixerIcon;
QPixmap* view_transport_windowIcon;

//QPixmap* monoIcon;
//QPixmap* stereoIcon;
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
//QPixmap* sliderPngImage;
//QPixmap* knobPngImage;
//QPixmap* knobBasePngImage;
//QPixmap* rimBasePngImage;
//QPixmap* knobSmallPngImage;

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
//      playIcon     = MPIXMAP(play_xpm, "media-playback-start");
//      closedHandIcon = MPIXMAP(closed_hand_xpm, nullptr);
//      sizeAllIcon  = MPIXMAP(size_all_xpm, nullptr);
//      midiCtrlMergeIcon                   = MPIXMAP(midi_ctrl_graph_merge_xpm, nullptr);
      midiCtrlMergeEraseIcon              = MPIXMAP(midi_ctrl_graph_merge_erase_xpm, nullptr);
      midiCtrlMergeEraseInclusiveIcon     = MPIXMAP(midi_ctrl_graph_merge_erase_inclusive_xpm, nullptr);
      midiCtrlMergeEraseWysiwygIcon       = MPIXMAP(midi_ctrl_graph_merge_erase_wysiwyg_xpm, nullptr);
//      midiCtrlMergeCopyIcon               = MPIXMAP(midi_ctrl_graph_merge_copy_xpm, nullptr);
//      midiCtrlMergeCopyEraseIcon          = MPIXMAP(midi_ctrl_graph_merge_copy_erase_xpm, nullptr);
//      midiCtrlMergeCopyEraseInclusiveIcon = MPIXMAP(midi_ctrl_graph_merge_copy_erase_inclusive_xpm, nullptr);
//      midiCtrlMergeCopyEraseWysiwygIcon   = MPIXMAP(midi_ctrl_graph_merge_copy_erase_wysiwyg_xpm, nullptr);

      record1_Icon  = MPIXMAP(record1_xpm, nullptr);
//      record_on_Icon = MPIXMAP(record_on_xpm, nullptr);
//      record_off_Icon = MPIXMAP(record_off_xpm, nullptr);
//      recordIcon   = MPIXMAP(record_xpm, "media-record");
//      stopIcon     = MPIXMAP(stop_xpm, "media-playback-stop");
//      startIcon    = MPIXMAP(start_xpm, "media-skip-backward");
//      fforwardIcon = MPIXMAP(fforward_xpm, "media-seek-forward");
//      frewindIcon  = MPIXMAP(frewind_xpm, "media-seek-backward");
      dotIcon      = MPIXMAP(dot_xpm, "dialog-ok-apply");
      dothIcon     = MPIXMAP(doth_xpm, "draw-circle");
      dot1Icon     = MPIXMAP(dot1_xpm, nullptr);
//      noteIcon     = MPIXMAP(note_xpm, nullptr);
      note1Icon    = MPIXMAP(note1_xpm, nullptr);
//      stickIcon    = MPIXMAP(stick_xpm, nullptr);
//      waveIcon     = MPIXMAP(wave_xpm, nullptr);
      synthIcon    = MPIXMAP(synth_xpm, nullptr);
      markIcon[0]  = MPIXMAP(cmark_xpm, nullptr);
      markIcon[1]  = MPIXMAP(lmark_xpm, nullptr);
      markIcon[2]  = MPIXMAP(rmark_xpm, nullptr);
      steprecIcon  = MPIXMAP(steprec_xpm, nullptr);
      cursorIcon   = MPIXMAP(cursor_xpm, nullptr);
//      quantIcon    = MPIXMAP(quant_xpm, nullptr);
      saveIcon     = MPIXMAP(filesave_xpm, "document-save");
      saveasIcon     = MPIXMAP(filesaveas_xpm, "document-save-as");
//      printIcon    = MPIXMAP(fileprint_xpm, "document-print");
      openIcon     = MPIXMAP(fileopen_xpm, "document-open");
      saveIconS     = MPIXMAP(filesaveS_xpm, "document-save");
//      saveasIconS     = MPIXMAP(filesaveasS_xpm, "document-save-as");
//      printIconS    = MPIXMAP(fileprintS_xpm, "document-print");
      openIconS     = MPIXMAP(fileopenS_xpm, "document-open");
//      masterIcon   = MPIXMAP(master_xpm, "mixer-master");
      filenewIcon  = MPIXMAP(filenew_xpm, "document-new");
//      filenewIconS  = MPIXMAP(filenewS_xpm, "document-new");
      filecloseIcon  = MPIXMAP(fileclose_xpm, "document-close");
      appexitIcon  = MPIXMAP(appexit_xpm, "application-exit");
//      homeIcon     = MPIXMAP(home_xpm, "user-home");
//      backIcon     = MPIXMAP(back_xpm, "go-previous");
//      forwardIcon  = MPIXMAP(forward_xpm, "go-next");
      muteIcon     = MPIXMAP(editmuteS_xpm, "audio-volume-muted");
      eyeIcon      = MPIXMAP(eye_xpm, nullptr);
      eyeCrossedIcon  = MPIXMAP(eye_crossed_xpm, nullptr);
      eyeGrayIcon  = MPIXMAP(eye_gray_xpm, nullptr);
      upIcon       = MPIXMAP(up_xpm, "go-up");
      downIcon     = MPIXMAP(down_xpm, "go-down");
//      boldIcon     = MPIXMAP(bold_xpm, "format-text-bold");
//      italicIcon     = MPIXMAP(italic_xpm, "format-text-italic");
//      underlinedIcon = MPIXMAP(underlined_xpm, "format-text-underline");
//      gvIcon     = MPIXMAP(gv_xpm, nullptr);
      midiinIcon = MPIXMAP(midiin_xpm, nullptr);
      sysexIcon   = MPIXMAP(sysex_xpm, nullptr);
      ctrlIcon    = MPIXMAP(ctrl_xpm, nullptr);
      metaIcon    = MPIXMAP(meta_xpm, nullptr);
//      pitchIcon   = MPIXMAP(pitch_xpm, nullptr);
//      cafterIcon  = MPIXMAP(cafter_xpm, nullptr);
//      pafterIcon  = MPIXMAP(pafter_xpm, nullptr);
      flagIcon    = MPIXMAP(flag_xpm, nullptr);
      flagIconS   = MPIXMAP(flagS_xpm, nullptr);
      lockIcon    = MPIXMAP(lock_xpm, nullptr);
//      tocIcon     = MPIXMAP(toc_xpm, nullptr);
//      exitIconS   = MPIXMAP(exitS_xpm, "application-exit");

      speakerIcon    = MPIXMAP(speaker_xpm, nullptr);
      buttondownIcon = MPIXMAP(buttondown_xpm, "arrow-down");
//      configureIcon  = MPIXMAP(configure_xpm, nullptr);

//      editmuteIcon  = MPIXMAP(editmute_xpm, nullptr);
//      editmuteSIcon = MPIXMAP(editmuteS_xpm, nullptr);

      editcutIconSet       = MICON(editcutS_xpm, "edit-cut"); // ddskrjo
      editcopyIconSet      = MICON(editcopyS_xpm, "edit-copy");
      editpasteIconSet     = MICON(editpasteS_xpm, "edit-paste");
//      editmuteIconSet      = MICON(editmuteS_xpm, "audio-volume-muted");
      editpaste2TrackIconSet = MICON(editpaste2trackS_xpm, nullptr);
      editpasteCloneIconSet  = MICON(editpastecloneS_xpm, nullptr);
      editpasteClone2TrackIconSet = MICON(editpasteclone2trackS_xpm, nullptr); // ..
      /* Not used - Orcan
      pianoIcon                 = MICON(piano_xpm, nullptr);
      editcutIcon               = MICON(editcut_xpm, "edit-cut");
      editcopyIcon              = MICON(editcopy_xpm, "edit-copy");
      editpasteIcon             = MICON(editpaste_xpm, "edit-paste");
      editpasteCloneIcon        = MICON(editpasteclone_xpm, nullptr);
      editpaste2TrackIcon       = MICON(editpaste2track_xpm, nullptr);
      editpasteClone2TrackIcon  = MICON(editpasteclone2track_xpm, nullptr);
      */
      editpasteSIcon      = MPIXMAP(editpasteS_xpm, nullptr);
      editpasteCloneSIcon = MPIXMAP(editpastecloneS_xpm, nullptr);

      //exitIcon             = MPIXMAP(exit_xpm, "application-exit");
      //exit1Icon            = MPIXMAP(exit1_xpm, "application-exit");
      // Changed by Tim. There are IMO no suitable theme substitutes for these two so far...
      exitIcon             = MPIXMAP(exit_xpm, nullptr);
//      exit1Icon            = MPIXMAP(exit1_xpm, nullptr);

      // 2 lines odd code
//      newmuteIcon          = MPIXMAP(newmutebutton_xpm, nullptr);
//      soloIcon             = MPIXMAP(solobutton_xpm, nullptr);

      routesInIcon         = MPIXMAP(routing_input_button_slim_4_xpm, nullptr);
      routesOutIcon        = MPIXMAP(routing_output_button_slim_4_xpm, nullptr);
      routesMidiInIcon     = MPIXMAP(routing_midi_input_button_slim_xpm, nullptr);
      routesMidiOutIcon    = MPIXMAP(routing_midi_output_button_slim_xpm, nullptr);

//      recEchoIconOn        = MPIXMAP(rec_echo_on_xpm, nullptr);
//      recEchoIconOff       = MPIXMAP(rec_echo_off_xpm, nullptr);
//      muteIconOn           = MPIXMAP(mutebutton_on_xpm, nullptr);
//      muteIconOff          = MPIXMAP(mutebutton_off_xpm, nullptr);
//      soloIconOn           = MPIXMAP(solobutton_on_xpm, nullptr);
//      soloIconOff          = MPIXMAP(solobutton_off_xpm, nullptr);
//      soloblksqIconOn      = MPIXMAP(solobutton_on_blksq_xpm, nullptr);
//      soloblksqIconOff     = MPIXMAP(solobutton_off_blksq_xpm, nullptr);
      //soloIconSet1         = new QIcon(); 
      //soloIconSet2         = new QIcon();
      //soloIconSet1->addPixmap(*soloIconOn, QIcon::Normal, QIcon::On);
      //soloIconSet1->addPixmap(*soloIconOff, QIcon::Normal, QIcon::Off);
      //soloIconSet2->addPixmap(*soloblksqIconOn, QIcon::Normal, QIcon::On);
      //soloIconSet2->addPixmap(*soloblksqIconOff, QIcon::Normal, QIcon::Off);
      
      toggle_small_Icon    = MPIXMAP(toggle_small_xpm, nullptr);
//      redLedIcon           = MPIXMAP(redled_xpm, nullptr);
//      darkRedLedIcon       = MPIXMAP(darkredled_xpm, nullptr);
      
//      ledRedIcon           = new QIcon(*redLedIcon);
//      ledDarkRedIcon       = new QIcon(*darkRedLedIcon);
      
      greendotIcon         = MPIXMAP(greendot_xpm, nullptr);
      greendot12x12Icon    = MPIXMAP(greendot12x12_xpm, nullptr);
      reddotIcon           = MPIXMAP(reddot_xpm, nullptr);
//      darkgreendotIcon     = MPIXMAP(darkgreendot_xpm, nullptr);
      
      ledGreenIcon         = new QIcon(*greendotIcon);
//      ledDarkGreenIcon     = new QIcon(*darkgreendotIcon);
      
      bluedotIcon          = MPIXMAP(bluedot_xpm, nullptr);
      bluedot12x12Icon     = MPIXMAP(bluedot12x12_xpm, nullptr);
//      graydotIcon          = MPIXMAP(graydot_xpm, nullptr);
      graydot12x12Icon     = MPIXMAP(graydot12x12_xpm, nullptr);
      orangedotIcon        = MPIXMAP(orangedot_xpm, nullptr);
      orangedot12x12Icon   = MPIXMAP(orangedot12x12_xpm, nullptr);
//      offIcon              = MPIXMAP(off_xpm, nullptr);
//      blacksquareIcon      = MPIXMAP(blacksquare_xpm, nullptr);
//      blacksqcheckIcon     = MPIXMAP(blacksqcheck_xpm, nullptr);
//      checkSquareIcon      = MPIXMAP(checksquare_xpm, nullptr);

//      mastertrackSIcon     = MPIXMAP(mastertrackS_xpm, nullptr);
//      localoffSIcon        = MPIXMAP(localoffS_xpm, nullptr);
//      miditransformSIcon   = MPIXMAP(miditransformS_xpm, nullptr);
//      midi_plugSIcon       = MPIXMAP(midi_plugS_xpm, nullptr);
//      miditransposeSIcon   = MPIXMAP(miditransposeS_xpm, nullptr);
//      midiThruOnIcon       = MPIXMAP(midi_thru_on5_xpm, nullptr);
//      midiThruOffIcon      = MPIXMAP(midi_thru_off5_xpm, nullptr);
      
      mixerSIcon           = MPIXMAP(mixerS_xpm, nullptr);
//      mustangSIcon         = MPIXMAP(mustangS_xpm, nullptr);
//      resetSIcon           = MPIXMAP(resetS_xpm, nullptr);
//      track_addIcon        = MPIXMAP(track_add_xpm, nullptr);
//      track_deleteIcon     = MPIXMAP(track_delete_xpm, nullptr);
//      listSIcon            = MPIXMAP(listS_xpm, nullptr);
//      inputpluginSIcon     = MPIXMAP(inputpluginS_xpm, nullptr);
      cliplistSIcon        = MPIXMAP(cliplistS_xpm, nullptr);
//      mixerAudioSIcon      = MPIXMAP(mixerAudioS_xpm, nullptr);
//      initSIcon            = MPIXMAP(initS_xpm, nullptr);
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
//      edit_pianorollIcon            = MPIXMAP(edit_pianoroll_xpm, nullptr);
//      edit_scoreIcon                = MPIXMAP(edit_score_xpm, nullptr);
      edit_track_addIcon            = MPIXMAP(edit_track_add_xpm, nullptr);
      edit_track_delIcon            = MPIXMAP(edit_track_del_xpm, nullptr);
      mastertrack_graphicIcon       = MPIXMAP(mastertrack_graphic_xpm, nullptr);
      mastertrack_listIcon          = MPIXMAP(mastertrack_list_xpm, nullptr);
      midi_transformIcon            = MPIXMAP(midi_transform_xpm, nullptr);
//      midi_transposeIcon            = MPIXMAP(midi_transpose_xpm, nullptr);
      selectIcon                    = MPIXMAP(select_xpm, nullptr);
//      icon_select                   = new QIcon(*selectIcon);
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
//      edit_midiIcon                                 = MPIXMAP(edit_midi_xpm, nullptr);
      midi_edit_instrumentIcon                      = MPIXMAP(midi_edit_instrument_xpm, nullptr);
      midi_init_instrIcon                           = MPIXMAP(midi_init_instr_xpm, nullptr);
      midi_inputpluginsIcon                         = MPIXMAP(midi_inputplugins_xpm, nullptr);
      midi_inputplugins_midi_input_filterIcon       = MPIXMAP(midi_inputplugins_midi_input_filter_xpm, nullptr);
      midi_inputplugins_midi_input_transformIcon    = MPIXMAP(midi_inputplugins_midi_input_transform_xpm, nullptr);
//      midi_inputplugins_random_rhythm_generatorIcon = MPIXMAP(midi_inputplugins_random_rhythm_generator_xpm, nullptr);
      midi_inputplugins_remote_controlIcon          = MPIXMAP(midi_inputplugins_remote_control_xpm, nullptr);
      midi_inputplugins_transposeIcon               = MPIXMAP(midi_inputplugins_transpose_xpm, nullptr);
      midi_local_offIcon                            = MPIXMAP(midi_local_off_xpm, nullptr);
      midi_reset_instrIcon                          = MPIXMAP(midi_reset_instr_xpm, nullptr);
      settings_appearance_settingsIcon              = MPIXMAP(settings_appearance_settings_xpm, nullptr);
      settings_configureshortcutsIcon               = MPIXMAP(settings_configureshortcuts_xpm, nullptr);
//      settings_follow_songIcon                      = MPIXMAP(settings_follow_song_xpm, nullptr);
      settings_globalsettingsIcon                   = MPIXMAP(settings_globalsettings_xpm, nullptr);
      settings_midifileexportIcon                   = MPIXMAP(settings_midifileexport_xpm, nullptr);
      settings_midiport_softsynthsIcon              = MPIXMAP(settings_midiport_softsynths_xpm, nullptr);
      settings_midisyncIcon                         = MPIXMAP(settings_midisync_xpm, nullptr);
      view_bigtime_windowIcon                       = MPIXMAP(view_bigtime_window_xpm, nullptr);
//      view_cliplistIcon                             = MPIXMAP(view_cliplist_xpm, nullptr);
      view_markerIcon                               = MPIXMAP(view_marker_xpm, nullptr);
//      view_mixerIcon                                = MPIXMAP(view_mixer_xpm, nullptr);
      view_transport_windowIcon                     = MPIXMAP(view_transport_window_xpm, nullptr);

//      monoIcon                                      = MPIXMAP(mono_xpm, nullptr);
//      stereoIcon                                    = MPIXMAP(stereo_xpm, nullptr);

      museIcon                                      = MPIXMAP(muse_icon_xpm, nullptr);
      aboutMuseImage                                = MPIXMAP(about_muse_xpm, nullptr);
      museLeftSideLogo                              = MPIXMAP(muse_leftside_logo_xpm, nullptr);
      globalIcon                                    = MICON(global_xpm, "folder");
      userIcon                                      = MICON(user_xpm, "user-home");
      projectIcon                                   = MICON(project_xpm, "folder-sound");

      sineIcon                                      = MPIXMAP(sine_xpm, nullptr);
      sawIcon                                       = MPIXMAP(saw_xpm, nullptr);

      pianoNewIcon                                       = MICON(pianoNew_xpm, nullptr);
      presetsNewIcon                                       = MICON(presetsNew_xpm, nullptr);

      cpuIcon                                       = MICON(cpu_xpm, nullptr);
//      sliderPngImage                                = new MPNGIMG(":/png/slider-vol.png");

//      knobPngImage                                       = new MPNGIMG(":/png/knob.png");
//      knobBasePngImage                                   = new MPNGIMG(":/png/knob-base.png");
//      knobSmallPngImage                                  = new MPNGIMG(":/png/knob-small.png");
//      rimBasePngImage                                    = new MPNGIMG(":/png/rim-base.png");

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
//      delete playIcon;
//      delete closedHandIcon;
//      delete sizeAllIcon;
//      delete midiCtrlMergeIcon;
      delete midiCtrlMergeEraseIcon;
      delete midiCtrlMergeEraseInclusiveIcon;
      delete midiCtrlMergeEraseWysiwygIcon;
//      delete midiCtrlMergeCopyIcon;
//      delete midiCtrlMergeCopyEraseIcon;
//      delete midiCtrlMergeCopyEraseInclusiveIcon;
//      delete midiCtrlMergeCopyEraseWysiwygIcon;

      delete record1_Icon;
//      delete record_on_Icon;
//      delete record_off_Icon;
//      delete recordIcon;
//      delete stopIcon;
//      delete startIcon;
//      delete fforwardIcon;
//      delete frewindIcon;
      delete dotIcon;      
      delete dothIcon;     
      delete dot1Icon;     
//      delete noteIcon;
      delete note1Icon;    
//      delete stickIcon;
//      delete waveIcon;
      delete synthIcon;    
      delete markIcon[0];  
      delete markIcon[1];  
      delete markIcon[2];  
      delete steprecIcon;  
      delete cursorIcon;   
//      delete quantIcon;
      delete saveIcon;     
      delete saveasIcon;   
//      delete printIcon;
      delete openIcon;     
      delete saveIconS;    
//      delete saveasIconS;
//      delete printIconS;
      delete openIconS;    
//      delete masterIcon;
      delete filenewIcon;  
//      delete filenewIconS;
      delete filecloseIcon;  
      delete appexitIcon;  
//      delete homeIcon;
//      delete backIcon;
//      delete forwardIcon;
      delete muteIcon;
      delete upIcon;       
      delete downIcon;     
//      delete boldIcon;
//      delete italicIcon;
//      delete underlinedIcon;
//      delete gvIcon;
      delete midiinIcon;
      delete sysexIcon; 
      delete ctrlIcon;  
      delete metaIcon;  
//      delete pitchIcon;
//      delete cafterIcon;
//      delete pafterIcon;
      delete flagIcon;  
      delete flagIconS; 
      delete lockIcon;  
//      delete tocIcon;
//      delete exitIconS;

//      delete undoIcon;
//      delete redoIcon;
//      delete undoIconS;
//      delete redoIconS;

      delete speakerIcon;    
      delete buttondownIcon;
//      delete configureIcon;

//      delete editmuteIcon;
//      delete editmuteSIcon;

      delete editcutIconSet;     
      delete editcopyIconSet;    
      delete editpasteIconSet;   
//      delete editmuteIconSet;
      delete editpaste2TrackIconSet;
      delete editpasteCloneIconSet;
      delete editpasteClone2TrackIconSet;

      /* Not used - Orcan
      delete pianoIcon;                 
      delete editcutIcon;               
      delete editcopyIcon;              
      delete editpasteIcon;             
      delete editpasteCloneIcon;        
      delete editpaste2TrackIcon;       
      delete editpasteClone2TrackIcon; 
      */
      delete editpasteSIcon;
      delete editpasteCloneSIcon;

      delete exitIcon;             
//      delete exit1Icon;

      // 2 lines odd code
//      delete newmuteIcon;
//      delete soloIcon;

      delete routesInIcon;         
      delete routesOutIcon;        
      delete routesMidiInIcon;     
      delete routesMidiOutIcon;    

//      delete recEchoIconOn;
//      delete recEchoIconOff;
//      delete muteIconOn;
//      delete muteIconOff;
//      delete soloIconOn;
//      delete soloIconOff;
//      delete soloblksqIconOn;
//      delete soloblksqIconOff;
      //delete soloIconSet1       
      //delete soloIconSet2       

      delete toggle_small_Icon;

//      delete ledRedIcon;
//      delete ledDarkRedIcon;
      delete ledGreenIcon;
      delete ledDarkGreenIcon;

//      delete redLedIcon;
//      delete darkRedLedIcon;
      delete greendotIcon;         
      delete reddotIcon;
//      delete darkgreendotIcon;
      delete bluedotIcon;          
//      delete graydotIcon;
      delete orangedotIcon;          
//      delete offIcon;
//      delete blacksquareIcon;
//      delete blacksqcheckIcon;
//      delete checkSquareIcon;
      
//      delete mastertrackSIcon;
//      delete localoffSIcon;
//      delete miditransformSIcon;
//      delete midi_plugSIcon;
//      delete miditransposeSIcon;
//      delete midiThruOnIcon;
//      delete midiThruOffIcon;

      delete mixerSIcon;           
//      delete mustangSIcon;
//      delete resetSIcon;
//      delete track_addIcon;
//      delete track_deleteIcon;
//      delete listSIcon;
//      delete inputpluginSIcon;
      delete cliplistSIcon;        
//      delete mixerAudioSIcon;
//      delete initSIcon;
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
//      delete edit_pianorollIcon;
//      delete edit_scoreIcon;
      delete edit_track_addIcon;            
      delete edit_track_delIcon;            
      delete mastertrack_graphicIcon;       
      delete mastertrack_listIcon;          
      delete midi_transformIcon;            
//      delete midi_transposeIcon;
      delete selectIcon;                    
//      delete icon_select;
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
//      delete edit_midiIcon;
      delete midi_edit_instrumentIcon;      
      delete midi_init_instrIcon;           
      delete midi_inputpluginsIcon;         
      delete midi_inputplugins_midi_input_filterIcon;       
      delete midi_inputplugins_midi_input_transformIcon;    
//      delete midi_inputplugins_random_rhythm_generatorIcon;
      delete midi_inputplugins_remote_controlIcon;          
      delete midi_inputplugins_transposeIcon;               
      delete midi_local_offIcon;                            
      delete midi_reset_instrIcon;                          
      delete settings_appearance_settingsIcon;              
      delete settings_configureshortcutsIcon;               
//      delete settings_follow_songIcon;
      delete settings_globalsettingsIcon;                   
      delete settings_midifileexportIcon;                   
      delete settings_midiport_softsynthsIcon;              
      delete settings_midisyncIcon;                         
      delete view_bigtime_windowIcon;                       
//      delete view_cliplistIcon;
      delete view_markerIcon;                               
//      delete view_mixerIcon;
      delete view_transport_windowIcon;                     

//      delete monoIcon;
//      delete stereoIcon;

      delete museIcon;                                      
      delete aboutMuseImage;                                
      delete museLeftSideLogo;                              
      delete globalIcon;
      delete userIcon;
      delete projectIcon;

      delete sineIcon;                                      
      delete sawIcon;                                       
      
      delete cpuIcon;

//      delete sliderPngImage;

//      delete knobPngImage;
//      delete knobBasePngImage;
//      delete knobSmallPngImage;
//      delete rimBasePngImage;
      
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
