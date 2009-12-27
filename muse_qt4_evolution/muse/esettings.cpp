//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2004 Mathias Lundgren <lunar_shuttle@users.sourceforge.net>
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

#include "esettings.h"
#include "al/xml.h"
#include "midieditor.h" //only for debug prints

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void CtrlEditSettings::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "controller")
                  controller = i;
            else if (tag == "height")
                  height = i;
            else
                  printf("MusE:CtrlEditSettings: unknown tag: %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void CtrlEditSettings::writeStatus(Xml& xml)
      {
      xml.tag("ctrleditsettings");
      xml.intTag("height", height);
      xml.intTag("controller", controller);
      xml.etag("ctrleditsettings");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void EditorSettings::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "width")
                  _width = i;
            else if (tag == "height")
                  _height = i;
            else if (tag == "x")
                  _x = i;
            else if (tag == "y")
                  _y = i;
            else if (tag == "raster")
                  _raster = i;
            else
                  printf("MusE:EditorSettings: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void EditorSettings::writeStatus(Xml& xml) const
      {
      xml.tag("editorsettings");
      xml.intTag("width", _width);
      xml.intTag("height", _height);
      xml.intTag("x", _x);
      xml.intTag("y", _y);
      xml.intTag("raster", _raster);
      xml.etag("editorsettings");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void GraphEditorSettings::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "editorsettings") {
                  EditorSettings::readStatus(node.firstChild());
                  }
            else if (tag == "xmag")
                  _xmag = s.toDouble();
            else if (tag == "ymag")
                  _ymag = s.toDouble();
            else if (tag == "xpos")
                  _pos.setX(i);
            else if (tag == "ypos")
                  _pos.setY(i);
            else
                  printf("MusE:GraphEditorSettings: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void GraphEditorSettings::writeStatus(Xml& xml) const
      {
      xml.tag("grapheditorsettings");
      EditorSettings::writeStatus(xml);
      xml.doubleTag("xmag", _xmag);
      xml.doubleTag("ymag", _ymag);
      xml.intTag("xpos", _pos.x());
      xml.intTag("ypos", _pos.y());
      xml.etag("grapheditorsettings");
      }

bool ExtEditorSettings::_steprec = false;
bool ExtEditorSettings::_midiin  = false;


//---------------------------------------------------------
//   ExtEditorSettings constructor
//---------------------------------------------------------

ExtEditorSettings::ExtEditorSettings(int r, int w, int h, int x, int y, double xm, double ym, QPoint pos, int q, int apply)
   : GraphEditorSettings(r, w, h, x, y, xm, ym, pos), _quant(q), _applyTo(apply)
      {
      for (int i=0; i<MAXNOOFCTRLEDITSETTINGS; i++)
                  ctrlEdits[i] = 0;
      _numOfCtrlEdits = 0;
      }

//---------------------------------------------------------
//   ExtEditorSettings destructor
//---------------------------------------------------------
ExtEditorSettings::~ExtEditorSettings()
      {
      for (int i=0; i<MAXNOOFCTRLEDITSETTINGS; i++) {
            if (this->ctrlEdits[i])
                  delete ctrlEdits[i];
            }
      }

//---------------------------------------------------------
//   setControlEditSettings
//---------------------------------------------------------

void ExtEditorSettings::setControlEditSettings(int pos, CtrlEditSettings* c)
      {
      if (ctrlEdits[pos])
            delete ctrlEdits[pos];

       ctrlEdits[pos] = c;
      }
//---------------------------------------------------------
//   clone
//---------------------------------------------------------

EditorSettings* ExtEditorSettings::clone()
      {
      ExtEditorSettings* newSettings = new ExtEditorSettings(_raster, _width, _height, _x, _y, _xmag, _ymag, _pos, _quant, _applyTo);
      CtrlEditSettings* c;
      for (int i=0; i<_numOfCtrlEdits; i++) {
            c = new CtrlEditSettings();
            *c = *(ctrlEdits[i]); //copy
            newSettings->setControlEditSettings(i, c);
            }
      newSettings->setControlEditSize(_numOfCtrlEdits);
      return (EditorSettings*) newSettings;
      }

//---------------------------------------------------------
//   readStatic
//---------------------------------------------------------

void ExtEditorSettings::readStatic(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "steprec")
                  _steprec = i;
            else if (tag == "midiin")
                  _midiin = i;
            else
                  printf("MusE:ExtEditorSettings Static: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatic
//---------------------------------------------------------

void ExtEditorSettings::writeStatic(Xml& xml)
      {
      xml.tag("exteditorstatic");
      xml.intTag("steprec", _steprec);
      xml.intTag("midiin", _midiin);
      xml.etag("exteditorstatic");
     }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void ExtEditorSettings::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "grapheditorsettings")
                  GraphEditorSettings::readStatus(node.firstChild());
            else if (tag == "quant")
                  _quant = i;
            else if (tag == "applyTo")
                  _applyTo = i;
            else if (tag == "steprec")
                  _steprec = i;
            else if (tag == "midiin")
                  _midiin = i;
            else if (tag == "ctrleditsettings") {
                  ctrlEdits[_numOfCtrlEdits] = new CtrlEditSettings();
                  ctrlEdits[_numOfCtrlEdits]->readStatus(node.firstChild());
                  _numOfCtrlEdits++;
                  }
            else
                  printf("MusE:ExtEditorSettings: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void ExtEditorSettings::writeStatus(Xml& xml) const
      {
      //TODO: The ctrlEdits need to be updated (created) before trying to write status of ctrlEdits...
      xml.tag("exteditorsettings");
      GraphEditorSettings::writeStatus(xml);
      xml.intTag("quant", _quant);
      xml.intTag("applyTo", _applyTo);
      xml.intTag("steprec", _steprec);
      xml.intTag("midiin", _midiin);
      for (int i=0; i<_numOfCtrlEdits; i++) {
            ctrlEdits[i]->writeStatus(xml);
            }
      xml.etag("exteditorsettings");
      }

//
// DrumEditorSettings static init default values:
//
int DrumEditorSettings::_quantInit  = 96;
int DrumEditorSettings::_rasterInit = 96;
int DrumEditorSettings::_widthInit  = 600;
int DrumEditorSettings::_heightInit = 400;

//---------------------------------------------------------
//   DrumEditorSettings
//---------------------------------------------------------

DrumEditorSettings::DrumEditorSettings(int r, int w, int h, int x, int y,
  double xm, double ym, QPoint pos, int q, int apply, int dl, int dw)
      : ExtEditorSettings(r, w, h, x, y, xm, ym, pos, q, apply),
        _dlistWidth(dl), _dcanvasWidth(dw)
      {
      // For uninitialized values, go with static default values instead
      if (w == ES_UNINIT)
            _width  = _widthInit;
      if (h == ES_UNINIT)
            _height = _heightInit;
      if (r == ES_UNINIT)
            _raster = _rasterInit;
      if (q == ES_UNINIT)
            _quant = _quantInit;
      }

//---------------------------------------------------------
//   clone
//---------------------------------------------------------

EditorSettings* DrumEditorSettings::clone()
      {
      DrumEditorSettings* newSettings = new DrumEditorSettings(_raster, _width, _height, _x, _y, _xmag, _ymag, _pos, _quant, _applyTo, _dlistWidth, _dcanvasWidth);
      for (int i=0; i<_numOfCtrlEdits; i++) {
            CtrlEditSettings* c = new CtrlEditSettings();
            *c = *(ctrlEdits[i]); //copy
            newSettings->setControlEditSettings(i, c);
            }
      newSettings->setControlEditSize(_numOfCtrlEdits);
      return (EditorSettings*) newSettings;
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void DrumEditorSettings::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "exteditorsettings")
                  ExtEditorSettings::readStatus(node.firstChild());
            else if (tag == "dlistwidth")
                  _dlistWidth = i;
            else if (tag == "dcanvaswidth")
                  _dcanvasWidth = i;
            else
                  printf("MusE:DrumEditorSettings: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void DrumEditorSettings::writeStatus(Xml& xml) const
      {
      xml.tag("drumeditorsettings");
      ExtEditorSettings::writeStatus(xml);
      xml.intTag("dlistwidth", _dlistWidth);
      xml.intTag("dcanvaswidth", _dcanvasWidth);
      xml.etag("drumeditorsettings");
      }

//---------------------------------------------------------
//   setStaticInitValues
//---------------------------------------------------------
void DrumEditorSettings::setStaticInitValues(int widthinit, int heightinit, int rasterinit, int quantinit)
      {
      _widthInit = widthinit;
      _heightInit = heightinit;
      _rasterInit = rasterinit;
      _quantInit = quantinit;
      }

//---------------------------------------------------------
//   writeStatic
//! write static configuration values for pianoroll
//---------------------------------------------------------

void DrumEditorSettings::writeStatic(Xml& xml)
      {
      xml.tag("drumeditorstatic");
      xml.intTag("quantDefault", _quantInit);
      xml.intTag("rasterDefault", _rasterInit);
      xml.intTag("widthDefault", _widthInit);
      xml.intTag("heightDefault", _heightInit);
      xml.etag("drumeditorstatic");
     }

//---------------------------------------------------------
//   readStatic
//---------------------------------------------------------

void DrumEditorSettings::readStatic(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "quantDefault")
                  _quantInit = i;
            else if (tag == "rasterDefault")
                  _rasterInit = i;
            else if (tag == "widthDefault")
                  _widthInit = i;
            else if (tag == "heightDefault")
                  _heightInit = i;
            else
                  printf("MusE:DrumEditorSettings Static: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void DrumEditorSettings::dump()
      {
//      printf("%x: DrumEditorSettings: q=%d r=%d w=%d h=%d dlw=%d dcw=%d xmag: %d pos: %d ypos: %d\n", this, _quant, _raster, _width, _height, _dlistWidth, _dcanvasWidth, _xmag, _pos, _ypos);
      }

//
// PianorollSettings static init default values:
//
int PianorollSettings::_quantInit  = 96;
int PianorollSettings::_rasterInit = 96;
int PianorollSettings::_widthInit  = 600;
int PianorollSettings::_heightInit = 400;

PianorollSettings::PianorollSettings(int r, int w, int h, int x, int y,
   double xm, double ym, QPoint pos, int q, int apply,
   int qs, int qlim, int cmode, bool qlen, int pw)
   : ExtEditorSettings(r, w, h, x, y, xm, ym, pos, q, apply),
   _quantStrength(qs), _quantLimit(qlim), _colorMode(cmode),
   _quantLen(qlen), _pianoWidth(pw)
      {
      // For uninitialized values, go with static default values instead
      if (w == ES_UNINIT)
            _width  = _widthInit;
      if (h == ES_UNINIT)
            _height = _heightInit;
      if (r == ES_UNINIT)
            _raster = _rasterInit;
      if (q == ES_UNINIT)
            _quant = _quantInit;
      }

//---------------------------------------------------------
//   clone
//---------------------------------------------------------

EditorSettings* PianorollSettings::clone()
      {
      PianorollSettings* newSettings = new PianorollSettings(_raster, _width,
         _height, _x, _y, _xmag, _ymag, _pos, _quant, _applyTo, _quantStrength, _quantLimit, _colorMode, _quantLen, _pianoWidth);
      CtrlEditSettings* c;
      for (int i=0; i<_numOfCtrlEdits; i++) {
            c = new CtrlEditSettings();
            *c = *(ctrlEdits[i]); //copy
            newSettings->setControlEditSettings(i, c);
            }
      newSettings->setControlEditSize(_numOfCtrlEdits);
      return (EditorSettings*) newSettings;
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void PianorollSettings::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "exteditorsettings")
                  ExtEditorSettings::readStatus(node.firstChild());
            else if (tag == "quantstrength")
                  _quantStrength = i;
            else if (tag == "quantlimit")
                  _quantLimit = i;
            else if (tag == "colormode")
                  _colorMode = i;
            else if (tag == "quantLen")
                  _quantLen = i;
            else if (tag == "pianowidth")
                  _pianoWidth = i;
            else
                  printf("MusE:PianorollSettings: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void PianorollSettings::writeStatus(Xml& xml) const
      {
      xml.tag("prollsettings");
      ExtEditorSettings::writeStatus(xml);
      xml.intTag("quantstrength", _quantStrength);
      xml.intTag("quantlimit", _quantLimit);
      xml.intTag("colormode", _colorMode);
      xml.intTag("quantLen", _quantLen);
      xml.intTag("pianowidth", _pianoWidth);
      xml.etag("prollsettings");
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void PianorollSettings::dump()
      {
//      printf("%x: PianorollSettings: q=%d r=%d w=%d h=%d x:%d y:%d xmag: %d pos:%d ypos:%d qs:%d ql:%d cm:%d at:%d ql:%d pw:%d\n", this, _quant, _raster, _width, _height, _x, _y, _xmag, _pos, _ypos, _quantStrength, _quantLimit, _colorMode, _applyTo, _quantLen, _pianoWidth);
      }


//---------------------------------------------------------
//   setStaticInitValues
//! set static initialization values
//---------------------------------------------------------
void PianorollSettings::setStaticInitValues(int widthinit, int heightinit, int rasterinit, int quantinit)
      {
      _widthInit = widthinit;
      _heightInit = heightinit;
      _rasterInit = rasterinit;
      _quantInit = quantinit;
      }

//---------------------------------------------------------
//   writeStatic
//! write static configuration values for pianoroll
//---------------------------------------------------------

void PianorollSettings::writeStatic(Xml& xml)
      {
      xml.tag("pianorollstatic");
      xml.intTag("quantDefault", _quantInit);
      xml.intTag("rasterDefault", _rasterInit);
      xml.intTag("widthDefault", _widthInit);
      xml.intTag("heightDefault", _heightInit);
      xml.etag("pianorollstatic");
     }

//---------------------------------------------------------
//   readStatic
//---------------------------------------------------------

void PianorollSettings::readStatic(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "quantDefault")
                  _quantInit = i;
            else if (tag == "rasterDefault")
                  _rasterInit = i;
            else if (tag == "widthDefault")
                  _widthInit = i;
            else if (tag == "heightDefault")
                  _heightInit = i;
            else
                  printf("MusE:PianorollSettings Static: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   SettingsList
//---------------------------------------------------------

SettingsList::SettingsList()
      {
      //_default = d;
      //dump();
      }

//---------------------------------------------------------
//   ~SettingsList
//---------------------------------------------------------

SettingsList::~SettingsList()
      {
      for(iSettingsList i = begin(); i!=end(); i++)
            delete(i->second);
      }

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void SettingsList::set(unsigned id, EditorSettings* e)
      {
      iSettingsList i = find(id);
      if (i == end()) {
            //Create a new item
            EditorSettings* newItem = e->clone();
            if (DBG_ESETTINGS)
                  printf("SettingsList::set - storing item with id=%d, "
                     "object at %p, cloned object at: %p.\n", id, e, newItem);
            insert(std::pair<unsigned, EditorSettings*>(id, newItem));
            }
      else {
            EditorSettings* newItem = e->clone();
            if (DBG_ESETTINGS)
                  printf("SettingsList::set - Cloning item with id=%d, object at %p to obj at %p\n", id, e, newItem);
                  printf("SettingsList::set  - deleting settings at: %p\n", i->second);
            delete(i->second); //Hmmm, hehhh... Perhaps better to use copy-constructor instead of deletion in different places, but then it's difficult to create object of correct subclass (in a neat way)... (ml)
            i->second = newItem;
            }
      }

//---------------------------------------------------------
//   get
//---------------------------------------------------------

EditorSettings* SettingsList::get(unsigned id)
      {
      iSettingsList i = find(id);

      if (i == end()) {
            if (DBG_ESETTINGS)
                  printf("SettingsList::get - no match for id=%d\n", id);
            return 0;
            }
      else
            return i->second;
      }

//---------------------------------------------------------
//   removeSettings
// remove settings for parts that don't exist anymore
//---------------------------------------------------------

void SettingsList::removeSettings(unsigned id)
      {
      for(iSettingsList i = begin(); i!=end(); i++) {
            int bc = i->first;
            bc&= ~(0xf0000000);
            if (unsigned(bc) == id) {
                  if (DBG_ESETTINGS)
                        printf("SettingsList::removeSettings, deleting obj at %p\n", i->second);
                  delete i->second;
                  erase(i);
                  }
            }
      }

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void SettingsList::dump()
      {
#if 0
      printf("-------SettingsList-DUMP------\n");
      for(iSettingsList i = begin(); i!=end(); i++) {
            //printf("i=%x ",i);
            printf("PART NO: %x ", i->first);
            printf("- i->second at %x\n", i->second);

            i->second->dump();
            }
      printf("---END-SettingsList-DUMP------\n");
#endif
      }

//---------------------------------------------------------
//   writeStatus
//! writes out the settings for all editors and parts
//---------------------------------------------------------

void SettingsList::writeStatus(Xml& xml) const
      {
      xml.tag("settingslist");
      // Write static
      ExtEditorSettings::writeStatic(xml);
      PianorollSettings::writeStatic(xml);
      DrumEditorSettings::writeStatic(xml);

      //Write all nodes
      for (ciSettingsList i = begin(); i != end(); i++) {
            xml.tag("elem");
            xml.intTag("id", i->first);
            i->second->writeStatus(xml);
            xml.etag("elem");
            }
      xml.etag("settingslist");
      }

//---------------------------------------------------------
//   readStatus
//! reads the settings for all editors and parts
//---------------------------------------------------------

void SettingsList::readStatus(QDomNode node)
      {
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            if (tag == "elem")
                  readElem(node.firstChild());
            else if (tag == "exteditorstatic")
                  ExtEditorSettings::readStatic(node.firstChild());
            else if (tag == "pianorollstatic")
                  PianorollSettings::readStatic(node.firstChild());
            else if (tag == "drumeditorstatic")
                  DrumEditorSettings::readStatic(node.firstChild());
            else
                  printf("MusE:SettingsList: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }

//---------------------------------------------------------
//   readElem
//---------------------------------------------------------

void SettingsList::readElem(QDomNode node)
      {
      int id = 0;
      while (!node.isNull()) {
            QDomElement e = node.toElement();
            QString tag = e.tagName();
            QString s = e.text();
            int i = s.toInt();
            if (tag == "grapheditorsettings") {
                  GraphEditorSettings* temp = new GraphEditorSettings();
                  temp->readStatus(node.firstChild());
                  insert(std::pair<unsigned, EditorSettings*>(id, temp));
                  }
            else if (tag == "editorsettings") {
                  EditorSettings* temp = new EditorSettings();
                  temp->readStatus(node.firstChild());
                  insert(std::pair<unsigned, EditorSettings*>(id, temp));
                  }
            else if (tag == "drumeditorsettings") {
                  DrumEditorSettings* temp = new DrumEditorSettings();
                  temp->readStatus(node.firstChild());
                  insert(std::pair<unsigned, EditorSettings*>(id, temp));
                  }
            else if (tag == "prollsettings") {
                  PianorollSettings* temp = new PianorollSettings();
                  temp->readStatus(node.firstChild());
                  insert(std::pair<unsigned, EditorSettings*>(id, temp));
                  }
            else if (tag == "id")
                  id = i;
            else
                  printf("MusE:SettingsList element: unknown tag %s\n", e.tagName().toAscii().data());
            node = node.nextSibling();
            }
      }


//---------------------------------------------------------
/*!
    \fn SettingsList::reset()
    \brief Remove all previous settings and clean up
 */
//---------------------------------------------------------
void SettingsList::reset()
      {
      for (iSettingsList i = begin(); i != end(); i++) {
            delete i->second;
            }
      clear();
      }
