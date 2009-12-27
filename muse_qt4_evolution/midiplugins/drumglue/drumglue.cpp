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

#include "drumgluegui.h"
#include "drumglue.h"
#include "midi.h"
#include "midievent.h"



//---------------------------------------------------------
//   DrumInstrument - get next index
//     - next output instrument to use
//---------------------------------------------------------
int DrumInstrument::getNextIndex(int /*velocity*/)
{
	// for now we simply do a round robin
	//
	// future improvements are to keep track of
	// the time since the last hit and the
	// weight set for each instrument.
	// the incoming velocity should be checked that it's in range
	if (outputInstruments.size() == 0)
		return -1;
	
	if (lastOutputIndex+1 > outputInstruments.size()-1)
		return 0;
	else
		return lastOutputIndex+1;
}

//---------------------------------------------------------
//   DrumInstrument - get velocity
//     - velocity value to use
//---------------------------------------------------------
int DrumInstrument::getVelocity(int /*index*/, int velocity)
{
	// for now we just return the same velocity
	// future improvements are to allow for some
	return velocity;
}


//---------------------------------------------------------
//   DrumGlue - constructor
//---------------------------------------------------------
DrumGlue::DrumGlue(const char* name, const MempiHost* h)
   : Mempi(name, h)
      {
      gui = 0;
      saveData = NULL;
      }

//---------------------------------------------------------
//   DrumGlue - destructor
//---------------------------------------------------------
DrumGlue::~DrumGlue()
      {
      if (gui)
            delete gui;
      if (saveData)
      		delete saveData;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

bool DrumGlue::init()
      {
      gui = new DrumGlueGui(this, 0);
      gui->setWindowTitle("MusE: "+QString(name()));
      gui->show();
      return false;
      }

//---------------------------------------------------------
//   getGeometry
//---------------------------------------------------------

void DrumGlue::getGeometry(int* x, int* y, int* w, int* h) const
      {
      QPoint pos(gui->pos());
      QSize size(gui->size());
      *x = pos.x();
      *y = pos.y();
      *w = size.width();
      *h = size.height();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void DrumGlue::setGeometry(int x, int y, int w, int h)
      {
      gui->resize(QSize(w, h));
      gui->move(QPoint(x, y));
      }

//---------------------------------------------------------
//   process
//---------------------------------------------------------

void DrumGlue::process(unsigned , unsigned , MidiEventList* il, MidiEventList* ol)
      {

      for (iMidiEvent i = il->begin(); i != il->end(); ++i) {
            MidiEvent temp=*i;
            if (temp.isNote() && !temp.isNoteOff())
                {
					foreach(DrumInstrument *di, drumInstruments) {
						if (temp.dataA() == di->inKey) {
							int inVelocity = temp.dataB();
							int instrumentIdx = di->getNextIndex(inVelocity);
							if (instrumentIdx==-1) {
								// no instrument defined, yet, skip it
								break;
							}
							int outKey = di->outputInstruments[instrumentIdx]->outKey;
							int outVelocity= di->getVelocity(instrumentIdx, inVelocity);
							printf("inKey=%d outKey =%d outVelocity=%d instrumentIdx=%d\n", di->inKey, outKey, outVelocity, instrumentIdx);
							temp.setA(outKey);
							temp.setB(outVelocity);
							
							ol->insert(temp);	// note on
							
							temp.setB(0);
							ol->insert(temp);	// note off
							
							di->lastOutputIndex = instrumentIdx;
							di->outputTime = temp.time();
							break;
							}
                    	}
                 }
            if (temp.isNoteOff()) ; // we just throw it away, we will insert noteoffs for each note on
            }
      }

//
// getInitData - return configuration to MusE
//
void DrumGlue::getInitData(int* n, const unsigned char** p) const
      {
       QString saveStr;
       
       foreach (DrumInstrument *di, drumInstruments) {
       		QString drumline = "DRUM " +di->name + " " + QString("%1").arg(di->inKey) + "\n";
       		saveStr.append(drumline);
       		foreach (DrumOutputInstrument *doi, di->outputInstruments) {
				QString outputline = "OUTPUT " + 
									QString("%1").arg(doi->outKey) + " " + 
									QString("%1").arg(doi->lowestVelocity) + " " + 
									QString("%1").arg(doi->highestVelocity) + " " + 
									QString("%1").arg(doi->prefer) + " " + 
									QString("%1").arg(doi->preferFast) + "\n";
				saveStr.append(outputline);
       			
       		}
       }
       
      *n = saveStr.length();
      
      if (saveData)
      	delete saveData;
      	
      
      saveData = new unsigned char[saveStr.length()];
      
      strncpy((char*)saveData, saveStr.toLatin1().data(), saveStr.length());
      saveData[saveStr.length()]=0;
      printf("getInitData -\n%s\n",saveData);
      
      *p = saveData;

      }

void DrumGlue::setInitData(int n, const unsigned char* p)
      {
      if (saveData)
      		delete saveData;
      saveData = new unsigned char[n+1];
      		
      strncpy((char*)saveData,(char*)p,n);
      saveData[n]=0;
      
      QString loadStr = (char*)saveData;
      printf("setInitData -\n%s\n",saveData);
      	
      QStringList loadList = loadStr.split("\n");
      	
      DrumInstrument *currentInstrument=NULL;
      foreach (QString line, loadList) {
      		QStringList splitLine = line.split(" ");
      		if (splitLine[0] == "DRUM") {
      			if (currentInstrument)
      				drumInstruments.append(currentInstrument);
      			
      			currentInstrument = new DrumInstrument();
      			currentInstrument->name = splitLine[1];
      			currentInstrument->inKey = splitLine[2].toInt();
      		}
      		if (splitLine[0] == "OUTPUT") {
				DrumOutputInstrument *doi = new DrumOutputInstrument;
				doi->outKey = splitLine[1].toInt();
				doi->lowestVelocity = splitLine[2].toInt();
				doi->highestVelocity = splitLine[3].toInt();
				doi->prefer = splitLine[4].toInt();
				doi->preferFast = splitLine[5].toInt();
				currentInstrument->outputInstruments.append(doi);
      		}
      }
      if (currentInstrument)
		drumInstruments.append(currentInstrument);
      
      if (gui)
            gui->init();
      }

//---------------------------------------------------------
//   inst
//---------------------------------------------------------

static Mempi* instantiate(const char* name, const MempiHost* h)
      {
      return new DrumGlue(name, h);
      }

extern "C" {
      static MEMPI descriptor = {
            "DrumGlue",
            "Drum instrument mux filter",
            "0.1",                  // filter version string
            MEMPI_FILTER,           // plugin type
            MEMPI_MAJOR_VERSION, MEMPI_MINOR_VERSION,
            instantiate
            };

      const MEMPI* mempi_descriptor() { return &descriptor; }
      }

