//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrlcombo.cpp,v 1.1.1.1 2003/10/27 18:55:02 wschweer Exp $
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

#include "ctrlcombo.h"

namespace MusEGui {

//---------------------------------------------------------
//   CtrlComboBox
//---------------------------------------------------------

CtrlComboBox::CtrlComboBox(QWidget* parent)
   : QComboBox(parent)
      {
      const char* ctxt[] = {
            "No Ctrl",    "BankSelMSB", "Modulation",  "BreathCtrl",
            "Control 3",  "Foot Ctrl",  "Porta Time",  "DataEntMSB",
            "MainVolume", "Balance",    "Control 9",   "Pan",
            "Expression", "Control 12", "Control 13",  "Control 14",
            "Control 15", "Gen.Purp.1", "Gen.Purp.2",  "Gen.Purp.3",
            "Gen.Purp.4", "Control 20", "Control 21",  "Control 22",
            "Control 23", "Control 24", "Control 25",  "Control 26",
            "Control 27", "Control 28", "Control 29",  "Control 30",
            "Control 31", "BankSelLSB", "Modul. LSB",  "BrthCt.LSB",
            "Control 35", "FootCt.LSB", "Port.T LSB",  "DataEntLSB",
            "MainVolLSB", "BalanceLSB", "Control 41",  "Pan LSB",
            "Expr. LSB",  "Control 44", "Control 45",  "Control 46",
            "Control 47", "Gen.P.1LSB", "Gen.P.2LSB",  "Gen.P.3LSB",
            "Gen.P.4LSB", "Control 52", "Control 53",  "Control 54",
            "Control 55", "Control 56", "Control 57",  "Control 58",
            "Control 59", "Control 60", "Control 61",  "Control 62",
            "Control 63", "Sustain",    "Porta Ped",   "Sostenuto",
            "Soft Pedal", "Control 68", "Hold 2",      "Control 70",
            "HarmonicCo", "ReleaseTime", "Attack Time", "Brightness",
            "Control 75", "Control 76", "Control 77",  "Control 78",
            "Control 79", "Gen.Purp.5", "Gen.Purp.6",  "Gen.Purp.7",
            "Gen.Purp.8", "Porta Ctrl", "Control 85",  "Control 86",
            "Control 87", "Control 88", "Control 89",  "Control 90",
            "Effect1Dep", "Effect2Dep", "Effect3Dep",  "Effect4Dep",
            "Phaser Dep", "Data Incr",  "Data Decr",   "NRPN LSB",
            "NRPN MSB",   "RPN LSB",    "RPN MSB",     "Control102",
            "Control103", "Control104", "Control105",  "Control106",
            "Control107", "Control108", "Control109",  "Control110",
            "Control111", "Control112", "Control113",  "Control114",
            "Control115", "Control116", "Control117",  "Control118",
            "Control119", "AllSndOff",  "Reset Ctrl",  "Local Ctrl",
            "AllNoteOff", "OmniModOff", "OmniModeOn",  "MonoModeOn",
            "PolyModeOn"
            };
      for (unsigned int i = 0; i < sizeof(ctxt)/sizeof(*ctxt); ++i)
             insertItem(i, QString(ctxt[i]));
      }

} // namespace MusEGui
