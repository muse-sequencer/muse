//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2008 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2005- Werner Schweer (ws@seh.de)
// Copyright: See COPYING file that comes with this distribution
//=========================================================

#include "globalinstrumentview.h"
#include "outputinstrumentview.h"
#include "drumglue.h"

//---------------------------------------------------------
//   GlobalInstrumentView
//---------------------------------------------------------

GlobalInstrumentView::GlobalInstrumentView(DrumGlue* f, QWidget* parent, QString name)
  : QWidget(parent)
    {
      setupUi(this);
      drumGlue = f;
      
      instrumentName=name;
      
      DrumInstrument *di =getCurrentOutputInstrument();
      printf("di->inKey=%d\n", di->inKey);
      if (di)
      		inKeySpinBox->setValue(di->inKey);
      
      connect (addOutputButton, SIGNAL(clicked()), this, SLOT(addOutput()));
      connect (editOutputButton, SIGNAL(clicked()), this, SLOT(editOutput()));
      connect (removeOutputButton, SIGNAL(clicked()), this, SLOT(removeOutput()));
      connect (inKeySpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateInKey()));
      updateList();
    }


DrumInstrument *GlobalInstrumentView::getCurrentOutputInstrument()
{
	// get the global instrument belonging to this instance
	QList<DrumInstrument *>::iterator iter = drumGlue->drumInstruments.begin();
	while(iter != drumGlue->drumInstruments.end()) {
		printf("name = %s instrumentName= %s\n", (*iter)->name.toLatin1().data(), instrumentName.toLatin1().data());
		if ((*iter)->name == instrumentName) {
			break;
		}
		iter++;
	}
	if (iter == drumGlue->drumInstruments.end()) {
		printf("Reached the end without getting a hit\n");
		return NULL;
	}
	return *iter;
}

void GlobalInstrumentView::addOutput()
	{
	DrumInstrument *di = getCurrentOutputInstrument();
	if (!di)
		return;
	// create new output
	DrumOutputInstrument *doi = new DrumOutputInstrument;
	di->outputInstruments.append(doi);
	
	OutputInstrumentView *outputView = new OutputInstrumentView(doi,this);
	int res = outputView->exec();
	if (res == QDialog::Rejected) {
		// roll back
		di->outputInstruments.removeAll(doi);
		delete doi;
	}
	delete outputView;
	updateList();
	}

void GlobalInstrumentView::editOutput()
	{
	DrumInstrument *di = getCurrentOutputInstrument();
	
	int currIdx = outputListWidget->currentRow();
	if (currIdx < 0 || currIdx >= di->outputInstruments.size()) {
		printf("out of range!!\n");
		return;
	}
		
	DrumOutputInstrument *doi = di->outputInstruments[currIdx];
	
	DrumOutputInstrument doi_backup = *doi;
	OutputInstrumentView *outputView = new OutputInstrumentView(doi,this);
	int res = outputView->exec();
	if (res == QDialog::Rejected) {
		// roll back
		*doi = doi_backup;
	}
	delete outputView;
	updateList();
	}

void GlobalInstrumentView::removeOutput()
	{
 	int ret = QMessageBox::warning(this, tr("Remove output"),
		tr("Are you sure you want to remove current output?"),
		QMessageBox::No,
		QMessageBox::Yes);
	if (ret == QMessageBox::Yes) {
		DrumInstrument *di = getCurrentOutputInstrument();
		
		int currIdx = outputListWidget->currentRow();
		if (currIdx < 0 || currIdx >= di->outputInstruments.size()) {
			printf("out of range!!\n");
			return;
		}
			
		DrumOutputInstrument *doi = di->outputInstruments[currIdx];
		di->outputInstruments.removeAll(doi);
		delete doi;
	}		
	updateList();
	}
	
void GlobalInstrumentView::updateList()
{
	printf("updateList\n");
	outputListWidget->clear();
	
	QList<DrumInstrument *>::iterator iter = drumGlue->drumInstruments.begin();
	while(iter != drumGlue->drumInstruments.end()) {
		printf("name = %s instrumentName= %s\n", (*iter)->name.toLatin1().data(), instrumentName.toLatin1().data());
		if ((*iter)->name == instrumentName) {
			printf("updating current list\n");
			foreach (DrumOutputInstrument *doi, (*iter)->outputInstruments) {
				QListWidgetItem *outDrumItem = new QListWidgetItem(outputListWidget);
				QString str = QString("%1 %2 %3 %4 %5").arg(doi->outKey).arg(doi->lowestVelocity).arg(doi->highestVelocity).arg(doi->prefer).arg(doi->preferFast);
				printf("setting item to %s\n",str.toLatin1().data());
				outDrumItem->setText(str);
			}
		}
		iter++;
	}
}

void GlobalInstrumentView::updateInKey()
{
	QList<DrumInstrument *>::iterator iter = drumGlue->drumInstruments.begin();
	while(iter != drumGlue->drumInstruments.end()) {
		if ((*iter)->name == instrumentName) {
			(*iter)->inKey = inKeySpinBox->value();
		}
		iter++;
	}

}
