//=========================================================
//  MusE
//  Linux Music Editor
//  sig_tempo_toolbar.h
//  (C) Copyright 2012 Florian Jung (flo93@users.sourceforge.net)
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

#ifndef __SIG_TEMPO_TOOLBAR_H__
#define __SIG_TEMPO_TOOLBAR_H__

#include <QTimer>
#include <QDateTime>
#include <QToolBar>

#include "type_defs.h"

class QWidget;
class QLabel;
class QString;
class QToolButton;

namespace MusEGui
{
  class TempoEdit;
  class IconButton;
  class SigEdit;

  class TempoToolbar : public QToolBar
  {
    Q_OBJECT
    
    private:
      TempoEdit* tempo_edit;
      QToolButton *tap_button;
      IconButton* _masterButton;

      QTimer tap_timer;
      QDateTime last_tap_time;
      QTimer *blink_timer;
      bool blinkButtonState;
      QString buttonDefColor;
                  
      void init();
      
    public:
      TempoToolbar(QWidget* parent = nullptr);
      TempoToolbar(const QString& title, QWidget* parent = nullptr);
      bool masterTrack() const;
      void setMasterTrack(bool);
    
    signals:
      void returnPressed();
      void escapePressed();
      void masterTrackChanged(bool);
            
    private slots:
      void pos_changed(int,unsigned,bool);
      void song_changed(MusECore::SongChangedStruct_t);
      void syncChanged(bool);
      void tap_tempo();
      void tap_timer_signal();
      void masterToggled(bool);
      void tapButtonBlink();
  };


  class SigToolbar : public QToolBar
  {
    Q_OBJECT
    
    private:
//      QLabel* label;
      SigEdit* sig_edit;   
      
      void init();
            
    public:
      SigToolbar(QWidget* parent = 0);
      SigToolbar(const QString& title, QWidget* parent = 0);
            
    signals:
      void returnPressed();
      void escapePressed();
    
    private slots:
      void pos_changed(int,unsigned,bool);
      void song_changed(MusECore::SongChangedStruct_t);
  };
  
}

#endif
