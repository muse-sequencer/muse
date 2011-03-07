//=========================================================
//  MusE
//  Linux Music Editor
//
//    drumglue - filter
//
//  (C) Copyright 2008 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2005- Werner Schweer (ws@seh.de)
// Copyright: See COPYING file that comes with this distribution
//=========================================================

#ifndef __DRUMGLUE_H__
#define __DRUMGLUE_H__

#include <QtGui>

#include "../libmidiplugin/mempi.h"

#include "drumgluegui.h"
//---------------------------------------------------------
//   drumglue - filter
//---------------------------------------------------------

struct DrumOutputInstrument {
	int outKey;		// key to send
	int lowestVelocity;  // lower velocity valid for this instrument
	int highestVelocity; // highest velocity valid for this instrument
	bool prefer;		 // true if this instrument is preferred
	bool preferFast;	 // true if this instrument is preferred for fast transitions
};

class DrumInstrument {
	public:
	DrumInstrument() 
	{ 
		inKey=0;
		lastOutputIndex=0;
		outputTime=0;
	}
	
	int getNextIndex(int velocity);
	int getVelocity(int index, int velocity);
	
	
	int inKey;		// the key which triggers this instrument
	QString name;
	QList <DrumOutputInstrument *> outputInstruments;

//  storage of runtime variables
    int lastOutputIndex;
    unsigned int outputTime;
};




class DrumGlue : public Mempi {
      friend class DrumGlueGui;
      friend class GlobalInstrumentView;
      friend class OutputInstrumentView;
      
      QList<DrumInstrument *> drumInstruments;

      DrumGlueGui* gui;
      
      mutable unsigned char *saveData;

      virtual void process(unsigned, unsigned, MidiEventList*, MidiEventList*);

   public:
      DrumGlue(const char* name, const MempiHost*);
      ~DrumGlue();
      virtual bool init();

      virtual bool hasGui() const      { return true;             }
      virtual bool guiVisible() const  { return gui->isVisible(); }
      virtual void showGui(bool val)   { gui->setShown(val);      }
      virtual void getGeometry(int* x, int* y, int* w, int* h) const;
      virtual void setGeometry(int, int, int, int);

      virtual void getInitData(int*, const unsigned char**) const;
      virtual void setInitData(int, const unsigned char*);
      };

#endif

