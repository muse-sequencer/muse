//=========================================================
//  MusE
//  Linux Music Editor
//  sig_tempo_toolbar.cpp
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


#include "sig_tempo_toolbar.h"
#include "tempolabel.h"
#include "awl/sigedit.h"
#include "song.h"

#include <QLayout>
#include <QLabel>


namespace MusEGui
{
	TempoToolbarWidget::TempoToolbarWidget(QWidget* p) : QWidget(p)
	{
		tempo_edit=new MusEGui::TempoEdit(this);
		tempo_edit->setToolTip(tr("tempo at current position"));
		tempo_edit->setFocusPolicy(Qt::StrongFocus);
		
		label=new QLabel(tr("Tempo: "),this);
		
		layout = new QHBoxLayout(this);
		layout->setContentsMargins(0,0,0,0);
		layout->setSpacing(1);
		layout->addWidget(label);
		layout->addWidget(tempo_edit);
		
		connect(MusEGlobal::song, SIGNAL(songChanged(int)), this, SLOT(song_changed(int)));
		connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(pos_changed(int,unsigned,bool)));
		
		connect(tempo_edit, SIGNAL(tempoChanged(double)), MusEGlobal::song, SLOT(setTempo(double)));
		connect(tempo_edit, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
		connect(tempo_edit, SIGNAL(escapePressed()), SIGNAL(escapePressed()));

		song_changed(-1);
	}
	
	void TempoToolbarWidget::pos_changed(int,unsigned,bool)
	{
		song_changed(SC_TEMPO);
	}

	void TempoToolbarWidget::song_changed(int type)
	{
		if (type & SC_TEMPO)
		{
			int tempo = MusEGlobal::tempomap.tempo(MusEGlobal::song->cpos());
			tempo_edit->blockSignals(true);
			tempo_edit->setValue(double(60000000.0/tempo));
			tempo_edit->blockSignals(false);
		}
		if (type & SC_MASTER)
		{
			tempo_edit->setEnabled(MusEGlobal::song->masterFlag());
			label->setEnabled(MusEGlobal::song->masterFlag());
		}
	}

	SigToolbarWidget::SigToolbarWidget(QWidget* p) : QWidget(p)
	{
		sig_edit=new Awl::SigEdit(this);
		sig_edit->setFocusPolicy(Qt::StrongFocus);
		sig_edit->setValue(AL::TimeSignature(4, 4));
		sig_edit->setToolTip(tr("time signature at current position"));
		
		label=new QLabel(tr("Signature: "),this);
		
		layout = new QHBoxLayout(this);
		layout->setContentsMargins(0,0,0,0);
		layout->setSpacing(1);
		layout->addWidget(label);
		layout->addWidget(sig_edit);
		
		connect(MusEGlobal::song, SIGNAL(songChanged(int)), this, SLOT(song_changed(int)));
		connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(pos_changed(int,unsigned,bool)));
		
		connect(sig_edit, SIGNAL(valueChanged(const AL::TimeSignature&)), MusEGlobal::song, SLOT(setSig(const AL::TimeSignature&)));
		connect(sig_edit, SIGNAL(returnPressed()), SIGNAL(returnPressed()));
		connect(sig_edit, SIGNAL(escapePressed()), SIGNAL(escapePressed()));

		song_changed(-1);
	}
	
	void SigToolbarWidget::pos_changed(int,unsigned,bool)
	{
		song_changed(SC_SIG);
	}
	
	void SigToolbarWidget::song_changed(int type)
	{
		if (type & SC_SIG)
		{
			int z, n;
			AL::sigmap.timesig(MusEGlobal::song->cpos(), z, n);
			sig_edit->blockSignals(true);
			sig_edit->setValue(AL::TimeSignature(z, n));
			sig_edit->blockSignals(false);
		}
		if (type & SC_MASTER)
		{
			sig_edit->setEnabled(MusEGlobal::song->masterFlag());
			label->setEnabled(MusEGlobal::song->masterFlag());
		}
	}
}
