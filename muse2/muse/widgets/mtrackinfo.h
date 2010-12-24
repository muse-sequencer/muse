//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
//=========================================================

#ifndef __MTRACKINFO_H__
#define __MTRACKINFO_H__

#include "ui_mtrackinfobase.h"

class Track;

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public QWidget, public Ui::MidiTrackInfoBase 
{
      Q_OBJECT
      Track* selected;
      bool _midiDetect;
      int program, pan, volume;
      
   private slots:
      void iOutputChannelChanged(int);
      void iOutputPortChanged(int);
      void iProgHBankChanged();
      void iProgLBankChanged();
      void iProgramChanged();
      void iProgramDoubleClicked();
      void iLautstChanged(int);
      void iLautstDoubleClicked();
      void iTranspChanged(int);
      void iAnschlChanged(int);
      void iVerzChanged(int);
      void iLenChanged(int);
      void iKomprChanged(int);
      void iPanChanged(int);
      void iPanDoubleClicked();
      void songChanged(int);
      void instrPopup();
      void recordClicked();
      void progRecClicked();
      void volRecClicked();
      void panRecClicked();
      void recEchoToggled(bool);
      void inRoutesPressed();
      void outRoutesPressed();
      void routingPopupMenuActivated(QAction*);
      //void routingPopupViewActivated(const QModelIndex&);
      
   protected slots:
      virtual void heartBeat();

   signals:
      void outputPortChanged(int);  
   
   public:
      MidiTrackInfo(QWidget*, Track* = 0);
      Track* track() const { return selected; }
      void setTrack(Track*); 
      void setLabelText();
      void setLabelFont();
      void updateTrackInfo(int);
};



#endif

