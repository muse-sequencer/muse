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

#include "globals.h"

#include <QIcon>

#include "xpm/track_comment.xpm"
#include "xpm/audio_bounce_to_file.xpm"
#include "xpm/audio_bounce_to_track.xpm"
#include "xpm/audio_restartaudio.xpm"
#include "xpm/automation_clear_data.xpm"
#include "xpm/automation_mixer.xpm"
#include "xpm/automation_take_snapshot.xpm"
#include "xpm/edit_midi.xpm"
#include "xpm/midi_edit_instrument.xpm"
#include "xpm/midi_init_instr.xpm"
#include "xpm/midi_inputplugins.xpm"
#include "xpm/midi_inputplugins_midi_input_filter.xpm"
#include "xpm/midi_inputplugins_midi_input_transform.xpm"
#include "xpm/midi_inputplugins_remote_control.xpm"
#include "xpm/midi_inputplugins_transpose.xpm"
#include "xpm/midi_inputplugins_random_rhythm_generator.xpm"
#include "xpm/midi_local_off.xpm"
#include "xpm/midi_reset_instr.xpm"
#include "xpm/midi_thru_off4.xpm"
#include "xpm/midi_thru_on4.xpm"
#include "xpm/settings_appearance_settings.xpm"
#include "xpm/settings_configureshortcuts.xpm"
#include "xpm/settings_follow_song.xpm"
#include "xpm/settings_globalsettings.xpm"
#include "xpm/settings_metronome.xpm"
#include "xpm/settings_midifileexport.xpm"
#include "xpm/settings_midiport_softsynths.xpm"
#include "xpm/settings_midisync.xpm"
#include "xpm/view_bigtime_window.xpm"
#include "xpm/view_cliplist.xpm"
#include "xpm/view_marker.xpm"
#include "xpm/view_mixer.xpm"
#include "xpm/view_transport_window.xpm"

#include "xpm/pointer.xpm"
#include "xpm/pencil.xpm"
#include "xpm/delete.xpm"
#include "xpm/play.xpm"

#include "xpm/record1.xpm"
#include "xpm/record.xpm"
#include "xpm/record_on.xpm"
#include "xpm/record_off.xpm"
#include "xpm/stop.xpm"
#include "xpm/start.xpm"
#include "xpm/fforward.xpm"
#include "xpm/frewind.xpm"
#include "xpm/punchin.xpm"
#include "xpm/punchout.xpm"
#include "xpm/punchin1.xpm"
#include "xpm/punchout1.xpm"
#include "xpm/loop1.xpm"
#include "xpm/loop.xpm"
#include "xpm/dot.xpm"
#include "xpm/doth.xpm"
#include "xpm/dot1.xpm"
#include "xpm/note.xpm"
#include "xpm/note1.xpm"
#include "xpm/stick.xpm"
#include "xpm/wave.xpm"
#include "xpm/synth.xpm"
#include "xpm/cmark.xpm"
#include "xpm/lmark.xpm"
#include "xpm/rmark.xpm"
#include "xpm/cut.xpm"
#include "xpm/steprec.xpm"
#include "xpm/glue.xpm"
#include "xpm/draw.xpm"
#include "xpm/cursor.xpm"
#include "xpm/quant.xpm"
#include "xpm/fileprint.xpm"
#include "xpm/filesave.xpm"
#include "xpm/filesaveas.xpm"
#include "xpm/fileopen.xpm"
#include "xpm/fileprintS.xpm"
#include "xpm/filesaveS.xpm"
#include "xpm/filesaveasS.xpm"
#include "xpm/fileopenS.xpm"
#include "xpm/master.xpm"
#include "xpm/filenew.xpm"
#include "xpm/filenewS.xpm"
#include "xpm/home.xpm"
#include "xpm/back.xpm"
#include "xpm/forward.xpm"

// #include "xpm/mute.xpm"
#include "xpm/solobutton_on.xpm"
#include "xpm/solobutton_off.xpm"
#include "xpm/solobutton_on_blksq.xpm"
#include "xpm/solobutton_off_blksq.xpm"
#include "xpm/mutebutton_on.xpm"
#include "xpm/mutebutton_off.xpm"
#include "xpm/rec_echo_on.xpm"
#include "xpm/rec_echo_off.xpm"
#include "xpm/routing_input_button_slim_4.xpm"
#include "xpm/routing_output_button_slim_4.xpm"
#include "xpm/routing_midi_input_button_slim.xpm"
#include "xpm/routing_midi_output_button_slim.xpm"

#include "xpm/up.xpm"
#include "xpm/down.xpm"
#include "xpm/bold.xpm"
#include "xpm/italic.xpm"
#include "xpm/underlined.xpm"
#include "xpm/gv.xpm"
#include "xpm/midiin.xpm"
#include "xpm/sysex.xpm"
#include "xpm/ctrl.xpm"
#include "xpm/meta.xpm"
#include "xpm/pitch.xpm"
#include "xpm/cafter.xpm"
#include "xpm/pafter.xpm"
#include "xpm/flag.xpm"
#include "xpm/flagS.xpm"
#include "xpm/lock.xpm"
#include "xpm/toc.xpm"
// #include "xpm/piano.xpm" // not used -Orcan
// #include "xpm/pianoS.xpm"
#include "xpm/exitS.xpm"

#include "xpm/undo.xpm"
#include "xpm/redo.xpm"
#include "xpm/undoS.xpm"
#include "xpm/redoS.xpm"
#include "xpm/editcutS.xpm"
#include "xpm/editcopyS.xpm"
#include "xpm/editpasteS.xpm"
#include "xpm/editmute.xpm"
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
#include "xpm/configure.xpm"
#include "xpm/panic.xpm"


// next two lines will vanish soon
#include "xpm/solobutton.xpm"
#include "xpm/newmutebutton.xpm"
#include "xpm/exit.xpm"
#include "xpm/exit1.xpm"

#include "xpm/toggle_small.xpm"
#include "xpm/redled.xpm"
#include "xpm/darkredled.xpm"
#include "xpm/greendot.xpm"
//#include "xpm/darkgreendot.xpm"
#include "xpm/bluedot.xpm"
#include "xpm/graydot.xpm"
#include "xpm/off.xpm"
#include "xpm/blacksquare.xpm"
#include "xpm/blacksqcheck.xpm"

#include "xpm/mastertrackS.xpm"
#include "xpm/localoffS.xpm"
#include "xpm/miditransformS.xpm"
#include "xpm/midi_plugS.xpm"
#include "xpm/miditransposeS.xpm"
#include "xpm/mixerS.xpm"
#include "xpm/mustangS.xpm"
#include "xpm/resetS.xpm"
#include "xpm/track_add.xpm"
#include "xpm/track_delete.xpm"
#include "xpm/listS.xpm"
#include "xpm/inputpluginS.xpm"
#include "xpm/cliplistS.xpm"
#include "xpm/mixeraudioS.xpm"
#include "xpm/initS.xpm"
#include "xpm/delta_on.xpm"
#include "xpm/delta_off.xpm"

#include "xpm/addtrack_addmiditrack.xpm"
#include "xpm/addtrack_audiogroup.xpm"
#include "xpm/addtrack_audioinput.xpm"
#include "xpm/addtrack_audiooutput.xpm"
#include "xpm/addtrack_auxsend.xpm"
#include "xpm/addtrack_drumtrack.xpm"
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
#include "xpm/midi_transpose.xpm"
#include "xpm/select.xpm"
#include "xpm/select_all.xpm"
#include "xpm/select_all_parts_on_track.xpm"
#include "xpm/select_deselect_all.xpm"
#include "xpm/select_inside_loop.xpm"
#include "xpm/select_invert_selection.xpm"
#include "xpm/select_outside_loop.xpm"

#include "xpm/mono.xpm"
#include "xpm/stereo.xpm"
#include "xpm/muse_icon.xpm"
#include "xpm/about_muse.xpm"
#include "xpm/muse_leftside_logo.xpm"

#include "xpm/global.xpm"
#include "xpm/project.xpm"
#include "xpm/user.xpm"

#include "xpm/sine.xpm"
#include "xpm/saw.xpm"
#include "icons.h"

#if QT_VERSION >= 0x040600
#define MPIXMAP(a,b) QPixmap(QIcon::fromTheme(b, QIcon(QPixmap(a))).pixmap(QPixmap(a).width(),QPixmap(a).height()))
#define MICON(a,b) QIcon(QIcon::fromTheme(b, QIcon(QPixmap(a))))
#else
#define MPIXMAP(a,b) QPixmap(a)
#define MICON(a,b) QIcon(QPixmap(a))
#endif

namespace MusEGui {

/* Quick API reference:
   -------------------

   QPixmap MPIXMAP(const char * const[] xpm, const QString & name)
   QIcon MICON(const char * const[] xpm, const QString & name)

      xpm:  a valid XPM image data
      name: filename of a theme icon, without the extension; or NULL
*/

QPixmap* track_commentIcon;
QPixmap* mastertrackSIcon;
QPixmap* localoffSIcon;
QPixmap* miditransformSIcon;
QPixmap* midi_plugSIcon;
QPixmap* miditransposeSIcon;
QPixmap* midiThruOnIcon;
QPixmap* midiThruOffIcon;
QPixmap* mixerSIcon;
QPixmap* mustangSIcon;
QPixmap* resetSIcon;
QPixmap* track_addIcon;
QPixmap* track_deleteIcon;
QPixmap* listSIcon;
QPixmap* inputpluginSIcon;
QPixmap* cliplistSIcon;
QPixmap* mixerAudioSIcon;
QPixmap* initSIcon;
QPixmap* deltaOnIcon;
QPixmap* deltaOffIcon;

QPixmap* exitIcon;
QPixmap* exit1Icon;
QPixmap* newmuteIcon;
QPixmap* soloIcon;

QPixmap* routesInIcon;
QPixmap* routesOutIcon;
QPixmap* routesMidiInIcon;
QPixmap* routesMidiOutIcon;

QPixmap* pointerIcon;
QPixmap* pencilIcon;
QPixmap* deleteIcon;
QPixmap* punchinIcon;
QPixmap* punchoutIcon;
QPixmap* punchin1Icon;
QPixmap* punchout1Icon;
QPixmap* loopIcon;
QPixmap* loop1Icon;
QPixmap* playIcon;

QPixmap* record1_Icon;
QPixmap* record_on_Icon;
QPixmap* record_off_Icon;
QPixmap* recordIcon;
QPixmap* stopIcon;
QPixmap* startIcon;
QPixmap* fforwardIcon;
QPixmap* frewindIcon;
QPixmap* dotIcon;
QPixmap* dothIcon;
QPixmap* dot1Icon;
QPixmap* note1Icon;
QPixmap* noteIcon;
QPixmap* stickIcon;
QPixmap* waveIcon;
QPixmap* synthIcon;
QPixmap* markIcon[3];
QPixmap* cutIcon;
QPixmap* steprecIcon;
QPixmap* glueIcon;
QPixmap* drawIcon;
QPixmap* cursorIcon;
QPixmap* quantIcon;
QPixmap* printIcon;
QPixmap* printIconS;
QPixmap* openIcon;
QPixmap* saveIcon;
QPixmap* saveasIcon;
QPixmap* openIconS;
QPixmap* saveIconS;
QPixmap* saveasIconS;
QPixmap* masterIcon;
QPixmap* filenewIcon;
QPixmap* filenewIconS;
QPixmap* homeIcon;
QPixmap* backIcon;
QPixmap* forwardIcon;
QPixmap* muteIcon;
QPixmap* upIcon;
QPixmap* downIcon;
QPixmap* boldIcon;
QPixmap* italicIcon;
QPixmap* underlinedIcon;
QPixmap* gvIcon;
QPixmap* midiinIcon;
QPixmap* sysexIcon;
QPixmap* ctrlIcon;
QPixmap* metaIcon;
QPixmap* pitchIcon;
QPixmap* cafterIcon;
QPixmap* pafterIcon;
QPixmap* flagIcon;
QPixmap* flagIconS;
QPixmap* lockIcon;
QPixmap* tocIcon;
QPixmap* exitIconS;

QPixmap* undoIcon;
QPixmap* redoIcon;
QPixmap* undoIconS;
QPixmap* redoIconS;

QPixmap* speakerIcon;
QPixmap* buttondownIcon;
QPixmap* configureIcon;

QPixmap* recEchoIconOn;
QPixmap* recEchoIconOff;
QPixmap* muteIconOn;
QPixmap* muteIconOff;
QPixmap* soloIconOn;
QPixmap* soloIconOff;
QPixmap* soloblksqIconOn;
QPixmap* soloblksqIconOff;
//QIcon* soloIconSet1;
//QIcon* soloIconSet2;

QPixmap* editmuteIcon;
QPixmap* editmuteSIcon;
QPixmap* panicIcon;

QIcon* pianoIconSet;
QIcon* scoreIconSet;
QIcon* editcutIconSet;
QIcon* editmuteIconSet;
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

QPixmap* toggle_small_Icon;
QPixmap* redLedIcon;
QPixmap* darkRedLedIcon;
QPixmap* greendotIcon;
//QPixmap* darkgreendotIcon;
QPixmap* graydotIcon;
QPixmap* bluedotIcon;
QPixmap* offIcon;
QPixmap* blacksquareIcon;
QPixmap* blacksqcheckIcon;

QPixmap* addtrack_addmiditrackIcon;
QPixmap* addtrack_audiogroupIcon;
QPixmap* addtrack_audioinputIcon;
QPixmap* addtrack_audiooutputIcon;
QPixmap* addtrack_auxsendIcon;
QPixmap* addtrack_drumtrackIcon;
QPixmap* addtrack_wavetrackIcon;
QPixmap* edit_drummsIcon;
QPixmap* edit_listIcon;
QPixmap* edit_waveIcon;
QPixmap* edit_mastertrackIcon;
QPixmap* edit_pianorollIcon;
QPixmap* edit_scoreIcon;
QPixmap* edit_track_addIcon;
QPixmap* edit_track_delIcon;
QPixmap* mastertrack_graphicIcon;
QPixmap* mastertrack_listIcon;
QPixmap* midi_transformIcon;
QPixmap* midi_transposeIcon;
QPixmap* selectIcon;
QPixmap* select_allIcon;
QPixmap* select_all_parts_on_trackIcon;
QPixmap* select_deselect_allIcon;
QPixmap* select_inside_loopIcon;
QPixmap* select_invert_selectionIcon;
QPixmap* select_outside_loopIcon;

QPixmap* audio_bounce_to_fileIcon;
QPixmap* audio_bounce_to_trackIcon;
QPixmap* audio_restartaudioIcon;
QPixmap* automation_clear_dataIcon;
QPixmap* automation_mixerIcon;
QPixmap* automation_take_snapshotIcon;
QPixmap* edit_midiIcon;
QPixmap* midi_edit_instrumentIcon;
QPixmap* midi_init_instrIcon;
QPixmap* midi_inputpluginsIcon;
QPixmap* midi_inputplugins_midi_input_filterIcon;
QPixmap* midi_inputplugins_midi_input_transformIcon;
QPixmap* midi_inputplugins_random_rhythm_generatorIcon;
QPixmap* midi_inputplugins_remote_controlIcon;
QPixmap* midi_inputplugins_transposeIcon;
QPixmap* midi_local_offIcon;
QPixmap* midi_reset_instrIcon;
QPixmap* settings_appearance_settingsIcon;
QPixmap* settings_configureshortcutsIcon;
QPixmap* settings_follow_songIcon;
QPixmap* settings_globalsettingsIcon;
QPixmap* settings_metronomeIcon;
QPixmap* settings_midifileexportIcon;
QPixmap* settings_midiport_softsynthsIcon;
QPixmap* settings_midisyncIcon;
QPixmap* view_bigtime_windowIcon;
QPixmap* view_cliplistIcon;
QPixmap* view_markerIcon;
QPixmap* view_mixerIcon;
QPixmap* view_transport_windowIcon;

QPixmap* monoIcon;
QPixmap* stereoIcon;
QPixmap* museIcon;
QPixmap* aboutMuseImage;
QPixmap* museLeftSideLogo;

QIcon* globalIcon;
QIcon* projectIcon;
QIcon* userIcon;


QPixmap* sineIcon;
QPixmap* sawIcon;

//---------------------------------------------------------
//   initIcons
//---------------------------------------------------------

void initIcons()
      {
      track_commentIcon = new MPIXMAP(track_comment_xpm, NULL);
      pointerIcon  = new MPIXMAP(pointer_xpm, NULL);
      pencilIcon   = new MPIXMAP(pencil_xpm, NULL);
      deleteIcon   = new MPIXMAP(delete_xpm, "draw-eraser");
      punchinIcon  = new MPIXMAP(punchin_xpm, NULL);
      punchoutIcon = new MPIXMAP(punchout_xpm, NULL);
      punchin1Icon = new MPIXMAP(punchin1_xpm, NULL);
      punchout1Icon = new MPIXMAP(punchout1_xpm, NULL);
      loopIcon     = new MPIXMAP(loop_xpm, NULL);
      loop1Icon    = new MPIXMAP(loop1_xpm, NULL);
      playIcon     = new MPIXMAP(play_xpm, "media-playback-start");

      record1_Icon  = new MPIXMAP(record1_xpm, NULL);
      record_on_Icon = new MPIXMAP(record_on_xpm, NULL);
      record_off_Icon = new MPIXMAP(record_off_xpm, NULL);
      recordIcon   = new MPIXMAP(record_xpm, "media-record");
      stopIcon     = new MPIXMAP(stop_xpm, "media-playback-stop");
      startIcon    = new MPIXMAP(start_xpm, "media-skip-backward");
      fforwardIcon = new MPIXMAP(fforward_xpm, "media-seek-forward");
      frewindIcon  = new MPIXMAP(frewind_xpm, "media-seek-backward");
      dotIcon      = new MPIXMAP(dot_xpm, "dialog-ok-apply");
      dothIcon     = new MPIXMAP(doth_xpm, "draw-circle");
      dot1Icon     = new MPIXMAP(dot1_xpm, NULL);
      noteIcon     = new MPIXMAP(note_xpm, NULL);
      note1Icon    = new MPIXMAP(note1_xpm, NULL);
      stickIcon    = new MPIXMAP(stick_xpm, NULL);
      waveIcon     = new MPIXMAP(wave_xpm, NULL);
      synthIcon    = new MPIXMAP(synth_xpm, NULL);
      markIcon[0]  = new MPIXMAP(cmark_xpm, NULL);
      markIcon[1]  = new MPIXMAP(lmark_xpm, NULL);
      markIcon[2]  = new MPIXMAP(rmark_xpm, NULL);
      cutIcon      = new MPIXMAP(cut_xpm, "edit-cut");
      steprecIcon  = new MPIXMAP(steprec_xpm, NULL);
      glueIcon     = new MPIXMAP(glue_xpm, NULL);
      drawIcon     = new MPIXMAP(draw_xpm, NULL);
      cursorIcon   = new MPIXMAP(cursor_xpm, NULL);
      quantIcon    = new MPIXMAP(quant_xpm, NULL);
      saveIcon     = new MPIXMAP(filesave_xpm, "document-save");
      saveasIcon     = new MPIXMAP(filesaveas_xpm, "document-save-as");
      printIcon    = new MPIXMAP(fileprint_xpm, "document-print");
      openIcon     = new MPIXMAP(fileopen_xpm, "document-open");
      saveIconS     = new MPIXMAP(filesaveS_xpm, "document-save");
      saveasIconS     = new MPIXMAP(filesaveasS_xpm, "document-save-as");
      printIconS    = new MPIXMAP(fileprintS_xpm, "document-print");
      openIconS     = new MPIXMAP(fileopenS_xpm, "document-open");
      masterIcon   = new MPIXMAP(master_xpm, "mixer-master");
      filenewIcon  = new MPIXMAP(filenew_xpm, "document-new");
      filenewIconS  = new MPIXMAP(filenewS_xpm, "document-new");
      homeIcon     = new MPIXMAP(home_xpm, "user-home");
      backIcon     = new MPIXMAP(back_xpm, "go-previous");
      forwardIcon  = new MPIXMAP(forward_xpm, "go-next");
      muteIcon     = new MPIXMAP(editmuteS_xpm, "audio-volume-muted");
      upIcon       = new MPIXMAP(up_xpm, "go-up");
      downIcon     = new MPIXMAP(down_xpm, "go-down");
      boldIcon     = new MPIXMAP(bold_xpm, "format-text-bold");
      italicIcon     = new MPIXMAP(italic_xpm, "format-text-italic");
      underlinedIcon = new MPIXMAP(underlined_xpm, "format-text-underline");
      gvIcon     = new MPIXMAP(gv_xpm, NULL);
      midiinIcon = new MPIXMAP(midiin_xpm, NULL);
      sysexIcon   = new MPIXMAP(sysex_xpm, NULL);
      ctrlIcon    = new MPIXMAP(ctrl_xpm, NULL);
      metaIcon    = new MPIXMAP(meta_xpm, NULL);
      pitchIcon   = new MPIXMAP(pitch_xpm, NULL);
      cafterIcon  = new MPIXMAP(cafter_xpm, NULL);
      pafterIcon  = new MPIXMAP(pafter_xpm, NULL);
      flagIcon    = new MPIXMAP(flag_xpm, NULL);
      flagIconS   = new MPIXMAP(flagS_xpm, NULL);
      lockIcon    = new MPIXMAP(lock_xpm, NULL);
      tocIcon     = new MPIXMAP(toc_xpm, NULL);
      exitIconS   = new MPIXMAP(exitS_xpm, "application-exit");

      undoIcon     = new MPIXMAP(undo_xpm, "edit-undo");
      redoIcon     = new MPIXMAP(redo_xpm, "edit-redo");
      undoIconS    = new MPIXMAP(undoS_xpm, "edit-undo");
      redoIconS    = new MPIXMAP(redoS_xpm, "edit-redo");

      speakerIcon    = new MPIXMAP(speaker_xpm, NULL);
      buttondownIcon = new MPIXMAP(buttondown_xpm, "arrow-down");
      configureIcon  = new MPIXMAP(configure_xpm, NULL);

      editmuteIcon  = new MPIXMAP(editmute_xpm, NULL);
      editmuteSIcon = new MPIXMAP(editmuteS_xpm, NULL);
      panicIcon  = new MPIXMAP(panic_xpm, NULL);

      editcutIconSet       = new MICON(editcutS_xpm, "edit-cut"); // ddskrjo
      editcopyIconSet      = new MICON(editcopyS_xpm, "edit-copy");
      editpasteIconSet     = new MICON(editpasteS_xpm, "edit-paste");
      editmuteIconSet      = new MICON(editmuteS_xpm, "audio-volume-muted");
      editpaste2TrackIconSet = new MICON(editpaste2trackS_xpm, NULL);
      editpasteCloneIconSet  = new MICON(editpastecloneS_xpm, NULL);
      editpasteClone2TrackIconSet = new MICON(editpasteclone2trackS_xpm, NULL); // ..
      /* Not used - Orcan
      pianoIcon                 = new MICON(piano_xpm, NULL);
      editcutIcon               = new MICON(editcut_xpm, "edit-cut");
      editcopyIcon              = new MICON(editcopy_xpm, "edit-copy");
      editpasteIcon             = new MICON(editpaste_xpm, "edit-paste");
      editpasteCloneIcon        = new MICON(editpasteclone_xpm, NULL);
      editpaste2TrackIcon       = new MICON(editpaste2track_xpm, NULL);
      editpasteClone2TrackIcon  = new MICON(editpasteclone2track_xpm, NULL);
      */

      //exitIcon             = new MPIXMAP(exit_xpm, "application-exit");
      //exit1Icon            = new MPIXMAP(exit1_xpm, "application-exit");
      // Changed by Tim. There are IMO no suitable theme substitutes for these two so far...
      exitIcon             = new MPIXMAP(exit_xpm, NULL);
      exit1Icon            = new MPIXMAP(exit1_xpm, NULL);

      // 2 lines odd code
      newmuteIcon          = new MPIXMAP(newmutebutton_xpm, NULL);
      soloIcon             = new MPIXMAP(solobutton_xpm, NULL);

      routesInIcon         = new MPIXMAP(routing_input_button_slim_4_xpm, NULL);
      routesOutIcon        = new MPIXMAP(routing_output_button_slim_4_xpm, NULL);
      routesMidiInIcon     = new MPIXMAP(routing_midi_input_button_slim_xpm, NULL);
      routesMidiOutIcon    = new MPIXMAP(routing_midi_output_button_slim_xpm, NULL);

      recEchoIconOn        = new MPIXMAP(rec_echo_on_xpm, NULL);
      recEchoIconOff       = new MPIXMAP(rec_echo_off_xpm, NULL);
      muteIconOn           = new MPIXMAP(mutebutton_on_xpm, NULL);
      muteIconOff          = new MPIXMAP(mutebutton_off_xpm, NULL);
      soloIconOn           = new MPIXMAP(solobutton_on_xpm, NULL);
      soloIconOff          = new MPIXMAP(solobutton_off_xpm, NULL);
      soloblksqIconOn      = new MPIXMAP(solobutton_on_blksq_xpm, NULL);
      soloblksqIconOff     = new MPIXMAP(solobutton_off_blksq_xpm, NULL);
      //soloIconSet1         = new QIcon(); 
      //soloIconSet2         = new QIcon();
      //soloIconSet1->addPixmap(*soloIconOn, QIcon::Normal, QIcon::On);
      //soloIconSet1->addPixmap(*soloIconOff, QIcon::Normal, QIcon::Off);
      //soloIconSet2->addPixmap(*soloblksqIconOn, QIcon::Normal, QIcon::On);
      //soloIconSet2->addPixmap(*soloblksqIconOff, QIcon::Normal, QIcon::Off);
      
      toggle_small_Icon    = new MPIXMAP(toggle_small_xpm, NULL);
      redLedIcon           = new MPIXMAP(redled_xpm, NULL);
      darkRedLedIcon       = new MPIXMAP(darkredled_xpm, NULL);
      greendotIcon         = new MPIXMAP(greendot_xpm, NULL);
      //darkgreendotIcon     = new MPIXMAP(darkgreendot_xpm, NULL);
      bluedotIcon          = new MPIXMAP(bluedot_xpm, NULL);
      graydotIcon          = new MPIXMAP(graydot_xpm, NULL);
      offIcon              = new MPIXMAP(off_xpm, NULL);
      blacksquareIcon      = new MPIXMAP(blacksquare_xpm, NULL);
      blacksqcheckIcon     = new MPIXMAP(blacksqcheck_xpm, NULL);

      mastertrackSIcon     = new MPIXMAP(mastertrackS_xpm, NULL);
      localoffSIcon        = new MPIXMAP(localoffS_xpm, NULL);
      miditransformSIcon   = new MPIXMAP(miditransformS_xpm, NULL);
      midi_plugSIcon       = new MPIXMAP(midi_plugS_xpm, NULL);
      miditransposeSIcon   = new MPIXMAP(miditransposeS_xpm, NULL);
      midiThruOnIcon       = new MPIXMAP(midi_thru_on4_xpm, NULL);      
      midiThruOffIcon      = new MPIXMAP(midi_thru_off4_xpm, NULL);      
      
      mixerSIcon           = new MPIXMAP(mixerS_xpm, NULL);
      mustangSIcon         = new MPIXMAP(mustangS_xpm, NULL);
      resetSIcon           = new MPIXMAP(resetS_xpm, NULL);
      track_addIcon        = new MPIXMAP(track_add_xpm, NULL);
      track_deleteIcon     = new MPIXMAP(track_delete_xpm, NULL);
      listSIcon            = new MPIXMAP(listS_xpm, NULL);
      inputpluginSIcon     = new MPIXMAP(inputpluginS_xpm, NULL);
      cliplistSIcon        = new MPIXMAP(cliplistS_xpm, NULL);
      mixerAudioSIcon      = new MPIXMAP(mixerAudioS_xpm, NULL);
      initSIcon            = new MPIXMAP(initS_xpm, NULL);
      deltaOnIcon          = new MPIXMAP(delta_on_xpm, NULL);
      deltaOffIcon         = new MPIXMAP(delta_off_xpm, NULL);

      addtrack_addmiditrackIcon     = new MPIXMAP(addtrack_addmiditrack_xpm, NULL);
      addtrack_audiogroupIcon       = new MPIXMAP(addtrack_audiogroup_xpm, NULL);
      addtrack_audioinputIcon       = new MPIXMAP(addtrack_audioinput_xpm, NULL);
      addtrack_audiooutputIcon      = new MPIXMAP(addtrack_audiooutput_xpm, NULL);
      addtrack_auxsendIcon          = new MPIXMAP(addtrack_auxsend_xpm, NULL);
      addtrack_drumtrackIcon        = new MPIXMAP(addtrack_drumtrack_xpm, NULL);
      addtrack_wavetrackIcon        = new MPIXMAP(addtrack_wavetrack_xpm, NULL);
      edit_drummsIcon               = new MPIXMAP(edit_drumms_xpm, NULL);
      edit_listIcon                 = new MPIXMAP(edit_list_xpm, NULL);
      edit_waveIcon                 = new MPIXMAP(edit_wave_xpm, NULL);
      edit_mastertrackIcon          = new MPIXMAP(edit_mastertrack_xpm, NULL);
      edit_pianorollIcon            = new MPIXMAP(edit_pianoroll_xpm, NULL);
      edit_scoreIcon                = new MPIXMAP(edit_score_xpm, NULL);
      edit_track_addIcon            = new MPIXMAP(edit_track_add_xpm, NULL);
      edit_track_delIcon            = new MPIXMAP(edit_track_del_xpm, NULL);
      mastertrack_graphicIcon       = new MPIXMAP(mastertrack_graphic_xpm, NULL);
      mastertrack_listIcon          = new MPIXMAP(mastertrack_list_xpm, NULL);
      midi_transformIcon            = new MPIXMAP(midi_transform_xpm, NULL);
      midi_transposeIcon            = new MPIXMAP(midi_transpose_xpm, NULL);
      selectIcon                    = new MPIXMAP(select_xpm, NULL);
      select_allIcon                = new MPIXMAP(select_all_xpm, NULL);
      select_all_parts_on_trackIcon = new MPIXMAP(select_all_parts_on_track_xpm, NULL);
      select_deselect_allIcon       = new MPIXMAP(select_deselect_all, NULL);
      select_inside_loopIcon        = new MPIXMAP(select_inside_loop_xpm, NULL);
      select_invert_selectionIcon   = new MPIXMAP(select_invert_selection, NULL);
      select_outside_loopIcon       = new MPIXMAP(select_outside_loop_xpm, NULL);
      pianoIconSet                  = new MICON(edit_pianoroll_xpm, NULL);
      scoreIconSet                  = new MICON(edit_score_xpm, NULL);

      audio_bounce_to_fileIcon                      = new MPIXMAP(audio_bounce_to_file_xpm, NULL);
      audio_bounce_to_trackIcon                     = new MPIXMAP(audio_bounce_to_track_xpm, NULL);
      audio_restartaudioIcon                        = new MPIXMAP(audio_restartaudio_xpm, NULL);
      automation_clear_dataIcon                     = new MPIXMAP(automation_clear_data_xpm, NULL);
      automation_mixerIcon                          = new MPIXMAP(automation_mixer_xpm, NULL);
      automation_take_snapshotIcon                  = new MPIXMAP(automation_take_snapshot_xpm, NULL);
      edit_midiIcon                                 = new MPIXMAP(edit_midi_xpm, NULL);
      midi_edit_instrumentIcon                      = new MPIXMAP(midi_edit_instrument_xpm, NULL);
      midi_init_instrIcon                           = new MPIXMAP(midi_init_instr_xpm, NULL);
      midi_inputpluginsIcon                         = new MPIXMAP(midi_inputplugins_xpm, NULL);
      midi_inputplugins_midi_input_filterIcon       = new MPIXMAP(midi_inputplugins_midi_input_filter_xpm, NULL);
      midi_inputplugins_midi_input_transformIcon    = new MPIXMAP(midi_inputplugins_midi_input_transform_xpm, NULL);
      midi_inputplugins_random_rhythm_generatorIcon = new MPIXMAP(midi_inputplugins_random_rhythm_generator_xpm, NULL);
      midi_inputplugins_remote_controlIcon          = new MPIXMAP(midi_inputplugins_remote_control_xpm, NULL);
      midi_inputplugins_transposeIcon               = new MPIXMAP(midi_inputplugins_transpose_xpm, NULL);
      midi_local_offIcon                            = new MPIXMAP(midi_local_off_xpm, NULL);
      midi_reset_instrIcon                          = new MPIXMAP(midi_reset_instr_xpm, NULL);
      settings_appearance_settingsIcon              = new MPIXMAP(settings_appearance_settings_xpm, NULL);
      settings_configureshortcutsIcon               = new MPIXMAP(settings_configureshortcuts_xpm, NULL);
      settings_follow_songIcon                      = new MPIXMAP(settings_follow_song_xpm, NULL);
      settings_globalsettingsIcon                   = new MPIXMAP(settings_globalsettings_xpm, NULL);
      settings_metronomeIcon                        = new MPIXMAP(settings_metronome_xpm, NULL);
      settings_midifileexportIcon                   = new MPIXMAP(settings_midifileexport_xpm, NULL);
      settings_midiport_softsynthsIcon              = new MPIXMAP(settings_midiport_softsynths_xpm, NULL);
      settings_midisyncIcon                         = new MPIXMAP(settings_midisync_xpm, NULL);
      view_bigtime_windowIcon                       = new MPIXMAP(view_bigtime_window_xpm, NULL);
      view_cliplistIcon                             = new MPIXMAP(view_cliplist_xpm, NULL);
      view_markerIcon                               = new MPIXMAP(view_marker_xpm, NULL);
      view_mixerIcon                                = new MPIXMAP(view_mixer_xpm, NULL);
      view_transport_windowIcon                     = new MPIXMAP(view_transport_window_xpm, NULL);

      monoIcon                                      = new MPIXMAP(mono_xpm, NULL);
      stereoIcon                                    = new MPIXMAP(stereo_xpm, NULL);

      museIcon                                      = new MPIXMAP(muse_icon_xpm, NULL);
      aboutMuseImage                                = new MPIXMAP(about_muse_xpm, NULL);
      museLeftSideLogo                              = new MPIXMAP(muse_leftside_logo_xpm, NULL);
      globalIcon                                    = new MICON(global_xpm, "folder");
      userIcon                                      = new MICON(user_xpm, "user-home");
      projectIcon                                   = new MICON(project_xpm, "folder-sound");

      sineIcon                                      = new MPIXMAP(sine_xpm, NULL);
      sawIcon                                       = new MPIXMAP(saw_xpm, NULL);
      }

//---------------------------------------------------------
//   deleteIcons
//---------------------------------------------------------

void deleteIcons()
      {
      delete track_commentIcon;
      delete pointerIcon;
      delete pencilIcon;
      delete deleteIcon;
      delete punchinIcon;
      delete punchoutIcon;
      delete punchin1Icon;
      delete punchout1Icon;
      delete loopIcon;
      delete loop1Icon;
      delete playIcon;

      delete record1_Icon;
      delete record_on_Icon;
      delete record_off_Icon;
      delete recordIcon;   
      delete stopIcon;     
      delete startIcon;    
      delete fforwardIcon; 
      delete frewindIcon;  
      delete dotIcon;      
      delete dothIcon;     
      delete dot1Icon;     
      delete noteIcon;     
      delete note1Icon;    
      delete stickIcon;    
      delete waveIcon;     
      delete synthIcon;    
      delete markIcon[0];  
      delete markIcon[1];  
      delete markIcon[2];  
      delete cutIcon;      
      delete steprecIcon;  
      delete glueIcon;     
      delete drawIcon;     
      delete cursorIcon;   
      delete quantIcon;    
      delete saveIcon;     
      delete saveasIcon;   
      delete printIcon;    
      delete openIcon;     
      delete saveIconS;    
      delete saveasIconS;
      delete printIconS;   
      delete openIconS;    
      delete masterIcon;   
      delete filenewIcon;  
      delete filenewIconS; 
      delete homeIcon;     
      delete backIcon;     
      delete forwardIcon;  
      delete muteIcon;     
      delete upIcon;       
      delete downIcon;     
      delete boldIcon;     
      delete italicIcon;   
      delete underlinedIcon;
      delete gvIcon;
      delete midiinIcon;
      delete sysexIcon; 
      delete ctrlIcon;  
      delete metaIcon;  
      delete pitchIcon; 
      delete cafterIcon;
      delete pafterIcon;
      delete flagIcon;  
      delete flagIconS; 
      delete lockIcon;  
      delete tocIcon;   
      delete exitIconS; 

      delete undoIcon;  
      delete redoIcon;  
      delete undoIconS; 
      delete redoIconS; 

      delete speakerIcon;    
      delete buttondownIcon;
      delete configureIcon;  

      delete editmuteIcon;
      delete editmuteSIcon;
      delete panicIcon;

      delete editcutIconSet;     
      delete editcopyIconSet;    
      delete editpasteIconSet;   
      delete editmuteIconSet;    
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

      delete exitIcon;             
      delete exit1Icon;            

      // 2 lines odd code
      delete newmuteIcon;          
      delete soloIcon;             

      delete routesInIcon;         
      delete routesOutIcon;        
      delete routesMidiInIcon;     
      delete routesMidiOutIcon;    

      delete recEchoIconOn;        
      delete recEchoIconOff;       
      delete muteIconOn;           
      delete muteIconOff;          
      delete soloIconOn;           
      delete soloIconOff;          
      delete soloblksqIconOn;      
      delete soloblksqIconOff;     
      //delete soloIconSet1       
      //delete soloIconSet2       

      delete toggle_small_Icon;    
      delete redLedIcon;           
      delete darkRedLedIcon;       
      delete greendotIcon;         
      //delete darkgreendotIcon;   
      delete bluedotIcon;          
      delete graydotIcon;          
      delete offIcon;              
      delete blacksquareIcon;      
      delete blacksqcheckIcon;     

      delete mastertrackSIcon;     
      delete localoffSIcon;        
      delete miditransformSIcon;   
      delete midi_plugSIcon;       
      delete miditransposeSIcon;   
      delete midiThruOnIcon;         
      delete midiThruOffIcon;         

      delete mixerSIcon;           
      delete mustangSIcon;         
      delete resetSIcon;           
      delete track_addIcon;        
      delete track_deleteIcon;     
      delete listSIcon;            
      delete inputpluginSIcon;     
      delete cliplistSIcon;        
      delete mixerAudioSIcon;      
      delete initSIcon;            
      delete deltaOnIcon;          
      delete deltaOffIcon;         

      delete addtrack_addmiditrackIcon;     
      delete addtrack_audiogroupIcon;       
      delete addtrack_audioinputIcon;       
      delete addtrack_audiooutputIcon;      
      delete addtrack_auxsendIcon;          
      delete addtrack_drumtrackIcon;        
      delete addtrack_wavetrackIcon;        
      delete edit_drummsIcon;               
      delete edit_listIcon;                 
      delete edit_waveIcon;                 
      delete edit_mastertrackIcon;          
      delete edit_pianorollIcon;            
      delete edit_scoreIcon;                
      delete edit_track_addIcon;            
      delete edit_track_delIcon;            
      delete mastertrack_graphicIcon;       
      delete mastertrack_listIcon;          
      delete midi_transformIcon;            
      delete midi_transposeIcon;            
      delete selectIcon;                    
      delete select_allIcon;                
      delete select_all_parts_on_trackIcon; 
      delete select_deselect_allIcon;       
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
      delete edit_midiIcon;                 
      delete midi_edit_instrumentIcon;      
      delete midi_init_instrIcon;           
      delete midi_inputpluginsIcon;         
      delete midi_inputplugins_midi_input_filterIcon;       
      delete midi_inputplugins_midi_input_transformIcon;    
      delete midi_inputplugins_random_rhythm_generatorIcon; 
      delete midi_inputplugins_remote_controlIcon;          
      delete midi_inputplugins_transposeIcon;               
      delete midi_local_offIcon;                            
      delete midi_reset_instrIcon;                          
      delete settings_appearance_settingsIcon;              
      delete settings_configureshortcutsIcon;               
      delete settings_follow_songIcon;                      
      delete settings_globalsettingsIcon;                   
      delete settings_metronomeIcon;                        
      delete settings_midifileexportIcon;                   
      delete settings_midiport_softsynthsIcon;              
      delete settings_midisyncIcon;                         
      delete view_bigtime_windowIcon;                       
      delete view_cliplistIcon;                             
      delete view_markerIcon;                               
      delete view_mixerIcon;                                
      delete view_transport_windowIcon;                     

      delete monoIcon;                                      
      delete stereoIcon;                                    

      delete museIcon;                                      
      delete aboutMuseImage;                                
      delete museLeftSideLogo;                              
      delete globalIcon;                                    
      delete userIcon;                                      
      delete projectIcon;                                   

      delete sineIcon;                                      
      delete sawIcon;                                       
      }

} // namespace MusEGui
