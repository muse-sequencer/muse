//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: icons.h,v 1.11.2.8 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef ICONS_H
#define ICONS_H

class QPixmap;
class QIcon;
class QCursor;

// NOTICE: Although it is tempting to use multi-icons (addPixmap, addFile etc.),
//          certain styles do not support it, such as QtCurve.
//         Therefore the separate icons must be manually set upon each state.
// No longer relevant, arbitrary styles not supported any more (MusE 4.0)

namespace MusEGui {

extern QPixmap* dotIcon;
extern QPixmap* dothIcon;

extern QPixmap* midiCtrlMergeEraseIcon;
extern QPixmap* midiCtrlMergeEraseInclusiveIcon;
extern QPixmap* midiCtrlMergeEraseWysiwygIcon;

extern QPixmap* record1_Icon;

extern QPixmap* routesInIcon;
extern QPixmap* routesOutIcon;
extern QPixmap* routesMidiInIcon;
extern QPixmap* routesMidiOutIcon;

extern QIcon* ledGreenIcon;
extern QIcon* ledDarkGreenIcon;

extern QPixmap* greendotIcon;
extern QPixmap* greendot12x12Icon;
extern QPixmap* reddotIcon;
extern QPixmap* darkgreendotIcon;
extern QPixmap* graydot12x12Icon;
extern QPixmap* bluedotIcon;
extern QPixmap* bluedot12x12Icon;
extern QPixmap* orangedotIcon;
extern QPixmap* orangedot12x12Icon;

extern QPixmap* deltaOnIcon;
extern QPixmap* deltaOffIcon;

extern QPixmap* select_allIcon;
extern QPixmap* select_all_parts_on_trackIcon;
extern QPixmap* select_deselect_allIcon;
extern QIcon*   icon_select_deselect_all;
extern QPixmap* select_inside_loopIcon;
extern QPixmap* select_invert_selectionIcon;
extern QPixmap* select_outside_loopIcon;

extern QPixmap* midi_edit_instrumentIcon;
extern QPixmap* settings_appearance_settingsIcon;

extern QPixmap* museIcon;
extern QPixmap* aboutMuseImage;

extern QIcon* globalIcon;
extern QIcon* projectIcon;
extern QIcon* userIcon;

extern QIcon* pianoNewIcon;
extern QIcon* presetsNewIcon;


//----------------------------------
//   SVG...
//----------------------------------

extern QIcon* dropDownTriangleSVGIcon;
extern QIcon* expandLeftRightSVGIcon;

extern QIcon* routingInputSVGIcon;
extern QIcon* routingOutputSVGIcon;
extern QIcon* routingInputUnconnectedSVGIcon;
extern QIcon* routingOutputUnconnectedSVGIcon;

extern QIcon* headphonesOffSVGIcon;
extern QIcon* headphonesOnSVGIcon;

extern QIcon* muteOffSVGIcon;
extern QIcon* muteOnSVGIcon;
extern QIcon* muteOnXSVGIcon;
extern QIcon* muteProxyOnSVGIcon;
extern QIcon* muteAndProxyOnSVGIcon;
extern QIcon* muteStateSVGIcon;

extern QIcon* soloOffSVGIcon;
extern QIcon* soloOnSVGIcon;
extern QIcon* soloOnAloneSVGIcon;
extern QIcon* soloProxyOnSVGIcon;
extern QIcon* soloProxyOnAloneSVGIcon;
extern QIcon* soloAndProxyOnSVGIcon;
extern QIcon* soloStateSVGIcon;

extern QIcon* trackOffSVGIcon;
extern QIcon* trackOnSVGIcon;

extern QIcon* stereoOffSVGIcon;
extern QIcon* stereoOnSVGIcon;

extern QIcon* preFaderOffSVGIcon;
extern QIcon* preFaderOnSVGIcon;

extern QIcon* recArmOffSVGIcon;
extern QIcon* recArmOnSVGIcon;
extern QIcon* recArmStateSVGIcon;

extern QIcon* monitorOffSVGIcon;
extern QIcon* monitorOnSVGIcon;
extern QIcon* monitorStateSVGIcon;

extern QIcon* velocityPerNoteSVGIcon;
extern QIcon* midiControllerNewSVGIcon;
extern QIcon* midiControllerSelectSVGIcon;
extern QIcon* midiControllerRemoveSVGIcon;

//extern QIcon* soloSVGIcon;
//extern QIcon* soloProxySVGIcon;
extern QIcon* muteSVGIcon;
//extern QIcon* trackEnableSVGIcon;
//extern QIcon* recArmSVGIcon;
extern QIcon* recMasterSVGIcon;

extern QIcon* stopSVGIcon;
extern QIcon* playSVGIcon;
extern QIcon* fastForwardSVGIcon;
extern QIcon* rewindSVGIcon;
extern QIcon* rewindToStartSVGIcon;

extern QIcon* externSyncOffSVGIcon;
extern QIcon* externSyncOnSVGIcon;

extern QIcon* masterTrackOffSVGIcon;
extern QIcon* masterTrackOnSVGIcon;

extern QIcon* jackTransportOffSVGIcon;
extern QIcon* jackTransportOnSVGIcon;

extern QIcon* timebaseMasterOffSVGIcon;
extern QIcon* timebaseMasterOnSVGIcon;

extern QIcon* metronomeOffSVGIcon;
extern QIcon* metronomeOnSVGIcon;

extern QIcon* fixedSpeedSVGIcon;
extern QIcon* transportAffectsLatencySVGIcon;
extern QIcon* overrideLatencySVGIcon;

extern QIcon* panicSVGIcon;
extern QIcon* loopSVGIcon;
extern QIcon* punchinSVGIcon;
extern QIcon* punchoutSVGIcon;
extern QIcon* undoSVGIcon;
extern QIcon* redoSVGIcon;
extern QIcon* midiinSVGIcon;
extern QIcon* steprecSVGIcon;
extern QIcon* speakerSVGIcon;
extern QIcon* speakerSingleNoteSVGIcon;
extern QIcon* speakerChordsSVGIcon;
extern QIcon* filenewSVGIcon;
extern QIcon* filetemplateSVGIcon;
extern QIcon* fileopenSVGIcon;
extern QIcon* filesaveSVGIcon;
extern QIcon* filesaveasSVGIcon;
extern QIcon* filesaveProjectSVGIcon;
extern QIcon* filesaveTemplateSVGIcon;
extern QIcon* filesaveRevisionSVGIcon;
extern QIcon* filecloseSVGIcon;
extern QIcon* appexitSVGIcon;
extern QIcon* whatsthisSVGIcon;
extern QIcon* infoSVGIcon;
extern QIcon* showFieldsSVGIcon;
extern QIcon* exitSVGIcon;
extern QIcon* noteSVGIcon;
extern QIcon* metaSVGIcon;
extern QIcon* ctrlSVGIcon;
extern QIcon* sysexSVGIcon;
extern QIcon* tracktypeSVGIcon;

extern QIcon* pencilIconSVG;
extern QIcon* glueIconSVG;
extern QIcon* cutterIconSVG;
extern QIcon* zoomIconSVG;
extern QIcon* zoomAtIconSVG;
extern QIcon* deleteIconSVG;
extern QIcon* drawIconSVG;
extern QIcon* pointerIconSVG;
extern QIcon* mutePartsIconSVG;
extern QIcon* handIconSVG;
extern QIcon* closedHandIconSVG;
extern QIcon* cursorIconSVG;
//extern QIcon* magnetIconSVG;
//extern QIcon* customMoveIconSVG;
extern QIcon* pencilMove4WayIconSVG;
extern QIcon* pencilMoveHorizIconSVG;
extern QIcon* pencilMoveVertIconSVG;
extern QIcon* audioStretchIconSVG;
extern QIcon* audioResampleIconSVG;

extern QIcon* pianorollSVGIcon;
extern QIcon* arrangerSVGIcon;
extern QIcon* waveeditorSVGIcon;
extern QIcon* scoreeditSVGIcon;
extern QIcon* mastereditSVGIcon;
extern QIcon* drumeditSVGIcon;
extern QIcon* listeditSVGIcon;

extern QIcon* synthSVGIcon;
extern QIcon* trackInputSVGIcon;
extern QIcon* trackOutputSVGIcon;
extern QIcon* trackAuxSVGIcon;
extern QIcon* trackGroupVGIcon;

extern QIcon* noscaleSVGIcon[3];
extern QIcon* ankerSVGIcon;
extern QIcon* settingsSVGIcon;
extern QIcon* transportSVGIcon;
extern QIcon* bigtimeSVGIcon;
extern QIcon* mixerSVGIcon;
extern QIcon* plusSVGIcon;
extern QIcon* minusSVGIcon;
extern QIcon* keySVGIcon;
extern QIcon* delSelTracksSVGIcon;
extern QIcon* duplSelTrackSVGIcon;
extern QIcon* duplSelTracksSVGIcon;

extern QIcon* routeAutoAdjustSVGIcon;
extern QIcon* routeSelSourceSVGIcon;
extern QIcon* routeSelDestSVGIcon;
extern QIcon* routeSourceSVGIcon;
extern QIcon* routeDestSVGIcon;

extern QIcon* gridOnSVGIcon;
extern QIcon* rangeToSelectionSVGIcon;
extern QIcon* quantizeSVGIcon;
extern QIcon* clearSVGIcon;
extern QIcon* downmixOffSVGIcon;
extern QIcon* downmixOnSVGIcon;
extern QIcon* downmixTrackSVGIcon;
extern QIcon* downmixStateSVGIcon;
extern QIcon* restartSVGIcon;
extern QIcon* snapshotSVGIcon;
extern QIcon* emptyBarSVGIcon;
extern QIcon* lockSVGIcon;

extern QIcon* midiResetSVGIcon;
extern QIcon* midiInitSVGIcon;
extern QIcon* midiLocalOffSVGIcon;
extern QIcon* midiTransformSVGIcon;
extern QIcon* midiInputTransformSVGIcon;
extern QIcon* midiInputTransposeSVGIcon;
extern QIcon* midiInputFilterSVGIcon;
extern QIcon* midiInputRemoteSVGIcon;
extern QIcon* midiSyncSVGIcon;
extern QIcon* midiExportImportSVGIcon;

extern QIcon* cutSVGIcon;
extern QIcon* copySVGIcon;
extern QIcon* pasteSVGIcon;
extern QIcon* pasteDialogSVGIcon;
extern QIcon* pasteCloneSVGIcon;
extern QIcon* copyRangeSVGIcon;
extern QIcon* deleteSVGIcon;
extern QIcon* pasteSelectedTrackSVGIcon;
extern QIcon* pasteCloneSelectedTrackSVGIcon;

extern QIcon* eyeIcon;
extern QIcon* eyeCrossedIcon;
extern QIcon* eyeGreyIcon;

extern QIcon* dummySVGIcon;

//----------------------------------
// Cursors
//----------------------------------

extern QCursor* editpasteSCursor;
extern QCursor* editpasteCloneSCursor;

extern QCursor* pencilCursor;
extern QCursor* glueCursor;
extern QCursor* cutterCursor;
extern QCursor* zoomCursor;
extern QCursor* zoomAtCursor;
extern QCursor* deleteCursor;
extern QCursor* drawCursor;
extern QCursor* mutePartsCursor;
extern QCursor* handCursor;
extern QCursor* closedHandCursor;
//extern QCursor* magnetCursor;
//extern QCursor* customMoveCursor;
extern QCursor* pencilMove4WayCursor;
extern QCursor* pencilMoveHorizCursor;
extern QCursor* pencilMoveVertCursor;

} // namespace MusEGui

#endif

