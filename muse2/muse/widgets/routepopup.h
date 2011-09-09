//=========================================================
//  MusE
//  Linux Music Editor
//
//  RoutePopupMenu.h 
//  (C) Copyright 2011 Tim E. Real (terminator356 A T sourceforge D O T net)
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
//=============================================================================

#ifndef __ROUTEPOPUPMENU_H__
#define __ROUTEPOPUPMENU_H__

#include <QObject>

class Track;
class AudioTrack;
class QWidget;
class QString;
class QAction;
class QPoint;

namespace MusEWidget {

class PopupMenu;

class RoutePopupMenu : public QObject
{
  Q_OBJECT
  
    PopupMenu* _pup;
    Track* _track;
    // Whether the route popup was shown by clicking the output routes button, or input routes button.
    bool _isOutMenu;
    
    void init();
    void prepare();
    
    int addMenuItem(AudioTrack* track, Track* route_track, PopupMenu* lb, int id, int channel, 
                    int channels, bool isOutput);
    int addAuxPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput);
    int addInPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput);
    int addOutPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput);
    int addGroupPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput);
    int addWavePorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput);
    int addSyntiPorts(AudioTrack* t, PopupMenu* lb, int id, int channel, int channels, bool isOutput);
    int addMultiChannelPorts(AudioTrack* t, PopupMenu* pup, int id, bool isOutput);
    int nonSyntiTrackAddSyntis(AudioTrack* t, PopupMenu* lb, int id, bool isOutput);
    int addMidiPorts(AudioTrack* t, PopupMenu* pup, int id, bool isOutput);
    
  private slots:
    void popupActivated(QAction*);
    void songChanged(int);
  
  public:
    RoutePopupMenu(QWidget* parent = 0, Track* track = 0, bool isOutput = false);
    RoutePopupMenu(const QString& title, QWidget* parent = 0, Track* track = 0, bool isOutput = false);
    ~RoutePopupMenu();
    
    void updateRouteMenus();
    void exec(Track* track = 0, bool isOutput = false);
    void exec(const QPoint& p, Track* track = 0, bool isOutput = false);
    void popup(const QPoint& p, Track* track = 0, bool isOutput = false);
};

}

#endif
