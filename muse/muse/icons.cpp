//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "globals.h"

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
#include "xpm/midi_local_off.xpm"
#include "xpm/midi_reset_instr.xpm"
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

#include "xpm/record_on.xpm"
#include "xpm/record_off.xpm"
#include "xpm/punchin1.xpm"
#include "xpm/punchout1.xpm"
#include "xpm/loop1.xpm"
#include "xpm/dot.xpm"
#include "xpm/doth.xpm"
#include "xpm/dot1.xpm"
#include "xpm/note.xpm"
#include "xpm/note1.xpm"
#include "xpm/stick.xpm"
#include "xpm/wave.xpm"
#include "xpm/cmark.xpm"
#include "xpm/lmark.xpm"
#include "xpm/rmark.xpm"
#include "xpm/cut.xpm"
#include "xpm/steprec.xpm"
#include "xpm/glue.xpm"
#include "xpm/draw.xpm"
#include "xpm/quant.xpm"
// #include "xpm/fileprint.xpm"
// #include "xpm/filesave.xpm"
// #include "xpm/fileopen.xpm"
// #include "xpm/fileprintS.xpm"
#include "xpm/filesaveS.xpm"
#include "xpm/fileopenS.xpm"
#include "xpm/master.xpm"
// #include "xpm/filenew.xpm"
#include "xpm/filenewS.xpm"
#include "xpm/home.xpm"
#include "xpm/back.xpm"
#include "xpm/forward.xpm"

// #include "xpm/mute.xpm"
#include "xpm/solobutton_on.xpm"
#include "xpm/solobutton_off.xpm"
#include "xpm/mutebutton_on.xpm"
#include "xpm/mutebutton_off.xpm"

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
// #include "xpm/piano.xpm"
#include "xpm/pianoS.xpm"
#include "xpm/exitS.xpm"

#include "xpm/undo.xpm"
#include "xpm/redo.xpm"
#include "xpm/editcut.xpm"
#include "xpm/editcopy.xpm"
#include "xpm/editpaste.xpm"
#include "xpm/undoS.xpm"
#include "xpm/redoS.xpm"
// #include "xpm/editcutS.xpm"
// #include "xpm/editcopyS.xpm"
// #include "xpm/editpasteS.xpm"
#include "xpm/editmute.xpm"
#include "xpm/editmuteS.xpm"

#include "xpm/speaker.xpm"
#include "xpm/buttondown.xpm"
#include "xpm/configure.xpm"
#include "xpm/panic.xpm"


// next two lines will vanish soon
#include "xpm/solobutton.xpm"
#include "xpm/newmutebutton.xpm"
#include "xpm/exit.xpm"
#include "xpm/exit1.xpm"

#include "xpm/greendot.xpm"
#include "xpm/darkgreendot.xpm"
#include "xpm/bluedot.xpm"
#include "xpm/graydot.xpm"
#include "xpm/off.xpm"

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

#include "xpm/addtrack_addmiditrack.xpm"
#include "xpm/addtrack_audiogroup.xpm"
#include "xpm/addtrack_audioinput.xpm"
#include "xpm/addtrack_audiooutput.xpm"
#include "xpm/addtrack_auxsend.xpm"
#include "xpm/addtrack_drumtrack.xpm"
#include "xpm/addtrack_wavetrack.xpm"
#include "xpm/edit_drumms.xpm"
#include "xpm/edit_list.xpm"
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
#include "xpm/config.xpm"
#include "xpm/minus.xpm"
#include "xpm/plus.xpm"

QPixmap* mastertrackSIcon;
QPixmap* localoffSIcon;
QPixmap* miditransformSIcon;
QPixmap* midi_plugSIcon;
QPixmap* miditransposeSIcon;
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

QPixmap* exitIcon;
QPixmap* exit1Icon;
QPixmap* newmuteIcon;
QPixmap* soloIcon;

QPixmap* pointerIcon;
QPixmap* pencilIcon;
QPixmap* deleteIcon;
QPixmap* punchin1Icon;
QPixmap* punchout1Icon;
QPixmap* loop1Icon;
QPixmap* playIcon;

QPixmap* record_on_Icon;
QPixmap* record_off_Icon;
QIcon* recordIcon;

QPixmap* stopIcon;
QPixmap* fforwardIcon;
QPixmap* frewindIcon;
QPixmap* dotIcon;
QPixmap* dothIcon;
QPixmap* dot1Icon;
QPixmap* note1Icon;
QPixmap* noteIcon;
QPixmap* stickIcon;
QPixmap* waveIcon;
QPixmap* markIcon[3];
QPixmap* cutIcon;
QPixmap* steprecIcon;
QPixmap* glueIcon;
QPixmap* drawIcon;
QPixmap* quantIcon;
// QPixmap* printIcon;
// QPixmap* printIconS;
QPixmap* openIcon;
QPixmap* saveIcon;
QPixmap* openIconS;
QPixmap* saveIconS;
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

QPixmap* muteIconOn;
QPixmap* muteIconOff;
QPixmap* soloIconOn;
QPixmap* soloIconOff;

QPixmap* editmuteIcon;
QPixmap* editmuteSIcon;
QPixmap* panicIcon;

QIcon* pianoIconSet;
QIcon* scoreIconSet;
QIcon* editcutIconSet;
QIcon* editmuteIconSet;
QIcon* editcopyIconSet;
QIcon* editpasteIconSet;

QPixmap* greendotIcon;
QPixmap* darkgreendotIcon;
QPixmap* graydotIcon;
QPixmap* bluedotIcon;
QPixmap* offIcon;

QPixmap* addtrack_addmiditrackIcon;
QPixmap* addtrack_audiogroupIcon;
QPixmap* addtrack_audioinputIcon;
QPixmap* addtrack_audiooutputIcon;
QPixmap* addtrack_auxsendIcon;
QPixmap* addtrack_drumtrackIcon;
QPixmap* addtrack_wavetrackIcon;
QPixmap* edit_drummsIcon;
QPixmap* edit_listIcon;
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
QPixmap* museIcon64;
QPixmap* configIcon;
QPixmap* minusIcon;
QPixmap* plusIcon;

//---------------------------------------------------------
//   initIcons
//---------------------------------------------------------

void initIcons()
      {
      pointerIcon  = new QPixmap(pointer_xpm);
      pencilIcon   = new QPixmap(pencil_xpm);
      deleteIcon   = new QPixmap(delete_xpm);
      punchin1Icon = new QPixmap(punchin1_xpm);
      punchout1Icon = new QPixmap(punchout1_xpm);
      loop1Icon    = new QPixmap(loop1_xpm);
      playIcon     = new QPixmap(play_xpm);

      record_on_Icon= new QPixmap(record_on_xpm);
      record_off_Icon= new QPixmap(record_off_xpm);

      recordIcon   = new QIcon(*record_off_Icon);
      recordIcon->addPixmap(*record_on_Icon, QIcon::Normal, QIcon::On);

      dotIcon      = new QPixmap(dot_xpm);
      dothIcon     = new QPixmap(doth_xpm);
      dot1Icon     = new QPixmap(dot1_xpm);
      noteIcon     = new QPixmap(note_xpm);
      note1Icon    = new QPixmap(note1_xpm);
      stickIcon    = new QPixmap(stick_xpm);
      waveIcon     = new QPixmap(wave_xpm);
      markIcon[0]  = new QPixmap(cmark_xpm);
      markIcon[1]  = new QPixmap(lmark_xpm);
      markIcon[2]  = new QPixmap(rmark_xpm);
      cutIcon      = new QPixmap(cut_xpm);
      steprecIcon  = new QPixmap(steprec_xpm);
      glueIcon     = new QPixmap(glue_xpm);
      drawIcon     = new QPixmap(draw_xpm);
      quantIcon    = new QPixmap(quant_xpm);
      saveIcon     = new QPixmap(":/xpm/filesave.png");
      openIcon     = new QPixmap(":/xpm/fileopen.png");
      saveIconS     = new QPixmap(filesaveS_xpm);
      openIconS     = new QPixmap(fileopenS_xpm);
      masterIcon   = new QPixmap(master_xpm);
      filenewIcon  = new QPixmap(":/xpm/filenew.png");
      filenewIconS  = new QPixmap(filenewS_xpm);
      homeIcon     = new QPixmap(home_xpm);
      backIcon     = new QPixmap(back_xpm);
      forwardIcon  = new QPixmap(forward_xpm);
      muteIcon     = new QPixmap(editmuteS_xpm);
      upIcon       = new QPixmap(up_xpm);
      downIcon     = new QPixmap(down_xpm);
      boldIcon     = new QPixmap(bold_xpm);
      italicIcon     = new QPixmap(italic_xpm);
      underlinedIcon = new QPixmap(underlined_xpm);
      gvIcon     = new QPixmap(gv_xpm);
      midiinIcon = new QPixmap(midiin_xpm);
      sysexIcon   = new QPixmap(sysex_xpm);
      ctrlIcon    = new QPixmap(ctrl_xpm);
      metaIcon    = new QPixmap(meta_xpm);
      pitchIcon   = new QPixmap(pitch_xpm);
      cafterIcon  = new QPixmap(cafter_xpm);
      pafterIcon  = new QPixmap(pafter_xpm);
      flagIcon    = new QPixmap(flag_xpm);
      flagIconS   = new QPixmap(flagS_xpm);
      lockIcon    = new QPixmap(lock_xpm);
      tocIcon     = new QPixmap(toc_xpm);
      exitIconS   = new QPixmap(exitS_xpm);

      undoIcon     = new QPixmap(undo_xpm);
      redoIcon     = new QPixmap(redo_xpm);
      undoIconS    = new QPixmap(undoS_xpm);
      redoIconS    = new QPixmap(redoS_xpm);

      speakerIcon    = new QPixmap(speaker_xpm);
      buttondownIcon = new QPixmap(buttondown_xpm);
      configureIcon  = new QPixmap(configure_xpm);

      editmuteIcon  = new QPixmap(editmute_xpm);
      editmuteSIcon = new QPixmap(editmuteS_xpm);
      panicIcon  = new QPixmap(panic_xpm);

      editcutIconSet       = new QIcon(QPixmap(editcut_xpm));
      editcopyIconSet      = new QIcon(QPixmap(editcopy_xpm));
      editpasteIconSet     = new QIcon(QPixmap(editpaste_xpm));
      editmuteIconSet      = new QIcon(QPixmap(editmute_xpm));

      exitIcon             = new QPixmap(exit_xpm);
      exit1Icon            = new QPixmap(exit1_xpm);

      // 2 lines odd code
      newmuteIcon          = new QPixmap(newmutebutton_xpm);
      soloIcon             = new QPixmap(solobutton_xpm);

      muteIconOn           = new QPixmap(mutebutton_on_xpm);
      muteIconOff          = new QPixmap(mutebutton_off_xpm);
      soloIconOn           = new QPixmap(solobutton_on_xpm);
      soloIconOff          = new QPixmap(solobutton_off_xpm);
      greendotIcon         = new QPixmap(greendot_xpm);
      darkgreendotIcon     = new QPixmap(darkgreendot_xpm);
      bluedotIcon          = new QPixmap(bluedot_xpm);
      graydotIcon          = new QPixmap(graydot_xpm);
      offIcon              = new QPixmap(off_xpm);

      mastertrackSIcon     = new QPixmap(mastertrackS_xpm);
      localoffSIcon        = new QPixmap(localoffS_xpm);
      miditransformSIcon   = new QPixmap(miditransformS_xpm);
      midi_plugSIcon       = new QPixmap(midi_plugS_xpm);
      miditransposeSIcon   = new QPixmap(miditransposeS_xpm);
      mixerSIcon           = new QPixmap(mixerS_xpm);
      mustangSIcon         = new QPixmap(mustangS_xpm);
      resetSIcon           = new QPixmap(resetS_xpm);
      track_addIcon        = new QPixmap(track_add_xpm);
      track_deleteIcon     = new QPixmap(track_delete_xpm);
      listSIcon            = new QPixmap(listS_xpm);
      inputpluginSIcon     = new QPixmap(inputpluginS_xpm);
      cliplistSIcon        = new QPixmap(cliplistS_xpm);
      mixerAudioSIcon      = new QPixmap(mixerAudioS_xpm);
      initSIcon            = new QPixmap(initS_xpm);

      addtrack_addmiditrackIcon     = new QPixmap(addtrack_addmiditrack_xpm);
      addtrack_audiogroupIcon       = new QPixmap(addtrack_audiogroup_xpm);
      addtrack_audioinputIcon       = new QPixmap(addtrack_audioinput_xpm);
      addtrack_audiooutputIcon      = new QPixmap(addtrack_audiooutput_xpm);
      addtrack_auxsendIcon          = new QPixmap(addtrack_auxsend_xpm);
      addtrack_drumtrackIcon        = new QPixmap(addtrack_drumtrack_xpm);
      addtrack_wavetrackIcon        = new QPixmap(addtrack_wavetrack_xpm);
      edit_drummsIcon               = new QPixmap(edit_drumms_xpm);
      edit_listIcon                 = new QPixmap(edit_list_xpm);
      edit_mastertrackIcon          = new QPixmap(edit_mastertrack_xpm);
      edit_pianorollIcon            = new QPixmap(edit_pianoroll_xpm);
      edit_scoreIcon                = new QPixmap(edit_score_xpm);
      edit_track_addIcon            = new QPixmap(edit_track_add_xpm);
      edit_track_delIcon            = new QPixmap(edit_track_del_xpm);
      mastertrack_graphicIcon       = new QPixmap(mastertrack_graphic_xpm);
      mastertrack_listIcon          = new QPixmap(mastertrack_list_xpm);
      midi_transformIcon            = new QPixmap(midi_transform_xpm);
      midi_transposeIcon            = new QPixmap(midi_transpose_xpm);
      selectIcon                    = new QPixmap(select_xpm);
      select_allIcon                = new QPixmap(select_all_xpm);
      select_all_parts_on_trackIcon = new QPixmap(select_all_parts_on_track_xpm);
      select_deselect_allIcon       = new QPixmap(select_deselect_all);
      select_inside_loopIcon        = new QPixmap(select_inside_loop_xpm);
      select_invert_selectionIcon   = new QPixmap(select_invert_selection);
      select_outside_loopIcon       = new QPixmap(select_outside_loop_xpm);
      pianoIconSet                  = new QIcon(QPixmap(pianoS_xpm));

      audio_bounce_to_fileIcon                      = new QPixmap(audio_bounce_to_file_xpm);
      audio_bounce_to_trackIcon                     = new QPixmap(audio_bounce_to_track_xpm);
      audio_restartaudioIcon                        = new QPixmap(audio_restartaudio_xpm);
      automation_clear_dataIcon                     = new QPixmap(automation_clear_data_xpm);
      automation_mixerIcon                          = new QPixmap(automation_mixer_xpm);
      automation_take_snapshotIcon                  = new QPixmap(automation_take_snapshot_xpm);
      edit_midiIcon                                 = new QPixmap(edit_midi_xpm);
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
      settings_follow_songIcon                      = new QPixmap(settings_follow_song_xpm);
      settings_globalsettingsIcon                   = new QPixmap(settings_globalsettings_xpm);
      settings_metronomeIcon                        = new QPixmap(settings_metronome_xpm);
      settings_midifileexportIcon                   = new QPixmap(settings_midifileexport_xpm);
      settings_midiport_softsynthsIcon              = new QPixmap(settings_midiport_softsynths_xpm);
      settings_midisyncIcon                         = new QPixmap(settings_midisync_xpm);
      view_bigtime_windowIcon                       = new QPixmap(view_bigtime_window_xpm);
      view_cliplistIcon                             = new QPixmap(view_cliplist_xpm);
      view_markerIcon                               = new QPixmap(view_marker_xpm);
      view_mixerIcon                                = new QPixmap(view_mixer_xpm);
      view_transport_windowIcon                     = new QPixmap(view_transport_window_xpm);

      monoIcon   = new QPixmap(mono_xpm);
      stereoIcon = new QPixmap(stereo_xpm);

      museIcon = new QPixmap(muse_icon_xpm);
      configIcon = new QPixmap(config_xpm);
      minusIcon = new QPixmap(minus_xpm);
      plusIcon = new QPixmap(plus_xpm);
      }

