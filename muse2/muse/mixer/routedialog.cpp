//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.cpp,v 1.5.2.2 2007/01/04 00:35:17 terminator356 Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2015 Tim E. Real (terminator356 on sourceforge)
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

#include <QCloseEvent>
//#include <QDialog>
// REMOVE Tim. Persistent routes. Added.
//#include <QListWidgetItem>
//#include <QTreeWidgetItem>
#include <QScrollBar>
#include <QVector>
#include <QList>
#include <QPainter>
#include <Qt>
//#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QRect>
#include <QPoint>
//#include <QBitArray>
#include <QModelIndex>
#include <QString>
#include <QHeaderView>
#include <QLayout>

#include "routedialog.h"
#include "globaldefs.h"
#include "track.h"
#include "song.h"
#include "audio.h"
#include "driver/jackaudio.h"
#include "globaldefs.h"
#include "app.h"
#include "operations.h"

// Undefine if and when multiple output routes are added to midi tracks.
#define _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

namespace MusEGui {

const QString RouteDialog::tracksCat(QObject::tr("Tracks:"));
const QString RouteDialog::midiPortsCat(QObject::tr("Midi ports:"));
const QString RouteDialog::midiDevicesCat(QObject::tr("Midi devices:"));
const QString RouteDialog::jackCat(QObject::tr("Jack:"));
const QString RouteDialog::jackMidiCat(QObject::tr("Jack midi:"));

// const QString RouteDialog::trackLabel(QObject::tr("Track"));
// const QString RouteDialog::midiDeviceLabel(QObject::tr("Midi port:device"));
// const QString RouteDialog::jackLabel(QObject::tr("Jack"));
// const QString RouteDialog::jackMidiLabel(QObject::tr("Jack midi"));

const int RouteDialog::channelDotDiameter = 9;
const int RouteDialog::channelDotSpacing = 1;
const int RouteDialog::channelDotsPerGroup = 4;
const int RouteDialog::channelDotGroupSpacing = 3;
const int RouteDialog::channelDotsMargin = 1;
const int RouteDialog::channelBarHeight = RouteDialog::channelDotDiameter + RouteDialog::channelDotsMargin;
const int RouteDialog::channelLinesSpacing = 2;

std::list<QString> tmpJackInPorts;
std::list<QString> tmpJackOutPorts;
std::list<QString> tmpJackMidiInPorts;
std::list<QString> tmpJackMidiOutPorts;


// //---------------------------------------------------------
// //   RouteChannelsList
// //---------------------------------------------------------
// 
// void RouteChannelsList::computeChannelYValues()
// {
//   
// }

//---------------------------------------------------------
//   RouteTreeWidgetItem
//---------------------------------------------------------

void RouteTreeWidgetItem::init()
{
  _curChannel = 0;

  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    case RouteItem:
    break;
    
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track)
          {
            if(_route.track->isMidiTrack())
            {
              _channels.resize(MIDI_CHANNELS);
              //_channelYValues.resize(MIDI_CHANNELS);
            }
            else
            {
              MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(_route.track);
              const int chans = atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH ? atrack->totalInChannels() : atrack->channels();
              if(chans != 0)
              {
                _channels.resize(chans);
                //_channelYValues.resize(chans);
              }
            }
          }
        break;  

        case MusECore::Route::JACK_ROUTE:  
        case MusECore::Route::MIDI_DEVICE_ROUTE:  
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;  
      }
    break;  
  }
  
  //computeChannelYValues();
}

void RouteTreeWidgetItem::getSelectedRoutes(MusECore::RouteList& routes)
{
  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    break;  
    case RouteItem:
      if(isSelected())
        routes.push_back(_route);
    break;  
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track)
          {
            MusECore::Route r(_route);
            const int sz = _channels.size();
            if(_route.track->isMidiTrack())
            {  
              for(int i = 0; i < sz && i < MIDI_CHANNELS; ++i)
              {
                //if(_channels.testBit(i))
                if(_channels.channelSelected(i))
                {
                  //r.channel = (1 << i);
                  r.channel = i;
                  routes.push_back(r);
                }
              }
            }
            else
            {
              for(int i = 0; i < sz; ++i)
              {
                //if(_channels.testBit(i))
                if(_channels.channelSelected(i))
                {
                  r.channel = i;
                  routes.push_back(r);
                }
              }
            }
          }
        break;
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
          if(isSelected())
            routes.push_back(_route);
        break;
      }
      
    break;  
  }
}

int RouteTreeWidgetItem::channelAt(const QPoint& pt, const QRect& rect) const
{
  const int col = treeWidget()->columnAt(pt.x());
  const int col_width = treeWidget()->columnWidth(col); 
  const int view_width = treeWidget()->viewport()->width();
  const int chans = _channels.size();
  const int view_offset = treeWidget()->header()->offset();
  //const int x_offset = (_isInput ? view_width - getSizeHint(col, col_width).width() + view_offset : -view_offset);
  const int x_offset = (_isInput ? view_width - getSizeHint(col, col_width).width() - view_offset : -view_offset);

  QPoint p(pt.x() - x_offset, pt.y() - rect.y());
  
  fprintf(stderr, "RouteTreeWidgetItem::channelAt() pt x:%d y:%d rect x:%d y:%d w:%d h:%d view_offset:%d x_offset:%d col w:%d header w:%d view w:%d p x:%d y:%d\n", 
          pt.x(), pt.y(), rect.x(), rect.y(), rect.width(), rect.height(), view_offset, x_offset, 
          treeWidget()->columnWidth(col), treeWidget()->header()->sectionSize(col), view_width, p.x(), p.y());  // REMOVE Tim.
  
  for(int i = 0; i < chans; ++i)
  {
    const RouteChannelsStruct& ch_struct = _channels.at(i);
    const QRect& ch_rect = ch_struct._buttonRect;
    if(ch_rect.contains(p))
      return i;
  }
  return -1;
}  
/*  
  const int channels = _channels.size();
  //const QRect rect = visualItemRect(item);
  QPoint p = pt - rect.topLeft();

  int w = RouteDialog::channelDotsMargin * 2 + RouteDialog::channelDotDiameter * channels;
  if(channels > 1)
    w += RouteDialog::channelDotSpacing * (channels - 1);
  if(channels > 4)
    w += RouteDialog::channelDotGroupSpacing * (channels - 1) / 4;
  
  const int xoff =_isInput ? rect.width() - w : RouteDialog::channelDotsMargin;
  const int yoff = RouteDialog::channelDotsMargin + (_isInput ? channels : 0);
  p.setY(p.y() - yoff);
  p.setX(p.x() - xoff);
  if(p.y() < 0 || p.y() >= RouteDialog::channelDotDiameter)
    return -1;
  for(int i = 0; i < channels; ++i)
  {
    if(p.x() < 0)
      return -1;
    if(p.x() < RouteDialog::channelDotDiameter)
      return i;
    p.setX(p.x() - RouteDialog::channelDotDiameter - RouteDialog::channelDotSpacing);
    if(i && ((i % 4) == 0))
      p.setX(p.x() - RouteDialog::channelDotGroupSpacing);
  }
  return -1;
}*/

int RouteTreeWidgetItem::connectedChannels() const
{
  int n = 0;
  //const int sz = _channelYValues.size();
  const int sz = _channels.size();
  for(int i = 0; i < sz; ++i)
    //if(_channelYValues.at(i) != -1)
    if(_channels.at(i)._connected)
      ++n;
  return n;
}

int RouteTreeWidgetItem::channelsPerWidth(int w) const
{
  if(type() == ChannelsItem) 
  {
    if(w == -1)
      w = treeWidget()->columnWidth(RouteDialog::ROUTE_NAME_COL);
    int groups_per_col = (w - 2 * RouteDialog::channelDotsMargin) / 
                         (RouteDialog::channelDotGroupSpacing + RouteDialog::channelDotsPerGroup * (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing));
    if(groups_per_col < 1)
      groups_per_col = 1;
    
    return RouteDialog::channelDotsPerGroup * groups_per_col;
  }
  return 0;
}

int RouteTreeWidgetItem::groupsPerChannels(int c) const
{
  
  int groups = c / RouteDialog::channelDotsPerGroup;
  //if(groups < 1)
  //  groups = 1;
  if(c % RouteDialog::channelDotsPerGroup)
    ++groups;
  return groups;
}

int RouteTreeWidgetItem::barsPerColChannels(int cc) const
{
  if(cc == 0)
    return 0;
  const int chans = _channels.size();
  int bars = chans / cc;
  if(chans % cc)
    ++bars;
  //if(chan_rows < 1)
  //  chan_rows = 1;
  return bars;
}


void RouteTreeWidgetItem::computeChannelYValues(int col_width)
{
  //_channelYValues.resize();
  if(type() != ChannelsItem)
    return;
  //_channelYValues.fill(-1);
  _channels.fillConnected(false);
  switch(_route.type)
  {
    case MusECore::Route::TRACK_ROUTE:
      if(_route.track)
      {
        //_channelYValues.fill(-1);
        //_channels.fillConnected(false);
        const MusECore::RouteList* rl = _isInput ? _route.track->outRoutes() : _route.track->inRoutes();
        for(MusECore::ciRoute ir = rl->begin(); ir != rl->end(); ++ir)
        {
          switch(ir->type)
          {
            case MusECore::Route::TRACK_ROUTE:
              //if(ir->track && ir->channel != -1)
              if(ir->channel != -1)
              {
                //if(ir->channel >= _channelYValues.size())
                //if(ir->channel >= _channels.size())
                //{
                  //fprintf(stderr, "RouteTreeWidgetItem::computeChannelYValues() Error: iRoute channel:%d out of channels range:%d\n", ir->channel, _channelYValues.size());
                //  fprintf(stderr, "RouteTreeWidgetItem::computeChannelYValues() Error: iRoute channel:%d out of channels range:%d\n", ir->channel, _channels.size());
                //  break; 
                //}
                // Mark the channel as used with a zero, for now.
                //_channelYValues.replace(ir->channel, 0);
                _channels.setConnected(ir->channel, true);
              }
            break;

            case MusECore::Route::MIDI_PORT_ROUTE:
              if(ir->isValid() && ir->channel != -1)
              {
//                 for(int i = 0; i < MIDI_CHANNELS; ++i)
//                 {
//                   if(ir->channel & (1 << i))
//                   {
//                     // Mark the channel as used with a zero, for now.
//                     //_channelYValues.replace(i, 0);
//                     _channels.setConnected(i, true);
//                   }
//                 }
                _channels.setConnected(ir->channel, true);
              }
            break;

            case MusECore::Route::JACK_ROUTE:
              if(ir->channel != -1)
                _channels.setConnected(ir->channel, true);
            break;

            case MusECore::Route::MIDI_DEVICE_ROUTE:
            break;
          }
        }
        
      }
    break;

    case MusECore::Route::JACK_ROUTE:
    case MusECore::Route::MIDI_DEVICE_ROUTE:
    case MusECore::Route::MIDI_PORT_ROUTE:
    break;
  }

  const int chans = _channels.size();
//   int w = RouteDialog::channelDotsMargin * 2 + RouteDialog::channelDotDiameter * chans;
//   //int w = RouteDialog::channelDotsMargin * 2 + (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing) * chans;
//   if(chans > 1)
//     w += RouteDialog::channelDotSpacing * (chans - 1);
//   if(chans > RouteDialog::channelDotsPerGroup)
//     w += RouteDialog::channelDotGroupSpacing * (chans - 1) / RouteDialog::channelDotsPerGroup;

  //const int col_width = treeWidget()->columnWidth(RouteDialog::ROUTE_NAME_COL);
  if(col_width == -1)
    col_width = treeWidget()->columnWidth(RouteDialog::ROUTE_NAME_COL);
  int chans_per_w = channelsPerWidth(col_width);
  // Limit to actual number of channels available.
  if(chans_per_w > chans)
    chans_per_w = chans;

  //fprintf(stderr, "RoutingItemDelegate::paint src list width:%d src viewport width:%d\n", router->newSrcList->width(), router->newSrcList->viewport()->width());  // REMOVE Tim.
  //int x = _isInput ? router->newSrcList->viewport()->width() - w : RouteDialog::midiDotsMargin;
  //int x = _isInput ? painter->device()->width() - w : RouteDialog::channelDotsMargin;
  //const int x_orig = _isInput ? treeWidget()->width() - w : RouteDialog::channelDotsMargin;
  const int x_orig = RouteDialog::channelDotsMargin;
  int x = x_orig;
  //int chan_y = RouteDialog::channelDotsMargin + (_isInput ? chans : 0);
  int chan_y = RouteDialog::channelDotsMargin;

  fprintf(stderr, "RouteTreeWidgetItem::computeChannelYValues() col_width:%d chans_per_w:%d\n", col_width, chans_per_w);  // REMOVE Tim.
  
  int line_y = RouteDialog::channelDotsMargin + (_isInput ? 0 : RouteDialog::channelBarHeight);

  //QList<int> chan_ys;
  
  //// If it's a source, take care of the first batch of lines first, which are above the first channel bar.
  //if(_isInput)
  //{
//     for(int i = 0; i < chans; ++i)
//     {
//       const bool new_group = i && (i % chans_per_w == 0);
//       // Is it marked as used?
//       if(_channels.at(i)._connected)
//       {
//         // Set the value to an appropriate y value useful for drawing channel lines.
//         _channels[i]._lineY = line_y;
//         if(new_group)
//           line_y += RouteDialog::channelBarHeight;
//         else
//           line_y += RouteDialog::channelLinesSpacing;
//       }
//     }
  //}

  
  int cur_chan = 0;
  for(int i = 0; i < chans; )
  //for(int i = 0; i < chans; ++i)
  {
    //const bool new_group = i && (i % RouteDialog::channelDotsPerGroup == 0);
    //const bool new_section = i && (i % chans_per_w == 0);
    const bool is_connected = _channels.at(i)._connected;
    
    //if(new_section)
    //{  
    //  chan_y = line_y + RouteDialog::channelDotsMargin;
    //}
    
    // Is it marked as used?
    //if(_channelYValues.at(i) != -1)
    //if(_channels.at(i)._connected)
    if(is_connected)
    {
      // Replace the zero value with an appropriate y value useful for drawing channel lines.
      //_channelYValues.replace(i, y);
      _channels[i]._lineY = line_y;
      //if(new_section)
      //  line_y += RouteDialog::channelBarHeight;
      //else
      //  line_y += RouteDialog::channelLinesSpacing;
    }
    
//     if(_isInput)
//     {
//       // If we finished a section set button rects, or we reached the end
//       //  set the remaining button rects, based on current line y (and x).
//       if(new_section || i + 1 == chans)
//       {
//        for( ; cur_chan < i; ++cur_chan)
//        {
//          _channels[cur_chan]._buttonRect = QRect(x, chan_y, RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter);
//        }
//       }
//       
//     }
//     else
//     {
//       _channels[i]._buttonRect = QRect(x, chan_y, RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter);
//       
//     }

    if(!_isInput)
      _channels[i]._buttonRect = QRect(x, chan_y, RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter);
    
    ++i;
    const bool new_group = (i % RouteDialog::channelDotsPerGroup == 0);
    const bool new_section = (i % chans_per_w == 0);

    if(is_connected)
      line_y += RouteDialog::channelLinesSpacing;
    
    if(_isInput)
    {
      // If we finished a section set button rects, or we reached the end
      //  set the remaining button rects, based on current line y (and x).
      if(new_section || i == chans)
      {
        x = x_orig;
        for( ; cur_chan < i; )
        {
          fprintf(stderr, "RouteTreeWidgetItem::computeChannelYValues() i:%d cur_chan:%d x:%d\n", i, cur_chan, x);  // REMOVE Tim.
          _channels[cur_chan]._buttonRect = QRect(x, line_y, RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter);
          ++cur_chan;
          x += RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing;
          if(cur_chan % RouteDialog::channelDotsPerGroup == 0)
            x += RouteDialog::channelDotGroupSpacing;
        }
        //++cur_chan;
      }
    }
//     else
//     {
//       if(new_section)
//         x = x_orig;  // Reset
//       else
//       {
//         x += RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing;
//         if(new_group)
//           x += RouteDialog::channelDotGroupSpacing;
//       }
//     }

    if(new_section)
    {
      x = x_orig;  // Reset
      chan_y = line_y;
      line_y += RouteDialog::channelBarHeight;
    }
    else
    {
      x += RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing;
      if(new_group)
        x += RouteDialog::channelDotGroupSpacing;
    }
  }
}

bool RouteTreeWidgetItem::mousePressHandler(QMouseEvent* e, const QRect& rect)
{
  const QPoint pt = e->pos(); 
  const Qt::KeyboardModifiers km = e->modifiers();
  bool ctl;
  switch(_itemMode)
  {
    case ExclusiveMode:
      ctl = false;
    break;
    case NormalMode:
      ctl = km & Qt::ControlModifier;
    break;
  }
  //bool shift = km & Qt::ShiftModifier;

//   RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(itemAt(pt));
//   bool is_cur = item && currentItem() && (item == currentItem());

  //if(is_cur)
  //  QTreeWidget::mousePressEvent(e);
  
  switch(type())
  {
    case NormalItem:
    case CategoryItem:
    case RouteItem:
    break;  
    case ChannelsItem:
      switch(_route.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(_route.track && _route.channel != -1) // && item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
          {
//             int chans;
//             if(_route.track->isMidiTrack())
//               chans = MIDI_CHANNELS;
//             else
//             {
//               MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(_route.track);
//               if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//                 chans = _isInput ? atrack->totalOutChannels() : atrack->totalInChannels();
//               else
//                 chans = atrack->channels();
//             }

            int ch = channelAt(pt, rect);
            
            //QBitArray ba = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
            //QBitArray ba_m = ba;
            //QBitArray ba_m = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
            //const int ba_sz = ba_m.size();
            const int ba_sz = _channels.size();
            bool changed = false;
            //if(!ctl)
            {
              //ba_m.fill(false);
              for(int i = 0; i < ba_sz; ++i)
              {
                //const bool b = ba_m.testBit(i);
                
                if(i == ch)
                {
                  if(ctl)
                  {
                    //_channels.toggleBit(i);
                    _channels[i].toggleSelected();
                    changed = true;
                  }
                  else
                  {
                    //if(!_channels.testBit(i))
                    if(!_channels.at(i)._selected)
                      changed = true;
                    //_channels.setBit(i);
                    _channels[i]._selected = true;
                  }
                }
                else if(!ctl)
                {
                  //if(_channels.testBit(i))
                  if(_channels. at(i)._selected)
                    changed = true;
                  //_channels.clearBit(i);
                  _channels[i]._selected = false;
                }
                  
  //               //if(ba_m.testBit(i))
  //               {
  //                 ba_m.clearBit(i);
  //                 changed = true;
  //               }
              }
            }
  //             //clearChannels();
  //           //  clearSelection();
  //           //int ch = channelAt(item, pt, chans);
  //           if(ch != -1 && ch < ba_sz)
  //           {
  //             ba_m.toggleBit(ch);
  //             changed = true;
  //           }

            //if(is_cur)
            //  QTreeWidget::mousePressEvent(e);
              
            //if(ba_m != ba)
//             if(changed)
//             {
//               item->setData(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole, qVariantFromValue<QBitArray>(ba_m));
//               //setCurrentItem(item);
//               update(visualItemRect(item));
//               //emit itemSelectionChanged();
//             }
            
//             //if(!is_cur)
//               QTreeWidget::mousePressEvent(e);

//             if(changed && is_cur)
//               //setCurrentItem(item);
//               emit itemSelectionChanged();
              
            //e->accept();
//             return;
            return changed;
          }
        break;
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;
      }
      
    break;  
  }

  return false;
  
//   QTreeWidget::mousePressEvent(e);
}

// bool RouteTreeWidgetItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
// {
//   //fprintf(stderr, "RoutingItemDelegate::paint\n");  // REMOVE Tim.
// //   RouteDialog* router = qobject_cast< RouteDialog* >(parent());
//   //if(parent() && qobject_cast< RouteDialog* >(parent()))
// //   if(router)
// //   {
//     //fprintf(stderr, "RoutingItemDelegate::paint parent is RouteDialog\n");  // REMOVE Tim.
//     //QWidget* qpd = qobject_cast<QWidget*>(painter->device());
//     //if(qpd)
//     if(painter->device())
//     {
//       //fprintf(stderr, "RoutingItemDelegate::paint device is QWidget\n");  // REMOVE Tim.
//       //RouteDialog* router = static_cast<RouteDialog*>(parent());
//       
// //       if(index.column() == RouteDialog::ROUTE_NAME_COL && index.data(RouteDialog::RouteRole).canConvert<MusECore::Route>()) 
//       if(type() == ChannelsItem && index.column() == RouteDialog::ROUTE_NAME_COL) 
//       {
//         //fprintf(stderr, "RoutingItemDelegate::paint data is Route\n");  // REMOVE Tim.
// //         MusECore::Route r = qvariant_cast<MusECore::Route>(index.data(RouteDialog::RouteRole));
//         QRect rect(option.rect);
// //         switch(r.type)
//         switch(_route.type)
//         {
//           case MusECore::Route::TRACK_ROUTE:
//             //fprintf(stderr, "RoutingItemDelegate::paint route is track\n");  // REMOVE Tim.
//             if(_route.track && _route.channel != -1)
//             {
//               const int chans = _channels.size(); 
// //               int chans; 
// //               if(_route.track->isMidiTrack())
// //               {
// //                 //fprintf(stderr, "RoutingItemDelegate::paint track is midi\n");  // REMOVE Tim.
// //                 chans = MIDI_CHANNELS;
// //               }
// //               else
// //               {
// //                 //fprintf(stderr, "RoutingItemDelegate::paint track is audio\n");  // REMOVE Tim.
// //                 MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(_route.track);
// //                 if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
// //                 {
// //                   if(_isInput)
// //                     chans = atrack->totalOutChannels();
// //                   else
// //                     chans = atrack->totalInChannels();
// //                 }
// //                 else
// //                   chans = atrack->channels();
// //               }
//               
//               int w = RouteDialog::channelDotsMargin * 2 + RouteDialog::channelDotDiameter * chans;
//               if(chans > 1)
//                 w += RouteDialog::channelDotSpacing * (chans - 1);
//               if(chans > 4)
//                 w += RouteDialog::channelDotGroupSpacing * (chans - 1) / 4;
//               
//               //fprintf(stderr, "RoutingItemDelegate::paint src list width:%d src viewport width:%d\n", router->newSrcList->width(), router->newSrcList->viewport()->width());  // REMOVE Tim.
//               //int x = _isInput ? router->newSrcList->viewport()->width() - w : RouteDialog::midiDotsMargin;
//               int x = _isInput ? painter->device()->width() - w : RouteDialog::channelDotsMargin;
//               const int y = RouteDialog::channelDotsMargin + (_isInput ? chans : 0);
//               
// //               QBitArray ba;
// //               int basize = 0;
// //               if(index.data(RouteDialog::ChannelsRole).canConvert<QBitArray>())
// //               {
// //                 ba = index.data(RouteDialog::ChannelsRole).value<QBitArray>();
// //                 basize = ba.size();
// //               }
//               
//               //const int y_sz = _channelYValues.size();
//               //const int y_sz = _channels.size();
//               const int connected_chans = connectedChannels();
//               int cur_chan_line = 0;
//               for(int i = 0; i < chans; )
//               {
//                 painter->setPen(Qt::black);
//                 //painter->drawRoundedRect(option.rect.x() + x, option.rect.y() + y, 
// //                 if(!ba.isNull() && i < basize && ba.testBit(i))
//                 //if(!_channels.isNull() && _channels.testBit(i))
//                 if(_channels.at(i)._selected)
//                   painter->fillRect(x, option.rect.y() + y, 
//                                            RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter,
//                                            option.palette.highlight());
//                 //else
//                   painter->drawRoundedRect(x, option.rect.y() + y, 
//                                            RouteDialog::channelDotDiameter, RouteDialog::channelDotDiameter,
//                                            30, 30);
//                 if((i % 2) == 0)
//                   painter->setPen(Qt::darkGray);
//                 else
//                   painter->setPen(Qt::black);
//                 const int xline = x + RouteDialog::channelDotDiameter / 2;
//                 //if(i < y_sz)
//                 if(_channels.at(i)._connected)
//                 {
//                   //const int chan_y = _channelYValues.at(i);
//                   const int chan_y = _channels.at(i)._lineY;
//                   // -1 means not connected.
//                   //if(chan_y != -1)
//                   //{
//                     if(_isInput)
//                     {
//                       //const int yline = option.rect.y() + y;
//                       //painter->drawLine(xline, yline, xline, yline - chans + i);
//                       //painter->drawLine(xline, yline - chans + i, painter->device()->width(), yline - chans + i);
//                       const int yline = option.rect.y() + chan_y;
//                       painter->drawLine(xline, yline, xline, yline - connected_chans + cur_chan_line);
//                       painter->drawLine(xline, yline - connected_chans + cur_chan_line, painter->device()->width(), yline - connected_chans + cur_chan_line);
//                     }
//                     else
//                     {
//                       //const int yline = option.rect.y() + RouteDialog::midiDotsMargin + RouteDialog::midiDotDiameter;
//                       //painter->drawLine(xline, yline, xline, yline + i);
//                       //painter->drawLine(0, yline + i, xline, yline + i);
//                       const int yline = option.rect.y() + RouteDialog::channelDotsMargin + RouteDialog::channelDotDiameter;
//                       painter->drawLine(xline, yline, xline, yline + chan_y);
//                       painter->drawLine(0, yline + chan_y, xline, yline + chan_y);
//                     }
//                     ++cur_chan_line;
//                   //}
//                 }
//                 
//                 ++i;
//                 x += RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing;
//                 if(i && ((i % 4) == 0))
//                   x += RouteDialog::channelDotGroupSpacing;
//               }
//               return true;
//             }
//           break;  
//           case MusECore::Route::MIDI_DEVICE_ROUTE:
//           case MusECore::Route::MIDI_PORT_ROUTE:
//           case MusECore::Route::JACK_ROUTE:
//           break;  
//         }
//       }
//     }
// //   }
// //   QStyledItemDelegate::paint(painter, option, index);
//   return false;
// }

bool RouteTreeWidgetItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if(treeWidget()->viewport())
  {
    if(index.column() == RouteDialog::ROUTE_NAME_COL) 
    {
      const QRect rect(option.rect);
      const int col_width = treeWidget()->columnWidth(index.column()); 
      const int view_width = treeWidget()->viewport()->width();
      const int chans = _channels.size();
      const int view_offset = treeWidget()->header()->offset();
      //const int x_offset = (_isInput ? view_width - getSizeHint(index.column(), col_width).width() + view_offset : -view_offset);
      const int x_offset = (_isInput ? view_width - getSizeHint(index.column(), col_width).width() - view_offset : -view_offset);

      fprintf(stderr, "RouteTreeWidgetItem::paint() rect x:%d y:%d w:%d h:%d view_offset:%d x_offset:%d dev w:%d col w:%d header w:%d view w:%d\n", 
              rect.x(), rect.y(), rect.width(), rect.height(), view_offset, x_offset, painter->device()->width(), 
              treeWidget()->columnWidth(index.column()), treeWidget()->header()->sectionSize(index.column()), view_width);  // REMOVE Tim.
      
      switch(type())
      {
        case ChannelsItem:
        {
          int cur_chan = 0;
          for(int i = 0; i < chans; ++i)
          {
            const RouteChannelsStruct& ch_struct = _channels.at(i);
            const QRect& ch_rect = ch_struct._buttonRect;
            painter->setPen(Qt::black);
            if(ch_struct._selected)
              painter->fillRect(x_offset + ch_rect.x(), option.rect.y() + ch_rect.y(), 
                                ch_rect.width(), ch_rect.height(),
                                option.palette.highlight());
            painter->drawRoundedRect(x_offset + ch_rect.x(), option.rect.y() + ch_rect.y(), 
                                      ch_rect.width(), ch_rect.height(),
                                      30, 30);
            if((cur_chan % 2) == 0)
              painter->setPen(Qt::darkGray);
            else
              painter->setPen(Qt::black);

            if(ch_struct._connected)
            {
              const int line_x = x_offset + ch_rect.x() + RouteDialog::channelDotDiameter / 2;
              const int line_y = option.rect.y() + ch_struct._lineY;
              if(_isInput)
              {
                const int ch_y = option.rect.y() + ch_rect.y() -1;
                fprintf(stderr, "RouteTreeWidgetItem::paint() input: line_x:%d ch_y:%d line_y:%d view_w:%d\n", line_x, ch_y, line_y, view_width);  // REMOVE Tim.

                painter->drawLine(line_x, ch_y, line_x, line_y);
                painter->drawLine(line_x, line_y, view_width, line_y);
              }
              else
              {
                //const int ch_y = option.rect.y() + RouteDialog::channelBarHeight + ch_rect.y();
                const int ch_y = option.rect.y() + ch_rect.y() + ch_rect.height();
                painter->drawLine(line_x, ch_y, line_x, line_y);
                painter->drawLine(x_offset, line_y, line_x, line_y);
              }
              ++cur_chan;
            }
          }
          return true;
        }
        break;
        
        case NormalItem:
        case CategoryItem:
        case RouteItem:          
        break;
      }
    }
  }
  return false;
}

QSize RouteTreeWidgetItem::getSizeHint(int col, int col_width) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//         return starRating.sizeHint();
//     } else

//   if(index.column() == ControlMapperDialog::C_COLOR)
//     return QSize(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__,
//                  __COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__));
//     
  //return QStyledItemDelegate::sizeHint(option, index);

//   fprintf(stderr, "RouteTreeWidgetItem::getSizeHint col:%d column width:%d\n",
//           col, treeWidget()->columnWidth(RouteDialog::ROUTE_NAME_COL));  // REMOVE Tim.
  
  //if(index.column() == RouteDialog::ROUTE_NAME_COL && index.data(RouteDialog::RouteRole).canConvert<MusECore::Route>()) 
  if(type() == ChannelsItem && col == RouteDialog::ROUTE_NAME_COL) 
  {
    if(col_width == -1)
      col_width = treeWidget()->columnWidth(col);

//     int groups_per_col = (col_width - 2 * RouteDialog::channelDotsMargin) / 
//                             (RouteDialog::channelDotGroupSpacing + RouteDialog::channelDotsPerGroup * (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing));
//     if(groups_per_col < 1)
//       groups_per_col = 1;
    
    const int chans = _channels.size();
    
    //const int chans_per_col = RouteDialog::channelDotsPerGroup * groups_per_col;
    int chans_per_col = channelsPerWidth(col_width);
    
    // Limit to actual number of channels available.
    if(chans_per_col > chans)
      chans_per_col = chans;
    
    const int groups_per_col = groupsPerChannels(chans_per_col);
    
    //const int chans = connectedChannels();

//     int chan_rows = chans / chans_per_col;
//     if(chans % chans_per_col)
//       ++chan_rows;
//     if(chan_rows < 1)
//       chan_rows = 1;
//     //int chan_rows = 1 + chans / chans_per_col;
    
    const int bars = barsPerColChannels(chans_per_col);
    const int h = bars * RouteDialog::channelBarHeight + RouteDialog::channelDotsMargin + connectedChannels() * RouteDialog::channelLinesSpacing;
    const int w = chans_per_col * (RouteDialog::channelDotDiameter + RouteDialog::channelDotSpacing) +
                  groups_per_col * RouteDialog::channelDotGroupSpacing +
                  2 * RouteDialog::channelDotsMargin;
    return QSize(w, h);
    
//     switch(_route.type)
//     {
//       case MusECore::Route::TRACK_ROUTE:
//         if(_route.track && _route.channel != -1)
//         {
//           //int chans; 
//           const int chans = _channels.size(); 
// //           if(_route.track->isMidiTrack())
// //             chans = MIDI_CHANNELS;
// //           else
// //           {
// //             MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(_route.track);
// //             if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
// //             {
// //               if(_isInput)
// //                 chans = atrack->totalOutChannels();
// //               else
// //                 chans = atrack->totalInChannels();
// //             }
// //             else
// //               chans = atrack->channels();
// //           }
//           
// //           int w = RouteDialog::midiDotsMargin * 2 + RouteDialog::midiDotDiameter * chans;
// //           if(chans > 1)
// //             w += RouteDialog::midiDotSpacing * (chans - 1);
// //           if(chans > RouteDialog::midiDotsPerGroup)
// //             w += RouteDialog::midiDotGroupSpacing * (chans - 1) / RouteDialog::midiDotsPerGroup;
//           int w = col_width;
//           const int h = RouteDialog::midiDotDiameter + RouteDialog::midiDotsMargin * 2 + chans;
//           return QSize(w, h);
//         }
//       break;  
//       case MusECore::Route::MIDI_DEVICE_ROUTE:
//       case MusECore::Route::MIDI_PORT_ROUTE:
//       case MusECore::Route::JACK_ROUTE:
//       break;  
//     }
  }
  
  //return QStyledItemDelegate::sizeHint(option, index);
  return QSize();
  //return sizeHint(col);
}

QSize RouteTreeWidgetItem::getSizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//         return starRating.sizeHint();
//     } else

//   if(index.column() == ControlMapperDialog::C_COLOR)
//     return QSize(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__,
//                  __COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__));
//     
  //return QStyledItemDelegate::sizeHint(option, index);

//   const QSize sz = getSizeHint(index.column());
//   fprintf(stderr, "RouteTreeWidgetItem::sizeHint opt rect x:%d y:%d w:%d h:%d  hint w:%d h:%d column width:%d\n",
//           option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height(), 
//           sz.width(), sz.height(), 
//           treeWidget()->columnWidth(RouteDialog::ROUTE_NAME_COL));  // REMOVE Tim.
//   return sz;
  return getSizeHint(index.column());
}
 
//   //if(index.column() == RouteDialog::ROUTE_NAME_COL && index.data(RouteDialog::RouteRole).canConvert<MusECore::Route>()) 
//   if(type() == ChannelsItem && index.column() == RouteDialog::ROUTE_NAME_COL) 
//   {
//     switch(_route.type)
//     {
//       case MusECore::Route::TRACK_ROUTE:
//         if(_route.track && _route.channel != -1)
//         {
//           //int chans; 
//           const int chans = _channels.size(); 
// //           if(_route.track->isMidiTrack())
// //             chans = MIDI_CHANNELS;
// //           else
// //           {
// //             MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(_route.track);
// //             if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
// //             {
// //               if(_isInput)
// //                 chans = atrack->totalOutChannels();
// //               else
// //                 chans = atrack->totalInChannels();
// //             }
// //             else
// //               chans = atrack->channels();
// //           }
//           int w = RouteDialog::midiDotsMargin * 2 + RouteDialog::midiDotDiameter * chans;
//           if(chans > 1)
//             w += RouteDialog::midiDotSpacing * (chans - 1);
//           if(chans > 4)
//             w += RouteDialog::midiDotGroupSpacing * (chans - 1) / 4;
//           const int h = RouteDialog::midiDotDiameter + RouteDialog::midiDotsMargin * 2 + chans;
//           return QSize(w, h);
//         }
//       break;  
//       case MusECore::Route::MIDI_DEVICE_ROUTE:
//       case MusECore::Route::MIDI_PORT_ROUTE:
//       case MusECore::Route::JACK_ROUTE:
//       break;  
//     }
//   }
//   //return QStyledItemDelegate::sizeHint(option, index);
//   //return QSize();
//   return sizeHint(index.column());
// }

// void RouteTreeWidgetItem::columnSizeChanged(int logicalIndex, int oldSize, int newSize)
// {
//   fprintf(stderr, "RouteTreeWidgetItem::columnSizeChanged idx:%d old sz:%d new sz:%d\n", logicalIndex, oldSize, newSize);
//   if(type() == ChannelsItem && logicalIndex == RouteDialog::ROUTE_NAME_COL)
//   {
//     //setSizeHint(logicalIndex, getSizeHint(logicalIndex));
//   }
// }

bool RouteTreeWidgetItem::testForRelayout(int col, int old_width, int new_width) const
{
  if(type() == ChannelsItem && col == RouteDialog::ROUTE_NAME_COL)
  {
    const QSize old_sz = getSizeHint(col, old_width);
    const QSize new_sz = getSizeHint(col, new_width);
    //return old_sz.isValid() && new_sz.isValid() && old_sz.height() != new_sz.height();
    //return old_sz.isValid() && new_sz.isValid() && old_sz != new_sz;
    return old_sz != new_sz;
  }
  return false;
}

bool RouteTreeWidgetItem::routeNodeExists()
{
  switch(type())
  {
    case CategoryItem:
    case NormalItem:
      return true;
    break;
    
    case RouteItem:
    case ChannelsItem:
      return _route.exists();
    break;
  }
  return false;
}


//-----------------------------------
//   ConnectionsView
//-----------------------------------

ConnectionsView::ConnectionsView(QWidget* parent, RouteDialog* d)
        : QFrame(parent), _routeDialog(d)
{
  lastY = 0;
  setMinimumWidth(20);
  //setMaximumWidth(120);
  setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
}

ConnectionsView::~ConnectionsView()
{
}

int ConnectionsView::itemY(RouteTreeWidgetItem* item, bool /*is_input*/, int channel) const
{
//   if(item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//   {        
//     const MusECore::Route r = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//   }
  
  
  QRect rect;
  QTreeWidget* tree = item->treeWidget();
  //QTreeWidgetItem* parent = item->parent();

  QTreeWidgetItem* top_closed = 0;
  QTreeWidgetItem* parent = item;
  while(parent)
  {
    parent = parent->parent();
    if(!parent)
      break;
    if(!parent->isExpanded())
      top_closed = parent;
  }
  
  const int line_width = _routeDialog->newSrcList->lineWidth();
  
  //if(parent && !parent->isExpanded()) 
  if(top_closed) 
  {
    rect = tree->visualItemRect(top_closed);
    return line_width + rect.top() + rect.height() / 2;
  } 
//   else 
//   {
    rect = tree->visualItemRect(item);
    if(channel != -1)
      return line_width + rect.top() + item->channelYValue(channel);
    return line_width + rect.top() + rect.height() / 2;
//   }
  //fprintf(stderr, "ConnectionsView::itemY: left:%d top:%d right:%d bottom:%d\n", rect.left(), rect.top(), rect.right(), rect.bottom());
//   if(channel != -1)
//     //return rect.top() + RouteDialog::channelDotsMargin + (is_input ? 0 : RouteDialog::channelDotDiameter) + channel;
//     return rect.top() + item->channelYValue(channel);
//   
//   return rect.top() + rect.height() / 2;
}


void ConnectionsView::drawConnectionLine(QPainter* pPainter,
        int x1, int y1, int x2, int y2, int h1, int h2 )
{
  //fprintf(stderr, "ConnectionsView::drawConnectionLine: x1:%d y1:%d x2:%d y2:%d h1:%d h2:%d\n", x1, y1, x2, y2, h1, h2);
  
  // Account for list view headers.
  y1 += h1;
  y2 += h2;

  // Invisible output ports don't get a connecting dot.
  if(y1 > h1)
    pPainter->drawLine(x1, y1, x1 + 4, y1);

  // How do we'll draw it? // TODO
  if(1) 
  {
    // Setup control points
    QPolygon spline(4);
    const int cp = int(float(x2 - x1 - 8) * 0.4f);
    spline.putPoints(0, 4,
            x1 + 4, y1, x1 + 4 + cp, y1, 
            x2 - 4 - cp, y2, x2 - 4, y2);
    // The connection line, it self.
    QPainterPath path;
    path.moveTo(spline.at(0));
    path.cubicTo(spline.at(1), spline.at(2), spline.at(3));
    pPainter->strokePath(path, pPainter->pen());
  }
  else 
    pPainter->drawLine(x1 + 4, y1, x2 - 4, y2);

  // Invisible input ports don't get a connecting dot.
  if(y2 > h2)
    pPainter->drawLine(x2 - 4, y2, x2, y2);
}

void ConnectionsView::drawItem(QPainter* painter, QTreeWidgetItem* routesItem, const QColor& col)
{
  const int yc = QWidget::pos().y();
  const int yo = _routeDialog->newSrcList->pos().y();
  const int yi = _routeDialog->newDstList->pos().y();
  const int x1 = 0;
  const int x2 = QWidget::width();
  const int h1 = (_routeDialog->newSrcList->header())->sizeHint().height();
  const int h2 = (_routeDialog->newDstList->header())->sizeHint().height();
  int y1;
  int y2;
  QPen pen;
  const int pen_wid_norm = 0;
  const int pen_wid_wide = 3;
  
  if(routesItem->data(RouteDialog::ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && 
     routesItem->data(RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
  {        
    const MusECore::Route src = routesItem->data(RouteDialog::ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
    const MusECore::Route dst = routesItem->data(RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
    RouteTreeWidgetItem* srcItem = _routeDialog->newSrcList->findItem(src);
    if(srcItem)
    {
      RouteTreeWidgetItem* dstItem = _routeDialog->newDstList->findItem(dst);
      if(dstItem)
      {
        int src_chan = src.channel;
        int dst_chan = dst.channel;
        bool src_wid = false;
        bool dst_wid = false;
        switch(src.type)
        {
          case MusECore::Route::TRACK_ROUTE:
//             if(src_chan != -1 && src.track && src.track->isMidiTrack())
//             {
//               for(int i = 0; i < MIDI_CHANNELS; ++i)
//                 if(src_chan & (1 << i))
//                 {
//                   src_chan = i;
//                   break;
//                 }
//             }  // Fall through
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          case MusECore::Route::MIDI_PORT_ROUTE:
            // Support port/device items (no channel bar) to track channel item routes:
            // Source channel will be -1 but destination channel will be valid. Copy destination channel to source.
            //src_chan = dst_chan;
            if(src_chan == -1 && src.channels == -1) 
              src_wid = true;
          break;
          case MusECore::Route::JACK_ROUTE:
          break;
        }
        switch(dst.type)
        {
          case MusECore::Route::TRACK_ROUTE:
//             if(dst_chan != -1 && dst.track && dst.track->isMidiTrack())
//             {
//               for(int i = 0; i < MIDI_CHANNELS; ++i)
//                 if(dst_chan & (1 << i))
//                 {
//                   dst_chan = i;
//                   break;
//                 }
//             } // Fall through
          case MusECore::Route::MIDI_DEVICE_ROUTE:
          case MusECore::Route::MIDI_PORT_ROUTE:
            // Support track channel items to port/device items (no channel bar) routes:
            // Destination channel will be -1 but source channel will be valid. Copy source channel to destination.
            //dst_chan = src_chan;
            if(dst_chan == -1 && dst.channels == -1) 
              dst_wid = true;
          break;
          case MusECore::Route::JACK_ROUTE:
          break;
        }

        if(src_wid && dst_wid) 
          pen.setWidth(pen_wid_wide);
        else
          pen.setWidth(pen_wid_norm);
        
        pen.setColor(col);
        painter->setPen(pen);
        y1 = itemY(srcItem, true, src_chan) + (yo - yc);
        y2 = itemY(dstItem, false, dst_chan) + (yi - yc);
        drawConnectionLine(painter, x1, y1, x2, y2, h1, h2);
      }
      else
      {
        fprintf(stderr, "ConnectionsView::drawItem: dstItem not found:\n");
        src.dump();
        dst.dump();
      }
    }
    else
    {
      fprintf(stderr, "ConnectionsView::drawItem: srcItem not found:\n");
      src.dump();
      dst.dump();
    }
  }
}

// Draw visible port connection relation arrows.
void ConnectionsView::paintEvent(QPaintEvent*)
{
  //fprintf(stderr, "ConnectionsView::paintEvent: _routeDialog:%p\n", _routeDialog);
  if(!_routeDialog)
    return;

//   const int yc = QWidget::pos().y();
//   const int yo = _routeDialog->newSrcList->pos().y();
//   const int yi = _routeDialog->newDstList->pos().y();

  QPainter painter(this);
//   int y1;
//   int y2;
  int i, rgb[3] = { 0x33, 0x66, 0x99 };
  //int i, rgb[3] = { 0x00, 0x2c, 0x7f };

  // Inline adaptive to darker background themes...
  if(QWidget::palette().window().color().value() < 0x7f)
    for (i = 0; i < 3; ++i) 
      //rgb[i] += 0x33;
      rgb[i] += 0x66;

  i = 0;
//   const int x1 = 0;
//   const int x2 = QWidget::width();
//   const int h1 = (_routeDialog->newSrcList->header())->sizeHint().height();
//   const int h2 = (_routeDialog->newDstList->header())->sizeHint().height();
//   QPen pen;
//   const int pen_wid_norm = 0;
//   const int pen_wid_wide = 3;
//   
//   QTreeWidgetItem* src_sel = 0;
//   QTreeWidgetItem* dst_sel = 0;
//   int src_sel_ch = -1;
//   int dst_sel_ch = -1;
//   bool sel_wid = false;
  
//   QTreeWidgetItem* cur_item = _routeDialog->routeList->currentItem();
  const int iItemCount = _routeDialog->routeList->topLevelItemCount();
  // Draw unselected items behind selected items.
  for(int iItem = 0; iItem < iItemCount; ++iItem, ++i) 
  {
    QTreeWidgetItem* item = _routeDialog->routeList->topLevelItem(iItem);
    //++i;
    //if(!item)
    if(!item || item->isHidden() || item->isSelected())
      continue;
    drawItem(&painter, item, QColor(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3], 128));
  } 
  // Draw selected items on top of unselected items.
  for(int iItem = 0; iItem < iItemCount; ++iItem) 
  {
    QTreeWidgetItem* item = _routeDialog->routeList->topLevelItem(iItem);
    //if(!item)
    if(!item || item->isHidden() || !item->isSelected())
      continue;
    drawItem(&painter, item, Qt::yellow);
  } 
    
    
//     const QColor col(rgb[i % 3], rgb[(i / 3) % 3], rgb[(i / 9) % 3]);
//     if(item->data(RouteDialog::ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && item->data(RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//     {        
//       const MusECore::Route src = item->data(RouteDialog::ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//       const MusECore::Route dst = item->data(RouteDialog::ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//       QTreeWidgetItem* srcItem = _routeDialog->newSrcList->findItem(src);
//       if(srcItem)
//       {
//         QTreeWidgetItem* dstItem = _routeDialog->newDstList->findItem(dst);
//         if(dstItem)
//         {
//           int src_chan = src.channel;
//           int dst_chan = dst.channel;
//           bool src_wid = false;
//           bool dst_wid = false;
//           switch(src.type)
//           {
//             case MusECore::Route::TRACK_ROUTE:
//               if(src_chan != -1 && src.track && src.track->isMidiTrack())
//               {
//                 for(int i = 0; i < MIDI_CHANNELS; ++i)
//                   if(src_chan & (1 << i))
//                   {
//                     src_chan = i;
//                     break;
//                   }
//               }  // Fall through
//             case MusECore::Route::MIDI_DEVICE_ROUTE:
//             case MusECore::Route::MIDI_PORT_ROUTE:
//               if(src_chan == -1 && src.channels == -1) 
//                 src_wid = true;
//             break;
//             case MusECore::Route::JACK_ROUTE:
//             break;
//           }
//           switch(dst.type)
//           {
//             case MusECore::Route::TRACK_ROUTE:
//               if(dst_chan != -1 && dst.track && dst.track->isMidiTrack())
//               {
//                 for(int i = 0; i < MIDI_CHANNELS; ++i)
//                   if(dst_chan & (1 << i))
//                   {
//                     dst_chan = i;
//                     break;
//                   }
//               } // Fall through
//             case MusECore::Route::MIDI_DEVICE_ROUTE:
//             case MusECore::Route::MIDI_PORT_ROUTE:
//               if(dst_chan == -1 && dst.channels == -1) 
//                 dst_wid = true;
//             break;
//             case MusECore::Route::JACK_ROUTE:
//             break;
//           }
// 
//           if(item == cur_item)
//           {
//             // Remember the selected items and draw that line last over top all else.
//             src_sel = srcItem;
//             dst_sel = dstItem;
//             src_sel_ch = src_chan;
//             dst_sel_ch = dst_chan;
//             sel_wid = src_wid && dst_wid;
//             continue;
//           }
// 
//           if(src_wid && dst_wid) 
//             pen.setWidth(pen_wid_wide);
//           else
//             pen.setWidth(pen_wid_norm);
//           
//           pen.setColor(col);
//           painter.setPen(pen);
//           y1 = itemY(srcItem, true, src_chan) + (yo - yc);
//           y2 = itemY(dstItem, false, dst_chan) + (yi - yc);
//           drawConnectionLine(&painter, x1, y1, x2, y2, h1, h2);
//         }
//         else
//         {
//           fprintf(stderr, "ConnectionsView::paintEvent: dstItem not found:\n");
//           src.dump();
//           dst.dump();
//         }
//       }
//       else
//       {
//         fprintf(stderr, "ConnectionsView::paintEvent: srcItem not found:\n");
//         src.dump();
//         dst.dump();
//       }
//     }
//   }

//   // Draw the selected items over top all else.
//   if(src_sel && dst_sel)
//   {
//     if(sel_wid) 
//       pen.setWidth(pen_wid_wide);
//     else
//       pen.setWidth(pen_wid_norm);
//     pen.setColor(Qt::yellow);
//     painter.setPen(pen);
//     y1 = itemY(src_sel, true, src_sel_ch) + (yo - yc);
//     y2 = itemY(dst_sel, false, dst_sel_ch) + (yi - yc);
//     drawConnectionLine(&painter, x1, y1, x2, y2, h1, h2);
//   }
}

void ConnectionsView::mousePressEvent(QMouseEvent* e)
{
  e->setAccepted(true);
  lastY = e->y();
}

void ConnectionsView::mouseMoveEvent(QMouseEvent* e)
{
  e->setAccepted(true);
  const Qt::MouseButtons mb = e->buttons();
  const int y = e->y();
  const int ly = lastY;
  lastY = y;
  if(mb & Qt::LeftButton)
     emit scrollBy(0, ly - y);
}

void ConnectionsView::wheelEvent(QWheelEvent* e)
{
  int delta = e->delta();
  fprintf(stderr, "ConnectionsView::wheelEvent: delta:%d\n", delta); // REMOVE Tim.
  e->setAccepted(true);
  emit scrollBy(0, delta < 0 ? 1 : -1);
}

// Context menu request event handler.
void ConnectionsView::contextMenuEvent(QContextMenuEvent* /*pContextMenuEvent*/)
{
//         qjackctlConnect *pConnect = m_pConnectView->binding();
//         if (pConnect == 0)
//                 return;
// 
//         QMenu menu(this);
//         QAction *pAction;
// 
//         pAction = menu.addAction(QIcon(":/images/connect1.png"),
//                 tr("&Connect"), pConnect, SLOT(connectSelected()),
//                 tr("Alt+C", "Connect"));
//         pAction->setEnabled(pConnect->canConnectSelected());
//         pAction = menu.addAction(QIcon(":/images/disconnect1.png"),
//                 tr("&Disconnect"), pConnect, SLOT(disconnectSelected()),
//                 tr("Alt+D", "Disconnect"));
//         pAction->setEnabled(pConnect->canDisconnectSelected());
//         pAction = menu.addAction(QIcon(":/images/disconnectall1.png"),
//                 tr("Disconnect &All"), pConnect, SLOT(disconnectAll()),
//                 tr("Alt+A", "Disconect All"));
//         pAction->setEnabled(pConnect->canDisconnectAll());
// 
//         menu.addSeparator();
//         pAction = menu.addAction(QIcon(":/images/refresh1.png"),
//                 tr("&Refresh"), pConnect, SLOT(refresh()),
//                 tr("Alt+R", "Refresh"));
// 
//         menu.exec(pContextMenuEvent->globalPos());
}


// // Widget event slots...
// void ConnectionsView::contentsChanged (void)
// {
//         QWidget::update();
// }


//-----------------------------------
//   RouteTreeWidget
//-----------------------------------

RouteTreeWidget::RouteTreeWidget(QWidget* parent, bool is_input) : QTreeWidget(parent), _isInput(is_input)
{
  if(header())
    connect(header(), SIGNAL(sectionResized(int,int,int)), SLOT(headerSectionResized(int,int,int))); 

  connect(verticalScrollBar(), SIGNAL(rangeChanged(int,int)), SLOT(scrollRangeChanged(int,int))); 
  connect(verticalScrollBar(), SIGNAL(sliderMoved(int)), SLOT(scrollSliderMoved(int))); 
  connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(scrollValueChanged(int))); 
}

RouteTreeWidget::~RouteTreeWidget()
{
}

void RouteTreeWidget::headerSectionResized(int logicalIndex, int oldSize, int newSize)
{
//   fprintf(stderr, "RouteTreeWidget::headerSectionResized idx:%d old sz:%d new sz:%d\n", logicalIndex, oldSize, newSize);
//   scheduleDelayedItemsLayout();
  
  // Self adjust certain item heights...
  // NOTE: Delegate sizeHints are NOT called automatically. scheduleDelayedItemsLayout() seems to solve it. 
  //       But that is costly here! And results in some flickering especially at scrollbar on/off conditions as it fights with itself. 
  //       So check if we really need to do it...
  QTreeWidgetItemIterator ii(this);
  bool do_layout = false;
  while(*ii)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*ii);
    //item->columnSizeChanged(logicalIndex, oldSize, newSize);
    if(item->testForRelayout(logicalIndex, oldSize, newSize))
    {
      do_layout = true;
      item->computeChannelYValues(newSize);
      //scheduleDelayedItemsLayout();
      //return;
    }
    ++ii;
  }
  if(do_layout)
  {
    fprintf(stderr, "RouteTreeWidget::headerSectionResized idx:%d old sz:%d new sz:%d calling scheduleDelayedItemsLayout()\n", logicalIndex, oldSize, newSize);
    scheduleDelayedItemsLayout();
  }
}

void RouteTreeWidget::scrollRangeChanged(int min, int max)
{
  fprintf(stderr, "RouteTreeWidget::scrollRangeChanged min:%d max:%d\n", min, max);
}

void RouteTreeWidget::scrollSliderMoved(int value)
{
  fprintf(stderr, "RouteTreeWidget::scrollSliderMoved val:%d\n", value);
}

void RouteTreeWidget::scrollValueChanged(int value)
{
  fprintf(stderr, "RouteTreeWidget::scrollValueChanged val:%d\n", value);
}

RouteTreeWidgetItem* RouteTreeWidget::itemFromIndex(const QModelIndex& index) const
{
  return static_cast<RouteTreeWidgetItem*>(QTreeWidget::itemFromIndex(index));
}

RouteTreeWidgetItem* RouteTreeWidget::findItem(const MusECore::Route& r, int type)
{
  QTreeWidgetItemIterator ii(this);
  while(*ii)
  {
    QTreeWidgetItem* item = *ii;
    switch(item->type())
    {
      case RouteTreeWidgetItem::NormalItem:
      case RouteTreeWidgetItem::CategoryItem:
      break;
      
      case RouteTreeWidgetItem::RouteItem:
      case RouteTreeWidgetItem::ChannelsItem:
      {
        RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);
        if((type == -1 || type == item->type()) && rtwi->route().compare(r))
          return rtwi;
      }
      break;
    }
    ++ii;
  }
  return NULL;
  
//   const int cnt = topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(topLevelItem(i));
//     if(!item)
//       continue;
//     if((type == -1 || type == RouteTreeWidgetItem::CategoryItem) && item->route().compare(r))
//       return item;
// 
//     const int c_cnt = item->childCount();
//     for(int j = 0; j < c_cnt; ++j)
//     {
//       RouteTreeWidgetItem* c_item = static_cast<RouteTreeWidgetItem*>(item->child(j));
//       if(!c_item)
//         continue;
//       if((type == -1 || type == RouteTreeWidgetItem::RouteItem) && c_item->route().compare(r))
//         return c_item;
// 
//       const int cc_cnt = c_item->childCount();
//       for(int k = 0; k < cc_cnt; ++k)
//       {
//         RouteTreeWidgetItem* cc_item = static_cast<RouteTreeWidgetItem*>(c_item->child(k));
//         if(!cc_item)
//           continue;
//         if((type == -1 || type == RouteTreeWidgetItem::ChannelsItem) && cc_item->route().compare(r))
//           return cc_item;
//       }
//     }
//   }
//   return 0;
}

RouteTreeWidgetItem* RouteTreeWidget::findCategoryItem(const QString& name)
{
  const int cnt = topLevelItemCount(); 
  for(int i = 0; i < cnt; ++i)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(topLevelItem(i));
    if(item && item->type() == RouteTreeWidgetItem::CategoryItem && item->text(RouteDialog::ROUTE_NAME_COL) == name)
      return item;
  }
  return 0;
}
      
void RouteTreeWidget::getSelectedRoutes(MusECore::RouteList& routes)
{
  RouteTreeItemList sel = selectedItems();
  const int selSz = sel.size();
  if(selSz == 0)
    return;
  for(int idx = 0; idx < selSz; ++idx)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(sel.at(idx));
    if(!item)
      continue;
    item->getSelectedRoutes(routes);
  }
}

//     if(!item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//       continue;
//     MusECore::Route r = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     if(item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
//     {
//       QBitArray ba = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
//       switch(r.type)
//       {
//         case MusECore::Route::TRACK_ROUTE:
//           if(r.track)
//           {
//             const int sz = ba.size();
//             if(r.track->isMidiTrack())
//             {  
//               for(int i = 0; i < sz; ++i)
//               {
//                 if(i >= MIDI_CHANNELS)
//                   break;
//                 if(ba.testBit(i))
//                 {
//                   r.channel = (1 << i);
//                   routes.push_back(r);
//                 }
//               }
//             }
//             else
//             {
//               for(int i = 0; i < sz; ++i)
//               {
//                 if(ba.testBit(i))
//                 {
//                   r.channel = i;
//                   routes.push_back(r);
//                 }
//               }
//             }
//           }
//         break;
//         case MusECore::Route::JACK_ROUTE:
//         case MusECore::Route::MIDI_DEVICE_ROUTE:
//         case MusECore::Route::MIDI_PORT_ROUTE:
//         break;
//       }
//     }
//     else
//       routes.push_back(r);
//   }
// }

int RouteTreeWidget::channelAt(RouteTreeWidgetItem* item, const QPoint& pt)
{
  const QRect rect = visualItemRect(item);
  
  return item->channelAt(pt, rect);
  
//   QPoint p = pt - rect.topLeft();
// 
//   int w = RouteDialog::midiDotsMargin * 2 + RouteDialog::midiDotDiameter * channels;
//   if(channels > 1)
//     w += RouteDialog::midiDotSpacing * (channels - 1);
//   if(channels > 4)
//     w += RouteDialog::midiDotGroupSpacing * (channels - 1) / 4;
//   
//   const int xoff =_isInput ? rect.width() - w : RouteDialog::midiDotsMargin;
//   const int yoff = RouteDialog::midiDotsMargin + (_isInput ? channels : 0);
//   p.setY(p.y() - yoff);
//   p.setX(p.x() - xoff);
//   if(p.y() < 0 || p.y() >= RouteDialog::midiDotDiameter)
//     return -1;
//   for(int i = 0; i < channels; ++i)
//   {
//     if(p.x() < 0)
//       return -1;
//     if(p.x() < RouteDialog::midiDotDiameter)
//       return i;
//     p.setX(p.x() - RouteDialog::midiDotDiameter - RouteDialog::midiDotSpacing);
//     if(i && ((i % 4) == 0))
//       p.setX(p.x() - RouteDialog::midiDotGroupSpacing);
//   }
//   return -1;
}

void RouteTreeWidget::mousePressEvent(QMouseEvent* e)
{
  const QPoint pt = e->pos(); 
  //Qt::KeyboardModifiers km = e->modifiers();
  //bool ctl = km & Qt::ControlModifier;
  //bool shift = km & Qt::ShiftModifier;
  RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(itemAt(pt));
  bool is_cur = item && currentItem() && (item == currentItem());

  //if(is_cur)
  //  QTreeWidget::mousePressEvent(e);
  
  if(item)
  {
    bool changed = item->mousePressHandler(e, visualItemRect(item));
    if(changed)
    {
      //setCurrentItem(item);
      //update(visualItemRect(item));
      setDirtyRegion(visualItemRect(item));
      //emit itemSelectionChanged();
    }
    
    //if(!is_cur)
      QTreeWidget::mousePressEvent(e);

    if(changed && is_cur)
      //setCurrentItem(item);
      emit itemSelectionChanged();
      
    //e->accept();
    return;
    
  }
  QTreeWidget::mousePressEvent(e);
}    
    
/*    
  if(item && item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
  {        
    const MusECore::Route r = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
    switch(r.type)
    {
      case MusECore::Route::TRACK_ROUTE:
        if(r.track && r.channel != -1 && item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
        {
          int chans;
          if(r.track->isMidiTrack())
            chans = MIDI_CHANNELS;
          else
          {
            MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(r.track);
            if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
              chans = _isInput ? atrack->totalOutChannels() : atrack->totalInChannels();
            else
              chans = atrack->channels();
          }

          int ch = channelAt(item, pt, chans);
          
          //QBitArray ba = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
          //QBitArray ba_m = ba;
          QBitArray ba_m = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
          const int ba_sz = ba_m.size();
          bool changed = false;
          //if(!ctl)
          {
            //ba_m.fill(false);
            for(int i = 0; i < ba_sz; ++i)
            {
              //const bool b = ba_m.testBit(i);
              
              if(i == ch)
              {
                if(ctl)
                {
                  ba_m.toggleBit(i);
                  changed = true;
                }
                else
                {
                  if(!ba_m.testBit(i))
                    changed = true;
                  ba_m.setBit(i);
                }
              }
              else if(!ctl)
              {
                if(ba_m.testBit(i))
                  changed = true;
                ba_m.clearBit(i);
              }
                
//               //if(ba_m.testBit(i))
//               {
//                 ba_m.clearBit(i);
//                 changed = true;
//               }
            }
          }
//             //clearChannels();
//           //  clearSelection();
//           //int ch = channelAt(item, pt, chans);
//           if(ch != -1 && ch < ba_sz)
//           {
//             ba_m.toggleBit(ch);
//             changed = true;
//           }

          //if(is_cur)
          //  QTreeWidget::mousePressEvent(e);
            
          //if(ba_m != ba)
          if(changed)
          {
            item->setData(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole, qVariantFromValue<QBitArray>(ba_m));
            //setCurrentItem(item);
            update(visualItemRect(item));
            //emit itemSelectionChanged();
          }
          
          //if(!is_cur)
            QTreeWidget::mousePressEvent(e);

          if(changed && is_cur)
            //setCurrentItem(item);
            emit itemSelectionChanged();
            
          //e->accept();
          return;
        }
      break;
      case MusECore::Route::JACK_ROUTE:
      case MusECore::Route::MIDI_DEVICE_ROUTE:
      case MusECore::Route::MIDI_PORT_ROUTE:
      break;
    }
  }  
  QTreeWidget::mousePressEvent(e);
}*/

// REMOVE Tim. Persistent routes. Added.
// void RouteTreeWidget::clearChannels()
// {
//   int cnt = topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     QTreeWidgetItem* item = topLevelItem(i);
//     if(item)
//     {
//       if(item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
//       {
//         QBitArray ba = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
//         ba.fill(false);
//         item->setData(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole, qVariantFromValue<QBitArray>(ba));
//       }
// 
//       int c_cnt = item->childCount();
//       for(int j = 0; j < c_cnt; ++j)
//       {
//         QTreeWidgetItem* c_item = item->child(j);
//         if(c_item)
//         {
//           if(c_item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
//           {
//             QBitArray c_ba = c_item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
//             c_ba.fill(false);
//             c_item->setData(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole, qVariantFromValue<QBitArray>(c_ba));
//           }
// 
//           int cc_cnt = c_item->childCount();
//           for(int k = 0; k < cc_cnt; ++k)
//           {
//             QTreeWidgetItem* cc_item = c_item->child(k);
//             if(cc_item)
//             {
//               if(cc_item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
//               {
//                 //cc_item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>().fill(false);
//                 QBitArray cc_ba = cc_item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
//                 cc_ba.fill(false);
//                 cc_item->setData(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole, qVariantFromValue<QBitArray>(cc_ba));
//               }
//             }
//           }
//         }
//       }
//     }
//   }
// }

QItemSelectionModel::SelectionFlags RouteTreeWidget::selectionCommand(const QModelIndex& index, const QEvent* e) const
{
  QItemSelectionModel::SelectionFlags flags = QTreeWidget::selectionCommand(index, e);
  fprintf(stderr, "RouteTreeWidget::selectionCommand flags:%d row:%d col:%d ev type:%d\n", int(flags), index.row(), index.column(), e ? e->type() : -1); // REMOVE Tim. Persistent routes. Added.

  RouteTreeWidgetItem* item = itemFromIndex(index);

  if(item && item->type() == RouteTreeWidgetItem::ChannelsItem)
  {
    if(flags & QItemSelectionModel::Toggle)
    {
      flags &= ~QItemSelectionModel::Toggle;
      flags |= QItemSelectionModel::Select;
      fprintf(stderr, "RouteTreeWidget::selectionCommand new flags:%d\n", int(flags)); // REMOVE Tim. Persistent routes. Added.
    }
  }
  
  return flags;
}

//   //if(index.data(RouteDialog::RouteRole).canConvert<MusECore::Route>()) 
//   if(item) 
//   {
//     fprintf(stderr, "RouteTreeWidget::selectionCommand can convert data to Route\n"); // REMOVE Tim. Persistent routes. Added.
//     //fprintf(stderr, "RoutingItemDelegate::paint data is Route\n");  // REMOVE Tim.
//     //const MusECore::Route r = qvariant_cast<MusECore::Route>(index.data(RouteDialog::RouteRole));
//     //const MusECore::Route r = index.data(RouteDialog::RouteRole).value<MusECore::Route>();
//     const MusECore::Route& r = item->route();
//     switch(r.type)
//     {
//       case MusECore::Route::TRACK_ROUTE:
//         //if(e->type() == QEvent:: r.channel != -1)
//         if(r.channel != -1)
//         {
//           if(flags & QItemSelectionModel::Toggle)
//           {
//             flags &= ~QItemSelectionModel::Toggle;
//             flags |= QItemSelectionModel::Select;
//             fprintf(stderr, "RouteTreeWidget::selectionCommand new flags:%d\n", int(flags)); // REMOVE Tim. Persistent routes. Added.
//           }
//         }
//       break;
//       
//       case MusECore::Route::JACK_ROUTE:
//       case MusECore::Route::MIDI_DEVICE_ROUTE:
//       case MusECore::Route::MIDI_PORT_ROUTE:
//       break;
//     }
//   }
//   return flags;
// }

void RouteTreeWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  QModelIndexList mil = deselected.indexes();
  const int dsz = mil.size();
  fprintf(stderr, "RouteTreeWidget::selectionChanged: selected size:%d deselected size:%d\n", selected.size(), dsz); // REMOVE Tim. Persistent routes. Added.
  for(int i = 0; i < dsz; ++i)
  {
    const QModelIndex& index = mil.at(i);
    RouteTreeWidgetItem* item = itemFromIndex(index);
    
    if(item && item->type() == RouteTreeWidgetItem::ChannelsItem)
      item->fillSelectedChannels(false);
  }    
  QTreeWidget::selectionChanged(selected, deselected);
}    
//     //if(item && item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//     if(item)
//     {
//       //const MusECore::Route r = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//       const MusECore::Route& r = item->route();
//       switch(r.type)
//       {
//         case MusECore::Route::TRACK_ROUTE:
//           //if(e->type() == QEvent:: r.channel != -1)
//           if(r.channel != -1)
//           {
// //             if(item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
// //             {
// //               fprintf(stderr, "RouteTreeWidget::selectionChanged: track route: deselected idx:%d clearing channels bitarray\n", i); // REMOVE Tim. Persistent routes. Added.
// //               QBitArray ba = item->data(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
// //               ba.fill(false);
// //               item->setData(RouteDialog::ROUTE_NAME_COL, RouteDialog::ChannelsRole, qVariantFromValue<QBitArray>(ba));
//               item->fillChannels(false);
// //             }
//           }
//         break;
//         
//         case MusECore::Route::JACK_ROUTE:
//         case MusECore::Route::MIDI_DEVICE_ROUTE:
//         case MusECore::Route::MIDI_PORT_ROUTE:
//         break;
//       }
//     }
//     
//   }
//   QTreeWidget::selectionChanged(selected, deselected);
// }

void RouteTreeWidget::scrollBy(int dx, int dy)
{
  fprintf(stderr, "RouteTreeWidget::scrollBy: dx:%d dy:%d\n", dx, dy); // REMOVE Tim.
  int hv = horizontalScrollBar()->value();
  int vv = verticalScrollBar()->value();
  if(dx)
  {
    hv += dx;
    horizontalScrollBar()->setValue(hv);
  }
  if(dy)
  {
    vv += dy;
    verticalScrollBar()->setValue(vv);
  }
}


void RouteTreeWidget::getItemsToDelete(QVector<QTreeWidgetItem*>& items_to_remove, bool showAllMidiPorts)
{
  QTreeWidgetItemIterator ii(this);
  while(*ii)
  {
    QTreeWidgetItem* item = *ii;
    if(item)
    {
      QTreeWidgetItem* twi = item;
      while((twi = twi->parent()))
      {
        if(items_to_remove.contains(twi))
          break;
      }
      // No parent found to be deleted. Determine if this should be deleted.
      if(!twi)
      {
        if(!items_to_remove.contains(item))
        {
          RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);

          switch(rtwi->type())
          {
            case RouteTreeWidgetItem::NormalItem:
            case RouteTreeWidgetItem::CategoryItem:
            case RouteTreeWidgetItem::ChannelsItem:
            break;

            case RouteTreeWidgetItem::RouteItem:
            {
              const MusECore::Route& rt = rtwi->route();
              switch(rt.type)
              {
                case MusECore::Route::MIDI_DEVICE_ROUTE:
                case MusECore::Route::TRACK_ROUTE:
                case MusECore::Route::JACK_ROUTE:
                break;
                
                case MusECore::Route::MIDI_PORT_ROUTE:
                {
                  bool remove_port = false;
                  if(!rt.isValid())
                    remove_port = true;
                  else 
                  if(!showAllMidiPorts)
                  {
                    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[rt.midiPort];
                    if(!mp->device() && (_isInput ? mp->outRoutes()->empty() : mp->inRoutes()->empty()))
                    {
                      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                      if(!_isInput)
                      {
                        MusECore::MidiTrackList* tl = MusEGlobal::song->midis();
                        MusECore::ciMidiTrack imt = tl->begin();
                        for( ; imt != tl->end(); ++imt)
                          if((*imt)->outPort() == rt.midiPort)
                            break;
                        if(imt == tl->end())
                          remove_port = true;
                      }
                      else
#endif  // _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
                        remove_port = true;

                    }
                  }

                  if(remove_port)
                    items_to_remove.append(item);
                  ++ii;
                  continue;
                }
                break;
              }
            }
            break;
          }

          if(!rtwi->routeNodeExists())
            items_to_remove.append(item);
        }
      }
    }
    ++ii;
  }
  
/*  
  
  const int cnt = topLevelItemCount(); 
  for(int i = 0; i < cnt; ++i)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(topLevelItem(i));
    if(item)
    {
      const int c_cnt = item->childCount();
      for(int j = 0; j < c_cnt; ++j)
      {
        RouteTreeWidgetItem* c_item = static_cast<RouteTreeWidgetItem*>(item->child(j));
        if(c_item)
        {
          const int cc_cnt = c_item->childCount();
          for(int k = 0; k < cc_cnt; ++k)
          {
            RouteTreeWidgetItem* cc_item = static_cast<RouteTreeWidgetItem*>(c_item->child(k));
            if(cc_item)
            {
              if(!cc_item->routeNodeExists())
                items_to_remove.append(cc_item);
            }
          }
          if(!c_item->routeNodeExists())
            items_to_remove.append(c_item);
        }
      }
      if(!item->routeNodeExists())
        items_to_remove.append(item);
    }
  }*/
}

//-----------------------------------
//   RoutingItemDelegate
//-----------------------------------

RoutingItemDelegate::RoutingItemDelegate(bool is_input, RouteTreeWidget* tree, QWidget *parent) 
                    : QStyledItemDelegate(parent), _tree(tree), _isInput(is_input)
{
  _firstPress = true;
}

// //-----------------------------------
// //   getItemRectangle
// //   editor is optional and provides info 
// //-----------------------------------
// 
// QRect RoutingItemDelegate::getItemRectangle(const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement subElement, QWidget* editor) const
// {
//     // Taken from QStyledItemDelegate source. 
//     QStyleOptionViewItemV4 opt = option;
//     initStyleOption(&opt, index);
//     const QWidget* widget = NULL;
//     const QStyleOptionViewItemV3* v3 = qstyleoption_cast<const QStyleOptionViewItemV3*>(&option);
//     if(v3)
//       widget = v3->widget;
//     // Let the editor take up all available space if the editor is not a QLineEdit or it is in a QTableView.
//     #if !defined(QT_NO_TABLEVIEW) && !defined(QT_NO_LINEEDIT)
//     if(editor && qobject_cast<QLineEdit*>(editor) && !qobject_cast<const QTableView*>(widget))
//       opt.showDecorationSelected = editor->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, editor);
//     else
//     #endif
//       opt.showDecorationSelected = true;
//     const QStyle *style = widget ? widget->style() : QApplication::style();
// //     if(editor->layoutDirection() == Qt::RightToLeft)
// //     {
// //       const int delta = qSmartMinSize(editor).width() - r.width();       // qSmartMinSize ???
// //       if (delta > 0)
// //       {
// //         //we need to widen the geometry
// //         r.adjust(-delta, 0, 0, 0);
// //       }
// //     }
// 
//   return style->subElementRect(subElement, &opt, widget);
// }
// 
// //-----------------------------------
// //   subElementHitTest
// //   editor is optional and provides info
// //-----------------------------------
// 
// bool RoutingItemDelegate::subElementHitTest(const QPoint& point, const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement* subElement, QWidget* editor) const
// {
//   QRect checkBoxRect = getItemRectangle(option, index, QStyle::SE_ItemViewItemCheckIndicator, editor);
//   if(checkBoxRect.isValid() && checkBoxRect.contains(point))
//   {
//     if(subElement)
//       (*subElement) = QStyle::SE_ItemViewItemCheckIndicator;
//     return true;
//   }
// 
//   QRect decorationRect = getItemRectangle(option, index, QStyle::SE_ItemViewItemDecoration, editor);
//   if(decorationRect.isValid() && decorationRect.contains(point))
//   {
//     if(subElement)
//       (*subElement) = QStyle::SE_ItemViewItemDecoration;
//     return true;
//   }
// 
//   QRect textRect = getItemRectangle(option, index, QStyle::SE_ItemViewItemText, editor);
//   if(textRect.isValid() && textRect.contains(point))
//   {
//     if(subElement)
//       (*subElement) = QStyle::SE_ItemViewItemText;
//     return true;
//   }
// 
//   return false;
// }

// //-----------------------------------
// //   updateEditorGeometry
// //-----------------------------------
// 
// void RoutingItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
// {
//   // REMOVE Tim.
//   fprintf(stderr, "ColorChooserEditor::updateEditorGeometry editor x:%d y:%d w:%d h:%d rect x:%d y:%d w:%d h:%d\n",
//           editor->x(), editor->y(), editor->width(), editor->height(),
//           option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height());
// 
//   // For the color editor, move it down to the start of the next item so it doesn't cover the current item row.
//   // Width and height are not used - the color editor fixates it's own width and height.
//   if(index.column() == ControlMapperDialog::C_NAME)
//   {
//     QRect r = getItemRectangle(option, index, QStyle::SE_ItemViewItemText, editor);  // Get the text rectangle.
//     if(r.isValid())
//     {
//       editor->move(r.x(), option.rect.y() + option.rect.height());
//       return;
//     }
//   }
//   
//   QStyledItemDelegate::updateEditorGeometry(editor, option, index);
// }

void RoutingItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
//   fprintf(stderr, "RoutingItemDelegate::paint row:%d col:%d, rect x:%d y:%d w:%d h:%d showDecorationSelected:%d\n",
//           index.row(), index.column(),
//           option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height(),
//           option.showDecorationSelected);  // REMOVE Tim.

  RouteTreeWidgetItem* item = _tree->itemFromIndex(index);
  if(item && item->paint(painter, option, index))
    return;
  QStyledItemDelegate::paint(painter, option, index);
}  

  //QStyleOptionViewItemV4 opt = option;
  //initStyleOption(&opt, index);
  //opt.showDecorationSelected = false;
  
  // TODO: Don't forget these if necessary.
  //painter->save();
  //painter->restore();
  
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//
//         if (option.state & QStyle::State_Selected)
//             painter->fillRect(option.rect, option.palette.highlight());
//
//         starRating.paint(painter, option.rect, option.palette,
//                         StarRating::ReadOnly);
//     } else

//   if(index.column() == ControlMapperDialog::C_NAME)
//   {
//     // TODO: Disable all this Style stuff if using a style sheet.
// 
//     //QRect disclosure_r = getItemRectangle(option, index, QStyle::SE_TreeViewDisclosureItem);  // Get the text rectangle.
//     //if(disclosure_r.isValid())
//     //{
//     //}
//       
//     QRect checkbox_r = getItemRectangle(option, index, QStyle::SE_ItemViewItemCheckIndicator);  // Get the text rectangle.
//     if(checkbox_r.isValid())
//     {
//       if(option.state & QStyle::State_Selected)
//         painter->fillRect(checkbox_r & option.rect, option.palette.highlight());
//       QStyleOptionViewItemV4 opt = option;
//       initStyleOption(&opt, index);         // Required ?
//       opt.rect = checkbox_r & option.rect;
//       QApplication::style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &opt, painter);
//       //QApplication::style()->drawControl();
//     }
// 
//     //QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
// 
//     //QApplication::style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, painter);
//     
//     QRect deco_r = getItemRectangle(option, index, QStyle::SE_ItemViewItemDecoration);  // Get the text rectangle.
//     if(deco_r.isValid())
//       painter->fillRect(deco_r & option.rect, index.data(Qt::DecorationRole).value<QColor>());
//     
//     QRect text_r = getItemRectangle(option, index, QStyle::SE_ItemViewItemText);  // Get the text rectangle.
//     if(text_r.isValid())
//     {
//       if(option.state & QStyle::State_Selected)
//         painter->fillRect(text_r & option.rect, option.palette.highlight());
//       QApplication::style()->drawItemText(painter, text_r & option.rect, option.displayAlignment, option.palette, true, index.data(Qt::DisplayRole).toString());
//     }
//     
//     return;
//   }
  
  //QStyledItemDelegate::paint(painter, option, index);
  
  //fprintf(stderr, "RoutingItemDelegate::paint\n");  // REMOVE Tim.
  
// //   RouteDialog* router = qobject_cast< RouteDialog* >(parent());
//   //if(parent() && qobject_cast< RouteDialog* >(parent()))
// //   if(router)
// //   {
//     //fprintf(stderr, "RoutingItemDelegate::paint parent is RouteDialog\n");  // REMOVE Tim.
//     //QWidget* qpd = qobject_cast<QWidget*>(painter->device());
//     //if(qpd)
//     if(painter->device())
//     {
//       //fprintf(stderr, "RoutingItemDelegate::paint device is QWidget\n");  // REMOVE Tim.
//       //RouteDialog* router = static_cast<RouteDialog*>(parent());
//       
//       if(index.column() == RouteDialog::ROUTE_NAME_COL && index.data(RouteDialog::RouteRole).canConvert<MusECore::Route>()) 
//       {
//         //fprintf(stderr, "RoutingItemDelegate::paint data is Route\n");  // REMOVE Tim.
//         MusECore::Route r = qvariant_cast<MusECore::Route>(index.data(RouteDialog::RouteRole));
//         QRect rect(option.rect);
//         switch(r.type)
//         {
//           case MusECore::Route::TRACK_ROUTE:
//             //fprintf(stderr, "RoutingItemDelegate::paint route is track\n");  // REMOVE Tim.
//             if(r.track && r.channel != -1)
//             {
//               int chans; 
//               if(r.track->isMidiTrack())
//               {
//                 //fprintf(stderr, "RoutingItemDelegate::paint track is midi\n");  // REMOVE Tim.
//                 chans = MIDI_CHANNELS;
//               }
//               else
//               {
//                 //fprintf(stderr, "RoutingItemDelegate::paint track is audio\n");  // REMOVE Tim.
//                 MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(r.track);
//                 if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//                 {
//                   if(_isInput)
//                     chans = atrack->totalOutChannels();
//                   else
//                     chans = atrack->totalInChannels();
//                 }
//                 else
//                   chans = atrack->channels();
//               }
//               
//               int w = RouteDialog::midiDotsMargin * 2 + RouteDialog::midiDotDiameter * chans;
//               if(chans > 1)
//                 w += RouteDialog::midiDotSpacing * (chans - 1);
//               if(chans > 4)
//                 w += RouteDialog::midiDotGroupSpacing * (chans - 1) / 4;
//               
//               //fprintf(stderr, "RoutingItemDelegate::paint src list width:%d src viewport width:%d\n", router->newSrcList->width(), router->newSrcList->viewport()->width());  // REMOVE Tim.
//               //int x = _isInput ? router->newSrcList->viewport()->width() - w : RouteDialog::midiDotsMargin;
//               //int x = _isInput ? painter->device()->width() - w : RouteDialog::midiDotsMargin;
//               int x = _isInput ? _tree->width() - w : RouteDialog::midiDotsMargin;
//               const int y = RouteDialog::midiDotsMargin + (_isInput ? chans : 0);
//               QBitArray ba;
//               int basize = 0;
//               if(index.data(RouteDialog::ChannelsRole).canConvert<QBitArray>())
//               {
//                 ba = index.data(RouteDialog::ChannelsRole).value<QBitArray>();
//                 basize = ba.size();
//               }
//               
//               for(int i = 0; i < chans; )
//               {
//                 painter->setPen(Qt::black);
//                 //painter->drawRoundedRect(option.rect.x() + x, option.rect.y() + y, 
//                 if(!ba.isNull() && i < basize && ba.testBit(i))
//                   painter->fillRect(x, option.rect.y() + y, 
//                                            RouteDialog::midiDotDiameter, RouteDialog::midiDotDiameter,
//                                            option.palette.highlight());
//                 //else
//                   painter->drawRoundedRect(x, option.rect.y() + y, 
//                                            RouteDialog::midiDotDiameter, RouteDialog::midiDotDiameter,
//                                            30, 30);
//                 if((i % 2) == 0)
//                   painter->setPen(Qt::darkGray);
//                 else
//                   painter->setPen(Qt::black);
//                 int xline = x + RouteDialog::midiDotDiameter / 2;
//                 if(_isInput)
//                 {
//                   int yline = option.rect.y() + y;
//                   painter->drawLine(xline, yline, xline, yline - chans + i);
//                   //painter->drawLine(xline, yline - chans + i, painter->device()->width(), yline - chans + i);
//                   painter->drawLine(xline, yline - chans + i, _tree->width(), yline - chans + i);
//                   
//                 }
//                 else
//                 {
//                   int yline = option.rect.y() + RouteDialog::midiDotsMargin + RouteDialog::midiDotDiameter;
//                   painter->drawLine(xline, yline, xline, yline + i);
//                   painter->drawLine(0, yline + i, xline, yline + i);
//                   
//                 }
//                 
//                 ++i;
//                 x += RouteDialog::midiDotDiameter + RouteDialog::midiDotSpacing;
//                 if(i && ((i % 4) == 0))
//                   x += RouteDialog::midiDotGroupSpacing;
//               }
//               return;
//             }
//           break;  
//           case MusECore::Route::MIDI_DEVICE_ROUTE:
//           case MusECore::Route::MIDI_PORT_ROUTE:
//           case MusECore::Route::JACK_ROUTE:
//           break;  
//         }
//       }
//     }
// //   }
//   QStyledItemDelegate::paint(painter, option, index);
// }

// QWidget* RoutingItemDelegate::createEditor(QWidget *parent,
//                                     const QStyleOptionViewItem &option,
//                                     const QModelIndex &index) const
// {
// //     if (index.data().canConvert<StarRating>()) {
// //         StarEditor *editor = new StarEditor(parent);
// //         connect(editor, SIGNAL(editingFinished()),
// //                 this, SLOT(commitAndCloseEditor()));
// //         return editor;
// //     } else
// 
//   int opt_state = option.state;
//   fprintf(stderr, "RoutingItemDelegate::createEditor option state:%d\n", opt_state);  // REMOVE Tim.
// 
//   // HACK: For some reason when using CurrentChanged trigger, createEditor is called upon opening the dialog, yet nothing is selected.
//   // It suddenly started doing that after working just fine. Can't find what may have changed.
//   //if(!(option.state & QStyle::State_Selected))   // Nope. option.state is always the same, never seems to change.
//   //  return NULL;
//   //if(_firstPress)
//   //  return NULL;
//   
//   switch(index.column())
//   {
// //     case ControlMapperDialog::C_SHOW:
// //       //return QStyledItemDelegate::createEditor(parent, option, index);
// //       // This is a checkbox column. No editable info.
// //       //fprintf(stderr, "ERROR: RoutingItemDelegate::createEditor called for SHOW column\n");
// //       return 0;
// 
//     //case ControlMapperDialog::C_NAME:
//       //fprintf(stderr, "ERROR: RoutingItemDelegate::createEditor called for NAME column\n");
//       // This seems to be a way we can prevent editing of a cell here in this tree widget.
//       // Table widget has individual item cell edting enable but here in tree widget it's per row.
//       //return 0;
// 
//     //case ControlMapperDialog::C_COLOR:
//     case ControlMapperDialog::C_NAME:
//     {
//       ColorEditor* color_list = new ColorEditor(parent);
//       //connect(color_list, SIGNAL(activated(int)), this, SLOT(colorEditorChanged()));
//       connect(color_list, SIGNAL(activated(const QColor&)), this, SLOT(editorChanged()));
//       return color_list;
//     }
// 
//     case ControlMapperDialog::C_ASSIGN_PORT:
//     {
//       QComboBox* combo = new QComboBox(parent);
// 
// //       combo->addItem(tr("<None>"), -1);
// //       combo->addItem(tr("Control7"), MusECore::MidiController::Controller7);
// //       combo->addItem(tr("Control14"), MusECore::MidiController::Controller14);
// //       combo->addItem(tr("RPN"), MusECore::MidiController::RPN);
// //       combo->addItem(tr("NPRN"), MusECore::MidiController::NRPN);
// //       combo->addItem(tr("RPN14"), MusECore::MidiController::RPN14);
// //       combo->addItem(tr("NRPN14"), MusECore::MidiController::NRPN14);
// //       combo->addItem(tr("Pitch"), MusECore::MidiController::Pitch);
// //       combo->addItem(tr("Program"), MusECore::MidiController::Program);
// //       //combo->addItem(tr("PolyAftertouch"), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
// //       combo->addItem(tr("Aftertouch"), MusECore::MidiController::Aftertouch);
// //       //combo->setCurrentIndex(0);
// 
// //       combo->addItem(tr("<None>"), -1);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller7), MusECore::MidiController::Controller7);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller14), MusECore::MidiController::Controller14);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN), MusECore::MidiController::RPN);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN), MusECore::MidiController::NRPN);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN14), MusECore::MidiController::RPN14);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN14), MusECore::MidiController::NRPN14);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Pitch), MusECore::MidiController::Pitch);
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Program), MusECore::MidiController::Program);
// //       //combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::PolyAftertouch), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
// //       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Aftertouch), MusECore::MidiController::Aftertouch);
// 
//       combo->addItem("---", -1);
//       int port = index.data(RouteDialog::RouteRole).toInt();
//       QString port_name;
//       for(int i = 0; i < MIDI_PORTS; ++i)
//       {
//         MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device();
//         //if(!md)  // In the case of this combo box, don't bother listing empty ports.
//         //  continue;
//         //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))
//         if(!(md && (md->rwFlags() & 2)) && (i != port))   // Only readable ports, or current one.
//           continue;
//         //name.sprintf("%d:%s", i+1, MusEGlobal::midiPorts[i].portname().toLatin1().constData());
//         QString name = QString("%1:%2").arg(i+1).arg(MusEGlobal::midiPorts[i].portname());
//         combo->addItem(name, i);
//       }
//       connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged()));
//       return combo;
//     }
// 
//     case ControlMapperDialog::C_ASSIGN_CHAN:
//     {
// //       QSpinBox* spin_box = new QSpinBox(parent);
// //       spin_box->setMinimum(0);
// //       spin_box->setMaximum(127);
// //       return spin_box;
// 
//       QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
//       QSpinBox* spin_box = qobject_cast<QSpinBox*>(widget);
//       if(spin_box)
//       {
//         spin_box->setMinimum(0);
//         spin_box->setMaximum(MIDI_CHANNELS - 1);
//       }
//       return widget;
//     }
//     
//     case ControlMapperDialog::C_MCTL_NUM:
//     {
//       QComboBox* combo = new QComboBox(parent);
// 
// //       combo->addItem(tr("<None>"), -1);
// //       combo->addItem(tr("Control7"), MusECore::MidiController::Controller7);
// //       combo->addItem(tr("Control14"), MusECore::MidiController::Controller14);
// //       combo->addItem(tr("RPN"), MusECore::MidiController::RPN);
// //       combo->addItem(tr("NPRN"), MusECore::MidiController::NRPN);
// //       combo->addItem(tr("RPN14"), MusECore::MidiController::RPN14);
// //       combo->addItem(tr("NRPN14"), MusECore::MidiController::NRPN14);
// //       combo->addItem(tr("Pitch"), MusECore::MidiController::Pitch);
// //       combo->addItem(tr("Program"), MusECore::MidiController::Program);
// //       //combo->addItem(tr("PolyAftertouch"), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
// //       combo->addItem(tr("Aftertouch"), MusECore::MidiController::Aftertouch);
// //       //combo->setCurrentIndex(0);
// 
//       //combo->addItem(tr("<None>"), -1);
//       combo->addItem("---", -1);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller7), MusECore::MidiController::Controller7);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller14), MusECore::MidiController::Controller14);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN), MusECore::MidiController::RPN);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN), MusECore::MidiController::NRPN);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN14), MusECore::MidiController::RPN14);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN14), MusECore::MidiController::NRPN14);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Pitch), MusECore::MidiController::Pitch);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Program), MusECore::MidiController::Program);
//       // TODO Per-pitch controls not supported yet. Need a way to select pitch.
//       //combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::PolyAftertouch), MusECore::MidiController::PolyAftertouch);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Aftertouch), MusECore::MidiController::Aftertouch);
//       connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged()));
//       return combo;
//     }
// 
// //     case ControlMapperDialog::C_MCTL_H:
// //     {
// // //       QSpinBox* spin_box = new QSpinBox(parent);
// // //       spin_box->setMinimum(0);
// // //       spin_box->setMaximum(127);
// // //       return spin_box;
// //       
// //       QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
// //       QSpinBox* spin_box = qobject_cast<QSpinBox*>(widget);
// //       if(spin_box)
// //       {
// //         spin_box->setMinimum(0);
// //         spin_box->setMaximum(127);
// //       }
// //       return widget;
// //     }
// 
// ///     case ControlMapperDialog::C_MCTL_H:
// //     case ControlMapperDialog::C_MCTL_L:
// //     {
// // //       QSpinBox* spin_box = new QSpinBox(parent);
// // //       spin_box->setMinimum(0);
// // //       spin_box->setMaximum(127);
// // //       return spin_box;
// //       
// //       QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
// //       QSpinBox* spin_box = qobject_cast<QSpinBox*>(widget);
// //       if(spin_box)
// //       {
// //         spin_box->setMinimum(0);
// //         spin_box->setMaximum(127);
// //       }
// //       return widget;
// //     }
//   }
//   
//   return QStyledItemDelegate::createEditor(parent, option, index);
// }

// void RoutingItemDelegate::editorChanged()
// {
// //     StarEditor *editor = qobject_cast<StarEditor *>(sender());
// //     emit commitData(editor);
// //     emit closeEditor(editor);
// 
//   fprintf(stderr, "RoutingItemDelegate::editorChanged\n");  // REMOVE Tim.
//   
//   // Wow, I thought using sender was frowned upon ("breaks modularity"). But hey, it's necessary sometimes. TODO Improve this?
//   //ColorEditor* editor = qobject_cast<ColorEditor*>(sender());
//   QWidget* editor = qobject_cast<QWidget*>(sender());
//   if(editor)
//   {
//     emit commitData(editor);
//     emit closeEditor(editor);
//   }
// }

// // void RoutingItemDelegate::commitAndCloseEditor()
// // {
// // //     StarEditor *editor = qobject_cast<StarEditor *>(sender());
// // //     emit commitData(editor);
// // //     emit closeEditor(editor);
// // }

// void RoutingItemDelegate::setEditorData(QWidget *editor,
//                                   const QModelIndex &index) const
// {
//   fprintf(stderr, "RoutingItemDelegate::setEditorData\n");  // REMOVE Tim.
// //      if (index.data().canConvert<StarRating>()) {
// //          StarRating starRating = qvariant_cast<StarRating>(index.data());
// //          StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
// //          starEditor->setStarRating(starRating);
// //      } else
//   
//    //if(index.column() == ControlMapperDialog::C_COLOR)
// 
// 
//   switch(index.column())
//   {
//     case ControlMapperDialog::C_NAME:
//     {
//       ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
//       if(color_editor)
//         color_editor->setColor(index.data(Qt::DecorationRole).value<QColor>());
//       return;
//     }
// 
//     case ControlMapperDialog::C_ASSIGN_PORT:
//     case ControlMapperDialog::C_MCTL_NUM:
//     {
//       QComboBox* combo = qobject_cast<QComboBox*>(editor);
//       if(combo)
//       {
//         int data = index.data(RouteDialog::RouteRole).toInt();
//         int idx = combo->findData(data);
//         if(idx != -1)
//         {
//           combo->blockSignals(true);     // Prevent currentIndexChanged or activated from being called
//           combo->setCurrentIndex(idx);
//           combo->blockSignals(false);
//         }
//       }
//       return;
//     }
// 
//     default:
//       QStyledItemDelegate::setEditorData(editor, index);
//   }
//    
// //    if(index.column() == ControlMapperDialog::C_NAME)
// //    {
// //      ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
// //      if(color_editor)
// //        color_editor->setColor(index.data(Qt::DecorationRole).value<QColor>());
// //    }
// //    else
// //    if(index.column() == ControlMapperDialog::C_ASSIGN_PORT)
// //    {
// //      QComboBox* combo = qobject_cast<QComboBox*>(editor);
// //      if(combo)
// //      {
// //        int data = index.data(RouteDialog::RouteRole).toInt();
// //        int idx = combo->findData(data);
// //        if(idx != -1)
// //          combo->setCurrentIndex(idx);
// //      }
// //    }
// //    else
// //    if(index.column() == ControlMapperDialog::C_MCTL_TYPE)
// //    {
// //      QComboBox* combo = qobject_cast<QComboBox*>(editor);
// //      if(combo)
// //      {
// //        int data = index.data(RouteDialog::RouteRole).toInt();
// //        int idx = combo->findData(data);
// //        if(idx != -1)
// //          combo->setCurrentIndex(idx);
// //      }
// //    }
// //    else
// //      QStyledItemDelegate::setEditorData(editor, index);
// }

void RoutingItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
  fprintf(stderr, "RoutingItemDelegate::setModelData\n");  // REMOVE Tim.
//      if (index.data().canConvert<StarRating>()) {
//          StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
//          model->setData(index, QVariant::fromValue(starEditor->starRating()));
//      } else

   //if(index.column() == ControlMapperDialog::C_COLOR)

  switch(index.column())
  {
//     case ControlMapperDialog::C_NAME:
//     {
//       ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
//       if(color_editor)
//         model->setData(index, color_editor->color(), Qt::DecorationRole);
//       return;
//     }
// 
//     case ControlMapperDialog::C_ASSIGN_PORT:
//     case ControlMapperDialog::C_MCTL_NUM:
//     {
//       QComboBox* combo = qobject_cast<QComboBox*>(editor);
//       if(combo)
//       {
//         int idx = combo->currentIndex();
//         if(idx != -1)
//         {
//           model->setData(index, combo->itemData(idx), RouteDialog::RouteRole);    // Do this one before the text so that the tree view's itemChanged handler gets it first!
//           model->blockSignals(true);
//           model->setData(index, combo->itemText(idx), Qt::DisplayRole); // This will cause another handler call. Prevent it by blocking.
//           model->blockSignals(false);
//         }
//       }
//       return;
//     }

    default:
       QStyledItemDelegate::setModelData(editor, model, index);
  }

//    if(index.column() == ControlMapperDialog::C_NAME)
//    {
//      ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
//      if(color_editor)
//        model->setData(index, color_editor->color(), Qt::DecorationRole);
//    }
//    else
//    if(index.column() == ControlMapperDialog::C_ASSIGN_PORT)
//    {
//      QComboBox* combo = qobject_cast<QComboBox*>(editor);
//      if(combo)
//      {
//        int idx = combo->currentIndex();
//        if(idx != -1)
//        {
//          model->setData(index, combo->itemText(idx), Qt::DisplayRole);
//          model->setData(index, combo->itemData(idx), RouteDialog::RouteRole);
//        }
//      }
//    }
//    else
//    if(index.column() == ControlMapperDialog::C_MCTL_TYPE)
//    {
//      QComboBox* combo = qobject_cast<QComboBox*>(editor);
//      if(combo)
//      {
//        int idx = combo->currentIndex();
//        if(idx != -1)
//        {
//          model->setData(index, combo->itemText(idx), Qt::DisplayRole);
//          model->setData(index, combo->itemData(idx), RouteDialog::RouteRole);
//        }
//      }
//    }
//    else
//      QStyledItemDelegate::setModelData(editor, model, index);
}

QSize RoutingItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//         return starRating.sizeHint();
//     } else

//   if(index.column() == ControlMapperDialog::C_COLOR)
//     return QSize(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__,
//                  __COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__));
//     
  //return QStyledItemDelegate::sizeHint(option, index);

  RouteTreeWidgetItem* item = _tree->itemFromIndex(index);
  if(item)
  { 
    const QSize sz = item->getSizeHint(option, index);
    if(sz.isValid())
      return sz;
    //return item->getSizeHint(option, index);
  }
  return QStyledItemDelegate::sizeHint(option, index);
}  

//   RouteDialog* router = qobject_cast< RouteDialog* >(parent());
//   if(router)
//   {
//     if(index.column() == RouteDialog::ROUTE_NAME_COL && index.data(RouteDialog::RouteRole).canConvert<MusECore::Route>()) 
//     {
//       MusECore::Route r = qvariant_cast<MusECore::Route>(index.data(RouteDialog::RouteRole));
//       switch(r.type)
//       {
//         case MusECore::Route::TRACK_ROUTE:
//           if(r.track && r.channel != -1)
//           {
//             int chans; 
//             if(r.track->isMidiTrack())
//               chans = MIDI_CHANNELS;
//             else
//             {
//               MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(r.track);
//               if(atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//               {
//                 if(_isInput)
//                   chans = atrack->totalOutChannels();
//                 else
//                   chans = atrack->totalInChannels();
//               }
//               else
//                 chans = atrack->channels();
//             }
//             int w = RouteDialog::midiDotsMargin * 2 + RouteDialog::midiDotDiameter * chans;
//             if(chans > 1)
//               w += RouteDialog::midiDotSpacing * (chans - 1);
//             if(chans > 4)
//               w += RouteDialog::midiDotGroupSpacing * (chans - 1) / 4;
//             const int h = RouteDialog::midiDotDiameter + RouteDialog::midiDotsMargin * 2 + chans;
//             return QSize(w, h);
//           }
//         break;  
//         case MusECore::Route::MIDI_DEVICE_ROUTE:
//         case MusECore::Route::MIDI_PORT_ROUTE:
//         case MusECore::Route::JACK_ROUTE:
//         break;  
//       }
//     }
//   }
//   return QStyledItemDelegate::sizeHint(option, index);
// }

bool RoutingItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
//   if(event->type() == QEvent::MouseMove)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "RoutingItemDelegate::editorEvent: Move X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
//     // If any buttons down, ignore.
// //     if(me->buttons() != Qt::NoButton)
// //     {
// //       event->accept();
// //       return true;
// //     }
//   }
//   else
//   if(event->type() == QEvent::MouseButtonPress)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "RoutingItemDelegate::editorEvent: Press X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
// 
// //     _firstPress = false;  // HACK
// //     
// //     QStyle::SubElement sub_element;
// //     if(subElementHitTest(me->pos(), option, index, &sub_element))
// //       _currentSubElement = sub_element;
// //     //event->accept();
// //     //return true;
//   }
//   else
//   if(event->type() == QEvent::MouseButtonRelease)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "RoutingItemDelegate::editorEvent: Release X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
// 
// //     // If the element under the mouse is not the one when pressed, eat up these events because
// //     //  they trigger the editor or action of the element under the mouse at the release position.
// //     QStyle::SubElement sub_element = _currentSubElement;
// //     if(!subElementHitTest(me->pos(), option, index, &sub_element) || sub_element != _currentSubElement)
// //     //QRect r = getItemRectangle(option, index, QStyle::SE_ItemViewItemDecoration);
// //     //if(!subElementHitTest(me->pos(), option, index, &sub_element) ||
// //     //  (sub_element != QStyle::SE_ItemViewItemCheckIndicator && sub_element != QStyle::SE_ItemViewItemDecoration))
// //     //if(r.isValid())
// //     {
// //       event->accept();
// //       return true;
// //     }
//   }
//   else
//   if(event->type() == QEvent::Close)
//   {
//     fprintf(stderr, "RoutingItemDelegate::editorEvent: Close\n");  // REMOVE Tim.
//   }
//   else
//     fprintf(stderr, "RoutingItemDelegate::editorEvent: event type:%d\n", event->type());  // REMOVE Tim.


//   switch(index.column())
//   {
//     case ControlMapperDialog::C_SHOW:
//       // This is checkbox column. No editable info.
//       //event->accept();
//       //return true;
//       //return false;
//       return QStyledItemDelegate::editorEvent(event, model, option, index);
// 
//     case ControlMapperDialog::C_NAME:
//       // This is non-editable name.
//       event->accept();
//       return true;
// 
//     case ControlMapperDialog::C_COLOR:
//     {
//       if(event->type() == QEvent::MouseButtonRelease)
//       {
//         QMouseEvent* me = static_cast<QMouseEvent*>(event);
//         fprintf(stderr, " X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
// 
//       }
// 
//       event->accept();
//       return true;
//     }
// 
//     case ControlMapperDialog::C_ASSIGN:
//       // This is editable assigned input controller.
//       return false;
// 
//     case ControlMapperDialog::C_MCTL_TYPE:
//       // This is editable midi control type.
//       return false;
// 
//     case ControlMapperDialog::C_MCTL_H:
//       // This is editable midi control num high.
//       return false;
// 
//     case ControlMapperDialog::C_MCTL_L:
//       // This is editable midi control num low.
//       return false;
//   }
// 
//   return false;

  return QStyledItemDelegate::editorEvent(event, model, option, index);
}


bool RoutingItemDelegate::eventFilter(QObject* editor, QEvent* event)
{
  if(event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "RoutingItemDelegate::eventFilter: Press X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
    //event->accept();
    //return true;
  }
  else
  if(event->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "RoutingItemDelegate::eventFilter: Release X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
    //event->accept();
    //return true;
  }
  else
  if(event->type() == QEvent::Close)
  {
    fprintf(stderr, "RoutingItemDelegate::eventFilter: Close\n");  // REMOVE Tim.
  }
  else
    fprintf(stderr, "RoutingItemDelegate::eventFilter: event type:%d\n", event->type());  // REMOVE Tim.

  return QStyledItemDelegate::eventFilter(editor, event);
}





//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

RouteDialog::RouteDialog(QWidget* parent)
   : QDialog(parent)
{
  setupUi(this);
  
  //newSrcList->viewport()->setLayoutDirection(Qt::LeftToRight);
  
  //_srcFilterItem = NULL;
  //_dstFilterItem = NULL;

  srcItemDelegate = new RoutingItemDelegate(true, newSrcList, this);
  dstItemDelegate = new RoutingItemDelegate(false, newDstList, this);
  
  newSrcList->setItemDelegate(srcItemDelegate);
  newDstList->setItemDelegate(dstItemDelegate);
  
  //newSrcList->setItemsExpandable(false);  // REMOVE Tim. For test only.
  //newDstList->setItemsExpandable(false);  // REMOVE Tim. For test only.
  
  connectionsWidget->setRouteDialog(this);

  // REMOVE Tim. Persistent routes. Added, changed.
  QStringList columnnames;
  columnnames << tr("Source");
              //<< tr("Type");
  newSrcList->setColumnCount(columnnames.size());
  newSrcList->setHeaderLabels(columnnames);
  for (int i = 0; i < columnnames.size(); ++i) {
        //setWhatsThis(newSrcList->horizontalHeaderItem(i), i);
        //setToolTip(newSrcList->horizontalHeaderItem(i), i);
        }
        
  columnnames.clear();
  columnnames << tr("Destination");
              //<< tr("Type");
  newDstList->setColumnCount(columnnames.size());
  newDstList->setHeaderLabels(columnnames);
  for (int i = 0; i < columnnames.size(); ++i) {
        //setWhatsThis(newDstList->horizontalHeaderItem(i), i);
        //setToolTip(newDstList->horizontalHeaderItem(i), i);
        }

  // We are using right-to-left layout for the source tree widget, to force the scroll bar on the left.
  // But this makes incorrect tree indentation (it indents towards the LEFT).
  // And for both the source and destination tree widgets the tree indent marks interfere with the column contents placement (pushing it over).
  // In this case, the tree works better (best) when a second column is reserved for it (LEFT of the source column, RIGHT of destination column.)
  // But that makes an awkward left-indenting tree left of the source column and right-indenting tree right of the destination column.
  // So get rid of the tree, move it to a second column, which does not exist. We will draw and handle our own tree.
  newSrcList->setTreePosition(1);
  newDstList->setTreePosition(1);
  
  // Need this. Don't remove.
  newSrcList->header()->setSectionResizeMode(QHeaderView::Stretch);
  newDstList->header()->setSectionResizeMode(QHeaderView::Stretch);

  newSrcList->setTextElideMode(Qt::ElideMiddle);
  newDstList->setTextElideMode(Qt::ElideMiddle);

  
  columnnames.clear();
  columnnames << tr("Source")
              << tr("Destination");
  routeList->setColumnCount(columnnames.size());
  routeList->setHeaderLabels(columnnames);
  for (int i = 0; i < columnnames.size(); ++i) {
        //setWhatsThis(routeList->horizontalHeaderItem(i), i);
        //setToolTip(routeList->horizontalHeaderItem(i), i);
        }
  
  routingChanged();

  connect(newSrcList->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), srcTreeScrollBar, SLOT(setRange(int,int))); 
  connect(newDstList->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), dstTreeScrollBar, SLOT(setRange(int,int))); 
  connect(newSrcList->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(srcTreeScrollValueChanged(int))); 
  connect(newDstList->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(dstTreeScrollValueChanged(int))); 
  connect(srcTreeScrollBar, SIGNAL(valueChanged(int)), SLOT(srcScrollBarValueChanged(int))); 
  connect(dstTreeScrollBar, SIGNAL(valueChanged(int)), SLOT(dstScrollBarValueChanged(int))); 
  
  connect(routeList, SIGNAL(itemSelectionChanged()), SLOT(routeSelectionChanged()));
  connect(newSrcList, SIGNAL(itemSelectionChanged()), SLOT(srcSelectionChanged()));
  connect(newDstList, SIGNAL(itemSelectionChanged()), SLOT(dstSelectionChanged()));
  //connect(newSrcList->verticalScrollBar(), SIGNAL(sliderMoved(int)), connectionsWidget, SLOT(update()));
  //connect(newDstList->verticalScrollBar(), SIGNAL(sliderMoved(int)), connectionsWidget, SLOT(update()));
  connect(newSrcList->verticalScrollBar(), SIGNAL(valueChanged(int)), connectionsWidget, SLOT(update()));
  connect(newDstList->verticalScrollBar(), SIGNAL(valueChanged(int)), connectionsWidget, SLOT(update()));
  connect(newSrcList, SIGNAL(itemCollapsed(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(newSrcList, SIGNAL(itemExpanded(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(newDstList, SIGNAL(itemCollapsed(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(newDstList, SIGNAL(itemExpanded(QTreeWidgetItem*)), connectionsWidget, SLOT(update()));
  connect(connectionsWidget, SIGNAL(scrollBy(int, int)), newSrcList, SLOT(scrollBy(int, int)));
  connect(connectionsWidget, SIGNAL(scrollBy(int, int)), newDstList, SLOT(scrollBy(int, int)));
  connect(removeButton, SIGNAL(clicked()), SLOT(disconnectClicked()));
  connect(connectButton, SIGNAL(clicked()), SLOT(connectClicked()));
  connect(allMidiPortsButton, SIGNAL(clicked(bool)), SLOT(allMidiPortsClicked(bool)));
  connect(filterSrcButton, SIGNAL(clicked(bool)), SLOT(filterSrcClicked(bool)));
  connect(filterDstButton, SIGNAL(clicked(bool)), SLOT(filterDstClicked(bool)));
  connect(srcRoutesButton, SIGNAL(clicked(bool)), SLOT(filterSrcRoutesClicked(bool)));
  connect(dstRoutesButton, SIGNAL(clicked(bool)), SLOT(filterDstRoutesClicked(bool)));
  connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged(MusECore::SongChangedFlags_t)));
}

void RouteDialog::srcTreeScrollValueChanged(int value)
{
  // Prevent recursion
  srcTreeScrollBar->blockSignals(true);
  srcTreeScrollBar->setValue(value);
  srcTreeScrollBar->blockSignals(false);
}

void RouteDialog::dstTreeScrollValueChanged(int value)
{
  // Prevent recursion
  dstTreeScrollBar->blockSignals(true);
  dstTreeScrollBar->setValue(value);
  dstTreeScrollBar->blockSignals(false);
}

void RouteDialog::srcScrollBarValueChanged(int value)
{
  // Prevent recursion
  newSrcList->blockSignals(true);
  newSrcList->verticalScrollBar()->setValue(value);
  newSrcList->blockSignals(false);
}

void RouteDialog::dstScrollBarValueChanged(int value)
{
  // Prevent recursion
  newDstList->blockSignals(true);
  newDstList->verticalScrollBar()->setValue(value);
  newDstList->blockSignals(false);
}

void RouteDialog::allMidiPortsClicked(bool /*v*/)
{
  // TODO: This is a bit brutal and sweeping... Refine this down to needed parts only.
  routingChanged();  
}

void RouteDialog::filterSrcClicked(bool v)
{
//   if(v)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(newSrcList->currentItem());
//     filter(item, false);
//   }
//   else
//     filter(NULL, false);
  
  
  //if(v)
  //  _srcFilterItems = newSrcList->selectedItems();  
  //else
  //  _srcFilterItems.clear();
  
  if(dstRoutesButton->isChecked())
  {
    dstRoutesButton->blockSignals(true);
    dstRoutesButton->setChecked(false);
    dstRoutesButton->blockSignals(false);
  }
  filter(v ? newSrcList->selectedItems() : RouteTreeItemList(), RouteTreeItemList(), true, false);
//   if(v)
//   {
//     //if(dstRoutesButton->isEnabled())
//     //  dstRoutesButton->setEnabled(false);
//     filter(newSrcList->selectedItems(), RouteTreeItemList(), true, false);
//   }
//   else
//   {
//     //if(!dstRoutesButton->isEnabled())
//     //  dstRoutesButton->setEnabled(true);
//     filter(RouteTreeItemList(), RouteTreeItemList(), true, false);
//   }
}

void RouteDialog::filterDstClicked(bool v)
{
//   if(v)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(newDstList->currentItem());
//     filter(item, true);
//   }
//   else
//     filter(NULL, true);
  
//   if(v)
//     _dstFilterItems = newDstList->selectedItems();  
//   else
//     _dstFilterItems.clear();
//   
//   filter();

  if(srcRoutesButton->isChecked())
  {
    srcRoutesButton->blockSignals(true);
    srcRoutesButton->setChecked(false);
    srcRoutesButton->blockSignals(false);
  }
 filter(RouteTreeItemList(), v ? newDstList->selectedItems() : RouteTreeItemList(), false, true);
//   if(v)
//   {
//     //if(srcRoutesButton->isEnabled())
//     //  srcRoutesButton->setEnabled(false);
//     filter(RouteTreeItemList(), newDstList->selectedItems(), false, true);
//   }
//   else
//   {
//     //if(!srcRoutesButton->isEnabled())
//     //  srcRoutesButton->setEnabled(true);
//     filter(RouteTreeItemList(), RouteTreeItemList(), false, true);
//   }
}

void RouteDialog::filterSrcRoutesClicked(bool /*v*/)
{
//   if(v)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(newSrcList->currentItem());
//     filter(item, false);
//   }
//   else
//     filter(NULL, false);
  
  
//   if(v)
//     _srcFilterItems = newSrcList->selectedItems();  
//   else
//     _srcFilterItems.clear();
//   
//   filter();
  
  if(dstRoutesButton->isChecked())
  {
    dstRoutesButton->blockSignals(true);
    dstRoutesButton->setChecked(false);
    dstRoutesButton->blockSignals(false);
  }
  if(filterDstButton->isChecked())
  {
    filterDstButton->blockSignals(true);
    filterDstButton->setChecked(false);
    filterDstButton->blockSignals(false);
  }
  // Unfilter the entire destination list, while (un)filtering with the 'show only possible routes' part.
  filter(RouteTreeItemList(), RouteTreeItemList(), false, true);
//   if(v)
//   {
//     //if(filterSrcButton->isEnabled())
//     //  filterSrcButton->setEnabled(false);
//     filter(RouteTreeItemList(), newDstList->selectedItems(), false, true);
//   }
//   else
//   {
//     //if(!filterSrcButton->isEnabled())
//     //  filterSrcButton->setEnabled(true);
//     filter(RouteTreeItemList(), RouteTreeItemList(), false, true);
//   }
}

void RouteDialog::filterDstRoutesClicked(bool /*v*/)
{
//   if(v)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(newDstList->currentItem());
//     filter(item, true);
//   }
//   else
//     filter(NULL, true);
  
//   if(v)
//     _dstFilterItems = newDstList->selectedItems();  
//   else
//     _dstFilterItems.clear();
//   
//   filter();
//   filter(RouteTreeItemList(), 
//          filterDstButton->isChecked() ? newDstList->selectedItems() : RouteTreeItemList(),
//          false, true);
  
  if(srcRoutesButton->isChecked())
  {
    srcRoutesButton->blockSignals(true);
    srcRoutesButton->setChecked(false);
    srcRoutesButton->blockSignals(false);
  }
  if(filterSrcButton->isChecked())
  {
    filterSrcButton->blockSignals(true);
    filterSrcButton->setChecked(false);
    filterSrcButton->blockSignals(false);
  }
  // Unfilter the entire source list, while (un)filtering with the 'show only possible routes' part.
  filter(RouteTreeItemList(), RouteTreeItemList(), true, false);
}

void RouteDialog::filter(const RouteTreeItemList& srcFilterItems, 
                         const RouteTreeItemList& dstFilterItems,
                         bool filterSrc, 
                         bool filterDst)
{
  bool src_changed = false;
  bool dst_changed = false;
  const RouteTreeItemList src_sel_items = newSrcList->selectedItems();
  const RouteTreeItemList dst_sel_items = newDstList->selectedItems();
  bool hide;

  QTreeWidgetItemIterator iSrcTree(newSrcList);
  while(*iSrcTree)
  {
    QTreeWidgetItem* item = *iSrcTree;
    hide = item->isHidden();

    if(filterSrc)
    {
      hide = false;
      if(!srcFilterItems.isEmpty())
      {
        RouteTreeItemList::const_iterator ciFilterItems = srcFilterItems.cbegin();
        for( ; ciFilterItems != srcFilterItems.cend(); ++ciFilterItems)
        {
          QTreeWidgetItem* flt_item = *ciFilterItems;
          QTreeWidgetItem* twi = flt_item;
          while(twi != item && (twi = twi->parent()))  ;
          
          if(twi == item)
            break;
          QTreeWidgetItem* twi_p = item;
          while(twi_p != flt_item && (twi_p = twi_p->parent()))  ;

          if(twi_p == flt_item)
            break;
        }
        hide = ciFilterItems == srcFilterItems.cend();
      }
    }
    //else
    
    if(!filterSrc || srcFilterItems.isEmpty())
    {
      hide = false;
      //// Is the item slated to hide? Check finally the 'show only possible routes' settings...
      //if(hide && dstRoutesButton->isChecked())
      // Check finally the 'show only possible routes' settings...
      if(dstRoutesButton->isChecked())
      {
        switch(item->type())
        {
          case RouteTreeWidgetItem::NormalItem:
          case RouteTreeWidgetItem::CategoryItem:
            hide = true;
          break;
          
          case RouteTreeWidgetItem::RouteItem:
          case RouteTreeWidgetItem::ChannelsItem:
          {
            RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);
            RouteTreeItemList::const_iterator iSelItems = dst_sel_items.cbegin();
            for( ; iSelItems != dst_sel_items.cend(); ++iSelItems)
            {
              RouteTreeWidgetItem* sel_dst_item = static_cast<RouteTreeWidgetItem*>(*iSelItems);
              //if(sel_dst_item->type() == item->type() && MusECore::routesCompatible(rtwi->route(), sel_dst_item->route(), false))
              if(MusECore::routesCompatible(rtwi->route(), sel_dst_item->route(), true))
              {
                //hide = false;
                // Hm, Qt doesn't seem to do this for us:
                QTreeWidgetItem* twi = item;
                while((twi = twi->parent()))
                {
                  //if(twi->isHidden() != hide)
                  if(twi->isHidden())
                  {
                    //twi->setHidden(hide);
                    twi->setHidden(false);
                    src_changed = true;
                  }
                }
                break;
              }
            }
            hide = iSelItems == dst_sel_items.cend();
          }
          break;
        }
      }
    }
    
    if(item->isHidden() != hide)
    {
      item->setHidden(hide);
      src_changed = true;
    }
    
    ++iSrcTree;
  }

  
  QTreeWidgetItemIterator iDstTree(newDstList);
  while(*iDstTree)
  {
    QTreeWidgetItem* item = *iDstTree;
    hide = item->isHidden();
    
    if(filterDst)
    {
      hide = false;
      if(!dstFilterItems.isEmpty())
      {
        RouteTreeItemList::const_iterator ciFilterItems = dstFilterItems.cbegin();
        for( ; ciFilterItems != dstFilterItems.cend(); ++ciFilterItems)
        {
          QTreeWidgetItem* flt_item = *ciFilterItems;
          QTreeWidgetItem* twi = flt_item;
          while(twi != item && (twi = twi->parent()))  ;
          
          if(twi == item)
            break;
          QTreeWidgetItem* twi_p = item;
          while(twi_p != flt_item && (twi_p = twi_p->parent()))  ;
          
          if(twi_p == flt_item)
            break;
        }
        hide = ciFilterItems == dstFilterItems.cend();
      }
    }
    //else
    
    if(!filterDst || dstFilterItems.isEmpty())
    {
      hide = false;
      //// Is the item slated to hide? Check finally the 'show only possible routes' settings...
      //if(hide && srcRoutesButton->isChecked())
      // Check finally the 'show only possible routes' settings...
      if(srcRoutesButton->isChecked())
      {
        switch(item->type())
        {
          case RouteTreeWidgetItem::NormalItem:
          case RouteTreeWidgetItem::CategoryItem:
            hide = true;
          break;
          
          case RouteTreeWidgetItem::RouteItem:
          case RouteTreeWidgetItem::ChannelsItem:
          {
            RouteTreeWidgetItem* rtwi = static_cast<RouteTreeWidgetItem*>(item);
            RouteTreeItemList::const_iterator iSelItems = src_sel_items.cbegin();
            for( ; iSelItems != src_sel_items.cend(); ++iSelItems)
            {
              RouteTreeWidgetItem* sel_src_item = static_cast<RouteTreeWidgetItem*>(*iSelItems);
              //if(sel_src_item->type() == item->type() && MusECore::routesCompatible(sel_src_item->route(), rtwi->route(), false))
              if(MusECore::routesCompatible(sel_src_item->route(), rtwi->route(), true))
              {
                //hide = false;
                // Hm, Qt doesn't seem to do this for us:
                QTreeWidgetItem* twi = item;
                while((twi = twi->parent()))
                {
                  //if(twi->isHidden() != hide)
                  if(twi->isHidden())
                  {
                    //twi->setHidden(hide);
                    twi->setHidden(false);
                    dst_changed = true;
                  }
                }
                break;
              }
            }
            hide = iSelItems == src_sel_items.cend();
          }
          break;
        }
      }
    }
    
    if(item->isHidden() != hide)
    {
      item->setHidden(hide);
      dst_changed = true;
    }
    
    ++iDstTree;
  }
  
  
  
  // Update the connecting lines.
  if(src_changed)
  {
    QTreeWidgetItemIterator iSrcList(newSrcList);
    while(*iSrcList)
    {
      RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*iSrcList);
      item->computeChannelYValues();
      ++iSrcList;
    }
  }
  if(dst_changed)
  {
    QTreeWidgetItemIterator iDstList(newDstList);
    while(*iDstList)
    {
      RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*iDstList);
      item->computeChannelYValues();
      ++iDstList;
    }
  }
  
  if(src_changed || dst_changed)  
    connectionsWidget->update();  // Redraw the connections.
  
  QTreeWidgetItemIterator iRouteTree(routeList);
//   //RouteTreeItemList::iterator iFilterItems;
//   //filterItems::iterator iFilterItems;
//   
  while(*iRouteTree)
  {
    QTreeWidgetItem* item = *iRouteTree;
    //if(item->data(isDestination ? RouteDialog::ROUTE_DST_COL : RouteDialog::ROUTE_SRC_COL, 
    //              RouteDialog::RouteRole).canConvert<MusECore::Route>())
    if(item && item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
    {        
      //const MusECore::Route r = item->data(isDestination ? RouteDialog::ROUTE_DST_COL : RouteDialog::ROUTE_SRC_COL, 
      //                                     RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      
//       bool hide = false;
//       
//       //if(_srcFilterItems.isEmpty())
//       //  item->setHidden(false);
//       //else
//       //{
//         RouteTreeItemList::const_iterator ciFilterItems = _srcFilterItems.begin();
//         for( ; ciFilterItems != _srcFilterItems.end(); ++ciFilterItems)
//           if(src.compare(static_cast<RouteTreeWidgetItem*>(*ciFilterItems)->route()))  // FIXME Ugly
//             break;
//         if(ciFilterItems == _srcFilterItems.end())
//           hide = true;
//         item->setHidden(ciFilterItems == filterItems.end());
//       //}
        
      RouteTreeWidgetItem* src_item = newSrcList->findItem(src);
      RouteTreeWidgetItem* dst_item = newDstList->findItem(dst);
      item->setHidden((src_item && src_item->isHidden()) || (dst_item && dst_item->isHidden()));
    }
    ++iRouteTree;
  }
  
  //routingChanged();
}


//---------------------------------------------------------
//   routingChanged
//---------------------------------------------------------

void RouteDialog::routingChanged()
{
  // Refill the lists of available external ports.
  tmpJackOutPorts = MusEGlobal::audioDevice->outputPorts();
  tmpJackInPorts = MusEGlobal::audioDevice->inputPorts();
  tmpJackMidiOutPorts = MusEGlobal::audioDevice->outputPorts(true);
  tmpJackMidiInPorts = MusEGlobal::audioDevice->inputPorts(true);
  removeItems();                // Remove unused items.
  addItems();                   // Add any new items.
  newSrcList->resizeColumnToContents(ROUTE_NAME_COL);
  newDstList->resizeColumnToContents(ROUTE_NAME_COL);
  routeList->resizeColumnToContents(ROUTE_SRC_COL);
  routeList->resizeColumnToContents(ROUTE_DST_COL);
  
  // Now that column resizing is done, update all channel y values in source and destination lists.
  // Must be done here because it relies on the column width.
  QTreeWidgetItemIterator iDstList(newDstList);
  while(*iDstList)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*iDstList);
    item->computeChannelYValues();
    ++iDstList;
  }
  QTreeWidgetItemIterator iSrcList(newSrcList);
  while(*iSrcList)
  {
    RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(*iSrcList);
    item->computeChannelYValues();
    ++iSrcList;
  }
  
  routeSelectionChanged();      // Init remove button.
  srcSelectionChanged();        // Init select button.
  connectionsWidget->update();  // Redraw the connections.
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void RouteDialog::songChanged(MusECore::SongChangedFlags_t v)
{
  if (v & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_MIDI_TRACK_PROP | SC_ROUTE | SC_CONFIG)) {
        routingChanged();
        }
}

//---------------------------------------------------------
//   routeSelectionChanged
//---------------------------------------------------------

void RouteDialog::routeSelectionChanged()
{
  QTreeWidgetItem* item = routeList->currentItem();
  if(item == 0)
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }
  if(!item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() || !item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }
  const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  RouteTreeWidgetItem* srcItem = newSrcList->findItem(src);
  RouteTreeWidgetItem* dstItem = newDstList->findItem(dst);
  newSrcList->blockSignals(true);
  newSrcList->setCurrentItem(srcItem);
  newSrcList->blockSignals(false);
  newDstList->blockSignals(true);
  newDstList->setCurrentItem(dstItem);
  newDstList->blockSignals(false);
  if(srcItem)
    newSrcList->scrollToItem(srcItem, QAbstractItemView::PositionAtCenter);
  if(dstItem)
    newDstList->scrollToItem(dstItem, QAbstractItemView::PositionAtCenter);
  connectionsWidget->update();
//   connectButton->setEnabled(MusECore::routeCanConnect(src, dst));
  connectButton->setEnabled(false);
//   removeButton->setEnabled(MusECore::routeCanDisconnect(src, dst));
//   removeButton->setEnabled(true);
  
  
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
                removeButton->setEnabled(mt->outPort() != dst.midiPort || mt->outChannel() != src.channel);
                return;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif
  
  removeButton->setEnabled(true);
  
}

//---------------------------------------------------------
//   disconnectClicked
//---------------------------------------------------------

void RouteDialog::disconnectClicked()
{
//   QTreeWidgetItem* item = routeList->currentItem();
//   if(item && item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//   {
//     const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     MusEGlobal::audio->msgRemoveRoute(src, dst);
//     MusEGlobal::audio->msgUpdateSoloStates();
//     MusEGlobal::song->update(SC_SOLO);
//   }
//   routingChanged();
// 
//   
//   const int cnt = routeList->topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     QTreeWidgetItem* item = routeList->topLevelItem(i);
//     if(!item || !item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() || !item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//       continue;
//     if(item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>() == src && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>() == dst)
//       return item;
//   }
  
  
  MusECore::PendingOperationList operations;
  QTreeWidgetItemIterator ii(routeList);
  while(*ii)
  {
    QTreeWidgetItem* item = *ii;
    if(item && item->isSelected() &&
       item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && 
       item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
    {
      const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      //if(MusECore::routeCanDisconnect(src, dst))
//         operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
        
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
//                 MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
//                 if(mt->outPort() != dst.midiPort || mt->outChannel() != src.channel)
//                   operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
                ++ii;
                continue;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif
        
      operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::DeleteRoute));
        
    }      
    ++ii;
  }
  
  if(!operations.empty())
  {
    operations.add(MusECore::PendingOperationItem((MusECore::TrackList*)NULL, MusECore::PendingOperationItem::UpdateSoloStates));
    MusEGlobal::audio->msgExecutePendingOperations(operations);
    //MusEGlobal::audio->msgUpdateSoloStates(); // TODO Include this in operations ?
    MusEGlobal::song->update(SC_ROUTE);
    //MusEGlobal::song->update(SC_SOLO);
    //routingChanged();
  }
  
  
//   QTreeWidgetItem* srcItem = newSrcList->currentItem();
//   QTreeWidgetItem* dstItem = newDstList->currentItem();
//   if(srcItem == 0 || dstItem == 0)
//     return;
//   if(srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//   {        
//     MusECore::Route src = srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     MusECore::Route dst = dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     MusEGlobal::audio->msgRemoveRoute(src, dst);
//     MusEGlobal::audio->msgUpdateSoloStates();
//     MusEGlobal::song->update(SC_SOLO);
//   }
//   routingChanged();
}

//---------------------------------------------------------
//   connectClicked
//---------------------------------------------------------

void RouteDialog::connectClicked()
{
//   RouteTreeWidgetItem* srcItem = static_cast<RouteTreeWidgetItem*>(newSrcList->currentItem());
//   RouteTreeWidgetItem* dstItem = static_cast<RouteTreeWidgetItem*>(newDstList->currentItem());
//   if(srcItem == 0 || dstItem == 0)
//     return;
//   
//   const MusECore::Route src = srcItem->route();
//   const MusECore::Route dst = dstItem->route();
//   MusEGlobal::audio->msgAddRoute(src, dst);
//   MusEGlobal::audio->msgUpdateSoloStates();
//   MusEGlobal::song->update(SC_SOLO);
//   routingChanged();
  
  MusECore::PendingOperationList operations;
  MusECore::RouteList srcList;
  MusECore::RouteList dstList;
  newSrcList->getSelectedRoutes(srcList);
  newDstList->getSelectedRoutes(dstList);
  const int srcSelSz = srcList.size();
  const int dstSelSz = dstList.size();
  bool upd_trk_props = false;

  for(int srcIdx = 0; srcIdx < srcSelSz; ++srcIdx)
  {
    const MusECore::Route& src = srcList.at(srcIdx);
    for(int dstIdx = 0; dstIdx < dstSelSz; ++dstIdx)
    {
      const MusECore::Route& dst = dstList.at(dstIdx);
      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
                if(src.channel >= 0 && src.channel < MIDI_CHANNELS && (mt->outPort() != dst.midiPort || mt->outChannel() != src.channel))
                {
                  MusEGlobal::audio->msgIdle(true);
                  mt->setOutPortAndChannelAndUpdate(dst.midiPort, src.channel);
                  MusEGlobal::audio->msgIdle(false);
                  //MusEGlobal::audio->msgUpdateSoloStates();
                  //MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
                  upd_trk_props = true;
                }
                continue;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif
      
      if(MusECore::routeCanConnect(src, dst))
        operations.add(MusECore::PendingOperationItem(src, dst, MusECore::PendingOperationItem::AddRoute));
    }
  }

  if(!operations.empty())
  {
    operations.add(MusECore::PendingOperationItem((MusECore::TrackList*)NULL, MusECore::PendingOperationItem::UpdateSoloStates));
    MusEGlobal::audio->msgExecutePendingOperations(operations);
    //MusEGlobal::audio->msgUpdateSoloStates(); // TODO Include this in operations ?
    MusEGlobal::song->update(SC_ROUTE | (upd_trk_props ? SC_MIDI_TRACK_PROP : 0));
    //MusEGlobal::song->update(SC_SOLO);
    //routingChanged();
  }
  else if(upd_trk_props)
    MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
    
}  
//   if(srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//   {    
//     const MusECore::Route src = srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     const MusECore::Route dst = dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     MusEGlobal::audio->msgAddRoute(src, dst);
//     MusEGlobal::audio->msgUpdateSoloStates();
//     MusEGlobal::song->update(SC_SOLO);
//   }
//   routingChanged();
// }

//---------------------------------------------------------
//   srcSelectionChanged
//---------------------------------------------------------

void RouteDialog::srcSelectionChanged()
{
  fprintf(stderr, "RouteDialog::srcSelectionChanged\n");  // REMOVE Tim.

  MusECore::RouteList srcList;
  MusECore::RouteList dstList;
  newSrcList->getSelectedRoutes(srcList);
  newDstList->getSelectedRoutes(dstList);
  const int srcSelSz = srcList.size();
  const int dstSelSz = dstList.size();
  //if(srcSelSz == 0 || dstSelSz == 0 || (srcSelSz > 1 && dstSelSz > 1))
  //{
  //  connectButton->setEnabled(false);
  //  removeButton->setEnabled(false);
  //  return;
  //}

  routeList->blockSignals(true);
  routeList->clearSelection();
  bool canConnect = false;
  QTreeWidgetItem* routesItem = 0;
  int routesSelCnt = 0;
  int routesRemoveCnt = 0;
  for(int srcIdx = 0; srcIdx < srcSelSz; ++srcIdx)
  {
    MusECore::Route& src = srcList.at(srcIdx);
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    int mt_to_mp_cnt = 0;
#endif
    for(int dstIdx = 0; dstIdx < dstSelSz; ++dstIdx)
    {
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      bool useMTOutProps = false;
#endif
      MusECore::Route& dst = dstList.at(dstIdx);
      // Special for some item type combos: Before searching, cross-update the routes if necessary.
      // To minimize route copying, here on each iteration we alter each list route, but should be OK.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                dst.channel = src.channel;
                
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
                useMTOutProps = true;
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                if(src.channel >= 0 && src.channel < MIDI_CHANNELS && (mt->outPort() != dst.midiPort || mt->outChannel() != src.channel))
                  ++mt_to_mp_cnt;
#endif
                
              }  
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::JACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: break;
          }
        break;

        case MusECore::Route::MIDI_PORT_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::TRACK_ROUTE: src.channel = dst.channel; break;
            case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::JACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: break;
          }
        break;
        
        case MusECore::Route::JACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: break;
      }
      
      QTreeWidgetItem* ri = findRoutesItem(src, dst);
      if(ri)
      {
        ri->setSelected(true);
        routesItem = ri;
        ++routesSelCnt;
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
        if(!useMTOutProps)
#endif
          ++routesRemoveCnt;
      }
      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
      if(useMTOutProps)
        continue;
#endif
      
      if(MusECore::routeCanConnect(src, dst))
        canConnect = true;
//       if(MusECore::routeCanDisconnect(src, dst))
//         canDisconnect = true;
    }
    
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    // If there was one and only one selected midi port for a midi track, allow (simulated) connection.
    if(mt_to_mp_cnt == 1)
      canConnect = true;
#endif
    
  }
  
  if(routesSelCnt == 0)
    routeList->setCurrentItem(0);
  //routeList->setCurrentItem(routesItem);
  routeList->blockSignals(false);
  if(routesSelCnt == 1)
    routeList->scrollToItem(routesItem, QAbstractItemView::PositionAtCenter);

  connectionsWidget->update();
  //connectButton->setEnabled(can_connect && (srcSelSz == 1 || dstSelSz == 1));
  connectButton->setEnabled(canConnect);
  //removeButton->setEnabled(can_disconnect);
//   removeButton->setEnabled(routesSelCnt > 0);
  removeButton->setEnabled(routesRemoveCnt != 0);

/*  
  RouteTreeItemList srcSel = newSrcList->selectedItems();
  RouteTreeItemList dstSel = newDstList->selectedItems();
  const int srcSelSz = srcSel.size();
  const int dstSelSz = dstSel.size();
  if(srcSelSz == 0 || dstSelSz == 0)
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }

  bool can_connect = false;
  bool can_disconnect = false;
  for(int srcIdx = 0; srcIdx < srcSelSz; ++srcIdx)
  {
    QTreeWidgetItem* srcItem = srcSel.at(srcIdx);
    if(!srcItem)
      continue;
    if(!srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
      continue;
    MusECore::Route src = srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
    if(srcItem->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
    {
      QBitArray ba = srcItem->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          if(src.track && src.track->isMidiTrack())
          {
            int chans = 0;
            const int sz = ba.size();
            for(int i = 0; i < sz; ++i)
            {
              if(i >= MIDI_CHANNELS)
                break;
              if(ba.testBit(i))
                src.channel |= (1 << i);
            }
          }
        break;
        case MusECore::Route::JACK_ROUTE:
        case MusECore::Route::MIDI_DEVICE_ROUTE:
        case MusECore::Route::MIDI_PORT_ROUTE:
        break;
      }
    }
    
    for(int dstIdx = 0; dstIdx < dstSelSz; ++dstIdx)
    {
      QTreeWidgetItem* dstItem = dstSel.at(dstIdx);
      if(!dstItem)
        continue;
      if(!dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
        continue;
      MusECore::Route dst = dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      if(dstItem->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
      {
        QBitArray ba = dstItem->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
      }
      
      
      
    }
    
    
    
  }
  
  
  QTreeWidgetItem* srcItem = newSrcList->currentItem();
  QTreeWidgetItem* dstItem = newDstList->currentItem();
  if(srcItem == 0 || dstItem == 0)
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }
  if(!srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() || !dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
  {
    connectButton->setEnabled(false);
    removeButton->setEnabled(false);
    return;
  }
  //const MusECore::Route src = srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  //const MusECore::Route dst = dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  MusECore::Route src = srcItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  MusECore::Route dst = dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
  if(srcItem->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
  {
    QBitArray ba = srcItem->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
  }
    
//     || !dstItem->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//   {
//     connectButton->setEnabled(false);
//     removeButton->setEnabled(false);
//     return;
//   }
  QTreeWidgetItem* routesItem = findRoutesItem(src, dst);
  routeList->blockSignals(true);
  routeList->setCurrentItem(routesItem);
  routeList->blockSignals(false);
  if(routesItem)
    routeList->scrollToItem(routesItem, QAbstractItemView::PositionAtCenter);
  connectionsWidget->update();
  connectButton->setEnabled(MusECore::routeCanConnect(src, dst));
  removeButton->setEnabled(MusECore::routeCanDisconnect(src, dst));*/
}

//---------------------------------------------------------
//   dstSelectionChanged
//---------------------------------------------------------

void RouteDialog::dstSelectionChanged()
{
  srcSelectionChanged();  
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RouteDialog::closeEvent(QCloseEvent* e)
{
  emit closed();
  e->accept();
}

// RouteTreeWidgetItem* RouteDialog::findSrcItem(const MusECore::Route& src)
// {
//   int cnt = newSrcList->topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(newSrcList->topLevelItem(i));
//     if(item)
//     {
//       if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//       {
//         //if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>() == src)
//         if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>().compare(src))
//           return item;
//       }
// 
//       int c_cnt = item->childCount();
//       for(int j = 0; j < c_cnt; ++j)
//       {
//         RouteTreeWidgetItem* c_item = static_cast<RouteTreeWidgetItem*>(item->child(j));
//         if(c_item)
//         {
//           if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//           {
//             //if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>() == src)
//             if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>().compare(src))
//               return c_item;
//           }
// 
//           int cc_cnt = c_item->childCount();
//           for(int k = 0; k < cc_cnt; ++k)
//           {
//             RouteTreeWidgetItem* cc_item = static_cast<RouteTreeWidgetItem*>(c_item->child(k));
//             if(cc_item)
//             {
//               if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//               {
//                 //if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>() == src)
//                 if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>().compare(src))
//                   return cc_item;
//               }
//             }
//           }
//         }
//       }
//     }
//   }
//   return 0;
// }

// RouteTreeWidgetItem* RouteDialog::findDstItem(const MusECore::Route& dst)
// {
//   int cnt = newDstList->topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(newDstList->topLevelItem(i));
//     if(item)
//     {
//       if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//       {
//         //if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>() == dst)
//         if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>().compare(dst))
//           return item;
//       }
// 
//       int c_cnt = item->childCount();
//       for(int j = 0; j < c_cnt; ++j)
//       {
//         RouteTreeWidgetItem* c_item = static_cast<RouteTreeWidgetItem*>(item->child(j));
//         if(c_item)
//         {
//           if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//           {
//             //if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>() == dst)
//             if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>().compare(dst))
//               return c_item;
//           }
// 
//           int cc_cnt = c_item->childCount();
//           for(int k = 0; k < cc_cnt; ++k)
//           {
//             RouteTreeWidgetItem* cc_item = static_cast<RouteTreeWidgetItem*>(c_item->child(k));
//             if(cc_item)
//             {
//               if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//               {
//                 //if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>() == dst)
//                 if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>().compare(dst))
//                   return cc_item;
//               }
//             }
//           }
//         }
//       }
//     }
//   }
//   return 0;
// }

QTreeWidgetItem* RouteDialog::findRoutesItem(const MusECore::Route& src, const MusECore::Route& dst)
{
  int cnt = routeList->topLevelItemCount(); 
  for(int i = 0; i < cnt; ++i)
  {
    QTreeWidgetItem* item = routeList->topLevelItem(i);
    if(!item || !item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() || !item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
      continue;
    if(item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>() == src && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>() == dst)
      return item;
  }
  return 0;
}
      
// RouteTreeWidgetItem* RouteDialog::findCategoryItem(QTreeWidget* tree, const QString& name)
// {
//   int cnt = tree->topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(tree->topLevelItem(i));
//     if(item && item->text(ROUTE_NAME_COL) == name)
//       return item;
//   }
//   return 0;
// }
      
// void RouteDialog::getSelectedRoutes(QTreeWidget* tree, MusECore::RouteList& routes)
// {
//   //fprintf(stderr, "RouteDialog::getSelectedRoutes\n");  // REMOVE Tim.
//   
//   RouteTreeItemList sel = tree->selectedItems();
//   const int selSz = sel.size();
//   if(selSz == 0)
//     return;
// 
//   for(int idx = 0; idx < selSz; ++idx)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(sel.at(idx));
//     if(!item)
//       continue;
//     if(!item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//       continue;
//     MusECore::Route r = item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>();
//     if(item->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).canConvert<QBitArray>())
//     {
//       QBitArray ba = item->data(ROUTE_NAME_COL, RouteDialog::ChannelsRole).value<QBitArray>();
//       switch(r.type)
//       {
//         case MusECore::Route::TRACK_ROUTE:
//           if(r.track)
//           {
//             const int sz = ba.size();
//             if(r.track->isMidiTrack())
//             {  
//               for(int i = 0; i < sz; ++i)
//               {
//                 if(i >= MIDI_CHANNELS)
//                   break;
//                 if(ba.testBit(i))
//                 {
//                   r.channel = (1 << i);
//                   routes.push_back(r);
//                 }
//               }
//             }
//             else
//             {
//               for(int i = 0; i < sz; ++i)
//               {
//                 if(ba.testBit(i))
//                 {
//                   r.channel = i;
//                   routes.push_back(r);
//                 }
//               }
//             }
//           }
//         break;
//         case MusECore::Route::JACK_ROUTE:
//         case MusECore::Route::MIDI_DEVICE_ROUTE:
//         case MusECore::Route::MIDI_PORT_ROUTE:
//         break;
//       }
//     }
//     else
//       routes.push_back(r);
//   }
// }
      
      
      
      
void RouteDialog::removeItems()
{
  QVector<QTreeWidgetItem*> itemsToDelete;
  
  newSrcList->getItemsToDelete(itemsToDelete);
  newDstList->getItemsToDelete(itemsToDelete);
  getRoutesToDelete(routeList, itemsToDelete);


  newSrcList->blockSignals(true);
  newDstList->blockSignals(true);
  routeList->blockSignals(true);
  
  if(!itemsToDelete.empty())
  {
    int cnt = itemsToDelete.size();
    for(int i = 0; i < cnt; ++i)
      delete itemsToDelete.at(i);
  }
  
  routeList->blockSignals(false);
  newDstList->blockSignals(false);
  newSrcList->blockSignals(false);
  
  //connectionsWidget->update();
}

// void RouteDialog::getItemsToDelete(QTreeWidget* tree, QVector<QTreeWidgetItem*>& items_to_remove)
// {
//   int cnt = tree->topLevelItemCount(); 
//   for(int i = 0; i < cnt; ++i)
//   {
//     RouteTreeWidgetItem* item = static_cast<RouteTreeWidgetItem*>(tree->topLevelItem(i));
//     if(item)
//     {
//       int c_cnt = item->childCount();
//       for(int j = 0; j < c_cnt; ++j)
//       {
//         RouteTreeWidgetItem* c_item = static_cast<RouteTreeWidgetItem*>(item->child(j));
//         if(c_item)
//         {
//           int cc_cnt = c_item->childCount();
//           for(int k = 0; k < cc_cnt; ++k)
//           {
//             RouteTreeWidgetItem* cc_item = static_cast<RouteTreeWidgetItem*>(c_item->child(k));
//             if(cc_item)
//             {
//               if(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//               {
//                 if(!routeNodeExists(cc_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>()))
//                   items_to_remove.append(cc_item);
//               }
//             }
//           }
//           if(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//           {
//             if(!routeNodeExists(c_item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>()))
//               items_to_remove.append(c_item);
//           }
//         }
//       }
//       if(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
//       {
//         if(!routeNodeExists(item->data(ROUTE_NAME_COL, RouteDialog::RouteRole).value<MusECore::Route>()))
//           items_to_remove.append(item);
//       }
//     }
//   }
// }

void RouteDialog::getRoutesToDelete(QTreeWidget* tree, QVector<QTreeWidgetItem*>& items_to_remove)
{
  const int iItemCount = tree->topLevelItemCount();
  for (int iItem = 0; iItem < iItemCount; ++iItem) 
  {
    QTreeWidgetItem *item = tree->topLevelItem(iItem);
    if(item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>() && item->data(ROUTE_DST_COL, RouteDialog::RouteRole).canConvert<MusECore::Route>())
    {        
      const MusECore::Route src = item->data(ROUTE_SRC_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      const MusECore::Route dst = item->data(ROUTE_DST_COL, RouteDialog::RouteRole).value<MusECore::Route>();
      
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
// Special: Allow simulated midi track to midi port route (a route found in our 'local' routelist
//           but not in any track or port routelist) until multiple output routes are allowed
//           instead of just single port and channel properties. The route is exclusive.
      switch(src.type)
      {
        case MusECore::Route::TRACK_ROUTE:
          switch(dst.type)
          {
            case MusECore::Route::MIDI_PORT_ROUTE:
              if(src.track->isMidiTrack())
              {
                MusECore::MidiTrack* mt = static_cast<MusECore::MidiTrack*>(src.track);
                // We cannot 'remove' a simulated midi track output port and channel route.
                // (Midi port cannot be -1 meaning 'no port'.)
                // Only remove it if it's a different port or channel. 
                if(mt->outPort() != dst.midiPort || mt->outChannel() != src.channel)
                  items_to_remove.append(item);
                continue;
              }
            break;
            
            case MusECore::Route::TRACK_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
              break;
          }
        break;
        
        case MusECore::Route::MIDI_PORT_ROUTE: case MusECore::Route::MIDI_DEVICE_ROUTE: case MusECore::Route::JACK_ROUTE:
        break;
      }
#endif

      if(!MusECore::routeCanDisconnect(src, dst))
        items_to_remove.append(item);
    }
  }
}


// bool RouteDialog::routeNodeExists(const MusECore::Route& r)
// {
//   switch(r.type)
//   {
//     case MusECore::Route::TRACK_ROUTE:
//     {
//       MusECore::TrackList* tl = MusEGlobal::song->tracks();
//       for(MusECore::ciTrack i = tl->begin(); i != tl->end(); ++i) 
//       {
//             if((*i)->isMidiTrack())
//               continue;
//             MusECore::AudioTrack* track = (MusECore::AudioTrack*)(*i);
//             if(track->type() == MusECore::Track::AUDIO_INPUT) 
//             {
//               if(r == MusECore::Route(track, -1))
//                 return true;
//               for(int channel = 0; channel < track->channels(); ++channel)
//                 if(r == MusECore::Route(track, channel))
//                   return true;
//               
// //               const MusECore::RouteList* rl = track->inRoutes();
// //               for (MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) {
// //                     //MusECore::Route dst(track->name(), true, r->channel);
// //                     QString src(r->name());
// //                     if(r->channel != -1)
// //                       src += QString(":") + QString::number(r->channel);
// //                     MusECore::Route dst(track->name(), true, r->channel, MusECore::Route::TRACK_ROUTE);
// //                     item = new QTreeWidgetItem(routeList, QStringList() << src << dst.name());
// //                     item->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(*r));
// //                     item->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
// //                     }
//             }
//             else if(track->type() != MusECore::Track::AUDIO_AUX)
//             {
//               if(r == MusECore::Route(track, -1))
//                 return true;
//             }
//             
//             if(track->type() == MusECore::Track::AUDIO_OUTPUT) 
//             {
//               if(r == MusECore::Route(track, -1))
//                 return true;
//               for (int channel = 0; channel < track->channels(); ++channel) 
//                 if(r == MusECore::Route(track, channel))
//                   return true;
//             }
//             else if(r == MusECore::Route(track, -1))
//               return true;
// 
//     //         const MusECore::RouteList* rl = track->outRoutes();
//     //         for (MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
//     //         {
//     //               QString srcName(track->name());
//     //               if (track->type() == MusECore::Track::AUDIO_OUTPUT) {
//     //                     MusECore::Route s(srcName, false, r->channel);
//     //                     srcName = s.name();
//     //                     }
//     //               if(r->channel != -1)
//     //                 srcName += QString(":") + QString::number(r->channel);
//     //               MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
//     //               item = new QTreeWidgetItem(routeList, QStringList() << srcName << r->name());
//     //               item->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
//     //               item->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(*r));
//     //         }
//       }
//     }
//     break;
//     
//     case MusECore::Route::JACK_ROUTE:
//     {
//       if(MusEGlobal::checkAudioDevice())
//       {
//         for(std::list<QString>::iterator i = tmpJackOutPorts.begin(); i != tmpJackOutPorts.end(); ++i)
//           if(r == MusECore::Route(*i, false, -1, MusECore::Route::JACK_ROUTE))
//             return true;
//         for (std::list<QString>::iterator i = tmpJackInPorts.begin(); i != tmpJackInPorts.end(); ++i)
//           if(r == MusECore::Route(*i, true, -1, MusECore::Route::JACK_ROUTE))
//             return true;
//         for(std::list<QString>::iterator i = tmpJackMidiOutPorts.begin(); i != tmpJackMidiOutPorts.end(); ++i)
//           if(r == MusECore::Route(*i, false, -1, MusECore::Route::JACK_ROUTE))
//             return true;
//         for (std::list<QString>::iterator i = tmpJackMidiInPorts.begin(); i != tmpJackMidiInPorts.end(); ++i)
//           if(r == MusECore::Route(*i, true, -1, MusECore::Route::JACK_ROUTE))
//             return true;
//       }
//     }
//     break;
//     
//     case MusECore::Route::MIDI_DEVICE_ROUTE:
//       for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
//       {
//         MusECore::MidiDevice* md = *i;
//         // Synth are tracks and devices. Don't list them as devices here, list them as tracks, above.
//         if(md->deviceType() == MusECore::MidiDevice::SYNTH_MIDI)
//           continue;
//         
//         if(r == MusECore::Route(md, -1))
//           return true;
//         for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
//           if(r == MusECore::Route(md, channel))
//             return true;
//       }
//       
//     case MusECore::Route::MIDI_PORT_ROUTE:
//       break;
//       
//   }
//   return false;
// }

void RouteDialog::addItems()
{
  RouteTreeWidgetItem* srcCatItem;
  RouteTreeWidgetItem* dstCatItem;
  RouteTreeWidgetItem* item;
  RouteTreeWidgetItem* subitem;
  QTreeWidgetItem* routesItem;
  Qt::Alignment align_flags = Qt::AlignLeft | Qt::AlignVCenter;
  
  //
  // Tracks:
  //
  
  dstCatItem = newDstList->findCategoryItem(tracksCat);
  srcCatItem = newSrcList->findCategoryItem(tracksCat);
  MusECore::TrackList* tl = MusEGlobal::song->tracks();
  for(MusECore::ciTrack i = tl->begin(); i != tl->end(); ++i) 
  {
    MusECore::Track* track = *i;
    
//     if((*i)->isMidiTrack())
//     {
//       MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(*i);
//       
//     }
//     else
//     {
//       const MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*i);
      
      //
      // DESTINATION section:
      //
      
      //if(track->type() == MusECore::Track::AUDIO_INPUT) 
      if(track->type() != MusECore::Track::AUDIO_AUX)
      {
        const MusECore::Route r(track, -1);
        item = newDstList->findItem(r, RouteTreeWidgetItem::RouteItem);
        if(item)
        {
          // Update the text.
          item->setText(ROUTE_NAME_COL, track->name());
        }
        else
        {
          if(!dstCatItem)
          {
            newDstList->blockSignals(true);
            //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << tracksCat << QString() );
            dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << tracksCat, RouteTreeWidgetItem::CategoryItem, false);
            dstCatItem->setFlags(Qt::ItemIsEnabled);
            QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
            fnt.setBold(true);
            fnt.setItalic(true);
            //fnt.setPointSize(fnt.pointSize() + 2);
            dstCatItem->setFont(ROUTE_NAME_COL, fnt);
            dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
            dstCatItem->setExpanded(true);
            newDstList->blockSignals(false);
          }
          newDstList->blockSignals(true);
          //item = new QTreeWidgetItem(dstCatItem, QStringList() << track->name() << trackLabel );
          item = new RouteTreeWidgetItem(dstCatItem, QStringList() << track->name(), RouteTreeWidgetItem::RouteItem, false, r);
          //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
          item->setTextAlignment(ROUTE_NAME_COL, align_flags);
          newDstList->blockSignals(false);
          //dstCatItem->setExpanded(true); // REMOVE Tim. For test only.
        }
        
        // NOTE: Keep for later if needed.
        if(track->isMidiTrack())
        {
          //for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
          //{
            //const MusECore::Route sub_r(track, channel, 1);
//             const MusECore::Route sub_r(track, 0);
            //const MusECore::Route sub_r(track, 0, 1);
            const MusECore::Route sub_r(track, 0);
            subitem = newDstList->findItem(sub_r, RouteTreeWidgetItem::ChannelsItem);
//             if(subitem)
//             {
//               // Update the channel y values.
//               //subitem->computeChannelYValues();
//             }
//             else
            if(!subitem)
            {
              newDstList->blockSignals(true);
              item->setExpanded(true);
              //subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) << QString() );
              //subitem = new QTreeWidgetItem(item, QStringList() << QString() << QString() );
              subitem = new RouteTreeWidgetItem(item, QStringList() << QString(), RouteTreeWidgetItem::ChannelsItem, false, sub_r);
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(sub_r));
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::ChannelsRole, QVariant::fromValue(QBitArray(MIDI_CHANNELS)));
              subitem->setTextAlignment(ROUTE_NAME_COL, align_flags);
              newDstList->blockSignals(false);
            }
            // Update the channel y values.
            //subitem->computeChannelYValues();
          //}
        }
        else
        {
          MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(track);
          const int chans = atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH ? atrack->totalInChannels() : atrack->channels();
          //for(int channel = 0; channel < chans; ++channel)
          if(chans != 0)
          {
            //const MusECore::Route sub_r(track, channel, 1);
            const MusECore::Route sub_r(track, 0, 1);
            subitem = newDstList->findItem(sub_r, RouteTreeWidgetItem::ChannelsItem);
//             if(subitem)
//             {
//               // Update the channel y values.
//               subitem->computeChannelYValues();
//             }
//             else
            if(!subitem)
            {
              newDstList->blockSignals(true);
              item->setExpanded(true);
              //subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) << QString() );
              //subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) );
              subitem = new RouteTreeWidgetItem(item, QStringList() << QString(), RouteTreeWidgetItem::ChannelsItem, false, sub_r);
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(sub_r));
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::ChannelsRole, QVariant::fromValue(QBitArray(chans)));
              subitem->setTextAlignment(ROUTE_NAME_COL, align_flags);
              newDstList->blockSignals(false);
            }
            // Update the channel y values.
            //subitem->computeChannelYValues();
          }
        }
      }

      const MusECore::RouteList* irl = track->inRoutes();
      for(MusECore::ciRoute r = irl->begin(); r != irl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After: [src  TrackA, Channel  4, Remote Channel  2]  dst: TrackB channel  2 Remote Channel  4
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        MusECore::Route src;
        MusECore::Route dst;
        QString srcName;
        QString dstName = track->name();
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE: 
            //src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, r->channels, -1, r->persistentJackPortName);
            //src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, -1, -1, r->persistentJackPortName);
            src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            //src = *r;
            //dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       r->channels, -1, 0);
            dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
            srcName = r->name();
          break;  
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
            continue;
          break;  
          // Midi ports taken care of below...
          case MusECore::Route::MIDI_PORT_ROUTE: 
            continue;
          break;  
            
          /*{
            //continue;  // TODO
//               //src = MusECore::Route(MusECore::Route::MIDI_PORT_ROUTE,  r->midiPort, 0, r->channel, -1, -1, 0);
//             MusECore::MidiDevice* md = MusEGlobal::midiPorts[r->midiPort].device();
            if(r->channel == -1)
            {
//               if(md)
//                 src = MusECore::Route(md);
//               else
                src = MusECore::Route(r->midiPort);
              dst = MusECore::Route(track);
              srcName = r->name();
              break;
            }
            
//             for(int i = 0; i < MIDI_CHANNELS; ++i)
            {
//               int chbits = 1 << i;
//               if(r->channel & chbits)
              {
                //src = MusECore::Route(r->midiPort, r->channel);
                //src = MusECore::Route(r->midiPort, 1 << i);
//                 if(md)
//                   //src = MusECore::Route(md, chbits);
//                   src = MusECore::Route(md);
//                   //src = MusECore::Route(md, r->channel);
//                 else
                  //src = MusECore::Route(r->midiPort, chbits);
                  //src = MusECore::Route(r->midiPort);
                  src = MusECore::Route(r->midiPort, r->channel);
  //               //dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
                //dst = MusECore::Route(track, r->channel, 1);
                dst = MusECore::Route(track, r->channel);
//                 dst = MusECore::Route(track, chbits);
                //dst = MusECore::Route(track, i);
                srcName = r->name();
                //if(src.channel != -1)
                //  srcName += QString(" [") + QString::number(i + 1) + QString("]");
//                 dstName = track->name() + QString(" [") + QString::number(i + 1) + QString("]");
                dstName = track->name() + QString(" [") + QString::number(r->channel + 1) + QString("]");
                routesItem = findRoutesItem(src, dst);
                if(routesItem)
                {
                  // Update the text.
                  routesItem->setText(ROUTE_SRC_COL, srcName);
                  routesItem->setText(ROUTE_DST_COL, dstName);
                }
                else
                {
                  routeList->blockSignals(true);
                  routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
                  routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
                  routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
                  routeList->blockSignals(false);
                }
              }
            }
            continue;
          }
          break;  */
          
          case MusECore::Route::TRACK_ROUTE: 
            src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, r->track, r->remoteChannel, r->channels, -1, 0);
            dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,    r->channel,       r->channels, -1, 0);
            srcName = r->name();
          break;  
        }

        if(src.channel != -1)
          srcName += QString(" [") + QString::number(src.channel) + QString("]");
        if(dst.channel != -1)
          dstName += QString(" [") + QString::number(dst.channel) + QString("]");
  


//         QString srcName(r->name());
//         if(r->channel != -1)
//           srcName += QString(":") + QString::number(r->channel);
//         
//         
//         MusECore::Route src(*r);
//         if(src.type == MusECore::Route::JACK_ROUTE)
//           src.channel = -1;
//         //const MusECore::Route dst(track->name(), true, r->channel, MusECore::Route::TRACK_ROUTE);
//         const MusECore::Route dst(MusECore::Route::TRACK_ROUTE, -1, track, r->remoteChannel, r->channels, r->channel, 0);
//         
//         
//         src.remoteChannel = src.channel;
//         dst.remoteChannel = dst.channel;
//         const int src_chan = src.channel;
//         src.channel = dst.channel;
//         dst.channel = src_chan;
        
        
        routesItem = findRoutesItem(src, dst);
        if(routesItem)
        {
          // Update the text.
          routesItem->setText(ROUTE_SRC_COL, srcName);
          routesItem->setText(ROUTE_DST_COL, dstName);
        }
        else
        {
          routeList->blockSignals(true);
          routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
          routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
          routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
          routeList->blockSignals(false);
        }
      }
//       else if(track->type() != MusECore::Track::AUDIO_AUX)
//       {
//         const MusECore::Route r(track, -1);
//         item = findDstItem(r);
//         if(item)
//         {
//           // Update the text.
//           item->setText(ROUTE_NAME_COL, track->name());
//         }
//         else
//         {
//           if(!dstCatItem)
//           {
//             newDstList->blockSignals(true);
//             dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << tracksCat << QString() );
//             //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//             newDstList->blockSignals(false);
//           }
//           newDstList->blockSignals(true);
//           item = new QTreeWidgetItem(dstCatItem, QStringList() << track->name() << trackLabel );
//           item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//           newDstList->blockSignals(false);
//         }
//         //if((*i)->isMidiTrack())        
//         //if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//         {
//           //for(int channel = 0; channel < track->channels(); ++channel)
//           const int chans = track->type() == MusECore::Track::AUDIO_SOFTSYNTH ? track->totalInChannels() : track->channels();
//           for(int channel = 0; channel < chans; ++channel)
//           {
//             const MusECore::Route subr(track, channel, 1);
//             subitem = findDstItem(subr);
//             if(!subitem)
//             {
//               newDstList->blockSignals(true);
//               subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) << QString() );
//               subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(subr));
//               newDstList->blockSignals(false);
//             }
//           }
//         }
//       }
      
      //
      // SOURCE section:
      //
      
      //if(track->type() == MusECore::Track::AUDIO_OUTPUT) 
      //{
        const MusECore::Route r(track, -1);
        item = newSrcList->findItem(r, RouteTreeWidgetItem::RouteItem);
        if(item)
        {
          // Update the text.
          item->setText(ROUTE_NAME_COL, track->name());
        }
        else
        {
          if(!srcCatItem)
          {
            newSrcList->blockSignals(true);
            //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << tracksCat << QString() );
            srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << tracksCat, RouteTreeWidgetItem::CategoryItem, true);
            srcCatItem->setFlags(Qt::ItemIsEnabled);
            QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
            fnt.setBold(true);
            fnt.setItalic(true);
            //fnt.setPointSize(fnt.pointSize() + 2);
            srcCatItem->setFont(ROUTE_NAME_COL, fnt);
            srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
            srcCatItem->setExpanded(true);
            newSrcList->blockSignals(false);
          }
          newSrcList->blockSignals(true);
          //item = new QTreeWidgetItem(srcCatItem, QStringList() << track->name() << trackLabel );
          item = new RouteTreeWidgetItem(srcCatItem, QStringList() << track->name(), RouteTreeWidgetItem::RouteItem, true, r);
          //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
          item->setTextAlignment(ROUTE_NAME_COL, align_flags);
          newSrcList->blockSignals(false);
        }
        
        // NOTE: Keep for later if needed.
        if(track->isMidiTrack())
        {
          //for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
          //{
            //const MusECore::Route sub_r(track, r.channel, 1);
//             const MusECore::Route sub_r(track, 0);
            //const MusECore::Route sub_r(track, 0, 1);
            const MusECore::Route sub_r(track, 0);
            subitem = newSrcList->findItem(sub_r, RouteTreeWidgetItem::ChannelsItem);
//             if(subitem)
//             {
//               // Update the channel y values.
//               subitem->computeChannelYValues();
//             }
//             else
            if(!subitem)
            {
              newSrcList->blockSignals(true);
              item->setExpanded(true);
              //subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) << QString() );
              //subitem = new QTreeWidgetItem(item, QStringList() << QString() << QString() );
              
              subitem = new RouteTreeWidgetItem(item, QStringList() << QString(), RouteTreeWidgetItem::ChannelsItem, true, sub_r
#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
              , RouteTreeWidgetItem::ExclusiveMode
#endif                
              );
              
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(sub_r));
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::ChannelsRole, QVariant::fromValue(QBitArray(MIDI_CHANNELS)));
              subitem->setTextAlignment(ROUTE_NAME_COL, align_flags);
              newSrcList->blockSignals(false);
            }
            // Update the channel y values.
            //subitem->computeChannelYValues();
          //}
        }
        else
        {
          MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(track);
          const int chans = atrack->type() == MusECore::Track::AUDIO_SOFTSYNTH ? atrack->totalOutChannels() : atrack->channels();
          //for(int channel = 0; channel < chans; ++channel)
          if(chans != 0)
          {
            //const MusECore::Route src_r(track, channel, 1);
            const MusECore::Route src_r(track, 0, 1);
            subitem = newSrcList->findItem(src_r, RouteTreeWidgetItem::ChannelsItem);
//             if(subitem)
//             {
//               // Update the channel y values.
//               subitem->computeChannelYValues();
//             }
//             else
            if(!subitem)
            {
              newSrcList->blockSignals(true);
              item->setExpanded(true);
              //subitem = new QTreeWidgetItem(item, QStringList() << QString("ch ") + QString::number(channel + 1) << QString() );
              //subitem = new QTreeWidgetItem(item, QStringList() << QString("ch ") + QString::number(channel + 1) );
              subitem = new RouteTreeWidgetItem(item, QStringList() << QString(), RouteTreeWidgetItem::ChannelsItem, true, src_r);
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src_r));
              //subitem->setData(ROUTE_NAME_COL, RouteDialog::ChannelsRole, QVariant::fromValue(QBitArray(chans)));
              subitem->setTextAlignment(ROUTE_NAME_COL, align_flags);
              newSrcList->blockSignals(false);
            }
            // Update the channel y values.
            //subitem->computeChannelYValues();
          }
        }
//       }
//       else
//       {
//         const MusECore::Route r(track, -1);
//         item = findSrcItem(r);
//         if(item)
//         {
//           // Update the text.
//           item->setText(ROUTE_NAME_COL, track->name());
//         }
//         else
//         {
//           if(!srcCatItem)
//           {
//             newSrcList->blockSignals(true);
//             srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << tracksCat << QString() );
//             //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//             newSrcList->blockSignals(false);
//           }
//           newSrcList->blockSignals(true);
//           item = new QTreeWidgetItem(srcCatItem, QStringList() << track->name() << trackLabel );
//           item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//           newSrcList->blockSignals(false);
//         }
//         
//         //if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//         {
//           //for(int channel = 0; channel < track->channels(); ++channel)
//           const int chans = track->type() == MusECore::Track::AUDIO_SOFTSYNTH ? track->totalOutChannels() : track->channels();
//           for(int channel = 0; channel < chans; ++channel)
//           {
//             const MusECore::Route subr(track, channel, 1);
//             subitem = findSrcItem(subr);
//             if(!subitem)
//             {
//               newSrcList->blockSignals(true);
//               subitem = new QTreeWidgetItem(item, QStringList() << QString("ch ") + QString::number(channel + 1) << QString() );
//               subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(subr));
//               newSrcList->blockSignals(false);
//             }
//           }
//         }
//       }

      const MusECore::RouteList* orl = track->outRoutes();
      for(MusECore::ciRoute r = orl->begin(); r != orl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src: TrackA, Channel  4, Remote Channel  2  [dst: TrackB channel  2 Remote Channel  4]
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src: TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        MusECore::Route src;
        MusECore::Route dst;
        QString srcName = track->name();
        QString dstName;
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE: 
            //src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       r->channels, -1, 0);
            src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
            //dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, r->channels, -1, r->persistentJackPortName);
            //dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, -1, -1, r->persistentJackPortName);
            dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            //dst = *r;
            dstName = r->name();
          break;  
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
            continue;  // TODO
          break;  
          // Midi ports taken care of below...
          case MusECore::Route::MIDI_PORT_ROUTE: 
            continue;
          break;  
          
          /*{
            //continue;  // TODO
               //src = MusECore::Route(r->midiPort, r->channel);
//             MusECore::MidiDevice* md = MusEGlobal::midiPorts[r->midiPort].device();
            if(r->channel == -1)
            {
              src = MusECore::Route(track);
//               if(md)
//                 dst = MusECore::Route(md);
//               else
                dst = MusECore::Route(r->midiPort);
              dstName = r->name();
              break;
            }
            
//             for(int i = 0; i < MIDI_CHANNELS; ++i)
            {
//               int chbits = 1 << i;
//               if(r->channel & chbits)
              {
                src = MusECore::Route(track, r->channel);
//                 src = MusECore::Route(track, chbits);
                //src = MusECore::Route(track, i);
  //               //dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
                //dst = MusECore::Route(track, r->channel, 1);
                //dst = MusECore::Route(track, r->channel);
                //dst = MusECore::Route(r->midiPort, r->channel);
                //dst = MusECore::Route(r->midiPort, 1 << i);
//                 if(md)
//                   //dst = MusECore::Route(md, chbits);
//                   dst = MusECore::Route(md);
//                   //dst = MusECore::Route(md, r->channel);
//                 else
                  //dst = MusECore::Route(r->midiPort, chbits);
                  //dst = MusECore::Route(r->midiPort);
                  dst = MusECore::Route(r->midiPort, r->channel);
//                 srcName = track->name() + QString(" [") + QString::number(i + 1) + QString("]");
                srcName = track->name() + QString(" [") + QString::number(r->channel + 1) + QString("]");
                dstName = r->name();
                //if(dst.channel != -1)
                //  dstName += QString(" [") + QString::number(i + 1) + QString("]");
                routesItem = findRoutesItem(src, dst);
                if(routesItem)
                {
                  // Update the text.
                  routesItem->setText(ROUTE_SRC_COL, srcName);
                  routesItem->setText(ROUTE_DST_COL, dstName);
                }
                else
                {
                  routeList->blockSignals(true);
                  routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
                  routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
                  routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
                  routeList->blockSignals(false);
                }
              }
            }
            continue;  
          }
          break;*/
          
          case MusECore::Route::TRACK_ROUTE: 
            src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track, r->channel, r->channels, -1, 0);
            dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, r->track, r->remoteChannel, r->channels, -1, 0);
            dstName = r->name();
          break;  
        }

        if(src.channel != -1)
          srcName += QString(" [") + QString::number(src.channel) + QString("]");
        if(dst.channel != -1)
          dstName += QString(" [") + QString::number(dst.channel) + QString("]");
        
        
        
        
        //QString srcName(track->name());
        //if(track->type() == MusECore::Track::AUDIO_OUTPUT) 
        //{
        //  const MusECore::Route s(srcName, false, r->channel);
        //  srcName = s.name();
        //}
        //if(src->channel != -1)
        //  srcName += QString(":") + QString::number(r->channel);
        //const MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
        //const MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
        //const MusECore::Route src(MusECore::Route::TRACK_ROUTE, -1, track, r->remoteChannel, r->channels, r->channel, 0);

        //MusECore::Route dst(*r);
        //if(dst.type == MusECore::Route::JACK_ROUTE)
        //  dst.channel = -1;
        routesItem = findRoutesItem(src, dst);
        if(routesItem)
        {
          // Update the text.
          routesItem->setText(ROUTE_SRC_COL, srcName);
          routesItem->setText(ROUTE_DST_COL, dstName);
        }
        else
        {
          routeList->blockSignals(true);
          routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
          routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
          routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
          routeList->blockSignals(false);
        }
      }
    }
  //}

  
  //
  // MIDI ports:
  //
  
  const QString none_str = tr("<none>");
  dstCatItem = newDstList->findCategoryItem(midiPortsCat);
  srcCatItem = newSrcList->findCategoryItem(midiPortsCat);
  for(int i = 0; i < MIDI_PORTS; ++i) 
  {
    MusECore::MidiPort* mp = &MusEGlobal::midiPorts[i];
    if(!mp)
      continue;
    MusECore::MidiDevice* md = mp->device();
    // Synth are tracks and devices. Don't list them as devices here, list them as tracks, above.
    if(md && md->deviceType() == MusECore::MidiDevice::SYNTH_MIDI)
      continue;
    
    QString mdname;
    mdname = QString::number(i + 1) + QString(":");
    mdname += md ? md->name() : none_str;
      
    //
    // DESTINATION section:
    //


#ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    bool non_route_found = false;
    // Simulate routes for each midi track's output port and channel properties.
    MusECore::MidiTrackList* tl = MusEGlobal::song->midis();
    for(MusECore::ciMidiTrack imt = tl->begin(); imt != tl->end(); ++imt)
    {
      MusECore::MidiTrack* mt = *imt;
      const int port = mt->outPort();
      const int chan = mt->outChannel();
      if(port != i)
        continue;
      non_route_found = true;
      const MusECore::Route src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, mt, chan, -1, -1, NULL);
      const MusECore::Route dst(i, chan);
      const QString srcName = mt->name() + QString(" [") + QString::number(chan) + QString("]");
      const QString dstName = mdname;
      routesItem = findRoutesItem(src, dst);
      if(routesItem)
      {
        // Update the text.
        routesItem->setText(ROUTE_SRC_COL, srcName);
        routesItem->setText(ROUTE_DST_COL, dstName);
      }
      else
      {
        routeList->blockSignals(true);
        routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
        routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
        routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
        routeList->blockSignals(false);
      }
    }
#endif  // _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
    
    //if(md->rwFlags() & 0x02) // Readable
//     if(md->rwFlags() & 0x01)   // Writeable
    //if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    
    // Show either all midi ports, or only ports that have a device or have input routes.
    if(allMidiPortsButton->isChecked() || md || !mp->inRoutes()->empty()
       #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
       || non_route_found
       #endif
    )
    {
      const MusECore::Route dst(i, -1);
      item = newDstList->findItem(dst, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << midiDevicesCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << midiPortsCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
          
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(dstCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, false, dst);
        //item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }
//       for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
//       {
//         const MusECore::Route sub_r(md, channel);
//         subitem = findDstItem(sub_r);
//         if(!subitem)
//         {
//           newDstList->blockSignals(true);
//           subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel + 1) << QString() );
//           subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(sub_r));
//           
//           QFont fnt(subitem->font(ROUTE_NAME_COL));
//           fnt.setPointSize(4);
//           //fprintf(stderr, "point size:%d family:%s\n", fnt.pointSize(), fnt.family().toLatin1().constData());
//           //subitem->font(ROUTE_NAME_COL).setPointSize(2);
//           //subitem->font(ROUTE_TYPE_COL).setPointSize(2);
//           subitem->setFont(ROUTE_NAME_COL, fnt);
//           subitem->setFont(ROUTE_TYPE_COL, fnt);
//           newDstList->blockSignals(false);
//         }
//       }


// #ifdef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
//       // Simulate routes for each midi track's output port and channel properties.
//       MusECore::MidiTrackList* tl = MusEGlobal::song->midis();
//       for(MusECore::ciMidiTrack imt = tl->begin(); imt != tl->end(); ++imt)
//       {
//         MusECore::MidiTrack* mt = *imt;
//         const int port = mt->outPort();
//         const int chan = mt->outChannel();
//         if(port != i)
//           continue;
//         const MusECore::Route src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, mt, chan, -1, -1, NULL);
//         const MusECore::Route dst(i, chan);
//         const QString srcName = mt->name() + QString(" [") + QString::number(chan) + QString("]");
//         const QString dstName = mdname;
//         routesItem = findRoutesItem(src, dst);
//         if(routesItem)
//         {
//           // Update the text.
//           routesItem->setText(ROUTE_SRC_COL, srcName);
//           routesItem->setText(ROUTE_DST_COL, dstName);
//         }
//         else
//         {
//           routeList->blockSignals(true);
//           routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
//           routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
//           routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
//           routeList->blockSignals(false);
//         }
//       }
// #endif  // _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_

      const MusECore::RouteList* rl = mp->inRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        switch(r->type)
        {
          case MusECore::Route::TRACK_ROUTE: 
            
#ifndef _USE_MIDI_TRACK_SINGLE_OUT_PORT_CHAN_
          {
            if(!r->track || !r->track->isMidiTrack())
              continue;
            
//             const MusECore::Route src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, r->track, r->channel, r->channels, r->remoteChannel, NULL);
            const MusECore::Route& src = *r;
            QString srcName = r->name();
            QString dstName = mdname;
            //const MusECore::Route dst(i, -1);
            const MusECore::Route dst(i, src.channel);

            if(src.channel != -1)
              srcName += QString(" [") + QString::number(src.channel) + QString("]");
//             if(dst.channel != -1)
//               dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              routeList->blockSignals(false);
            }
//             if(!r->jackPort)
//               routesItem->setBackground(ROUTE_SRC_COL, routesItem->background(ROUTE_SRC_COL).color().darker());
          }
#endif  // _USE_MIDI_TRACK_OUT_ROUTES_

          break;
          
          case MusECore::Route::JACK_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          break;
        }
      }
    }
//     else if(track->type() != MusECore::Track::AUDIO_AUX)
//     {
//       const MusECore::Route r(track, -1);
//       item = findDstItem(r);
//       if(!item)
//       {
//         if(!dstCatItem)
//         {
//           newDstList->blockSignals(true);
//           dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << QString("Tracks") << QString() );
//           //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//           newDstList->blockSignals(false);
//         }
//         newDstList->blockSignals(true);
//         item = new QTreeWidgetItem(dstCatItem, QStringList() << track->name() << QString("Track") );
//         item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//         newDstList->blockSignals(false);
//       }
//       //if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//       {
//         //for(int channel = 0; channel < track->channels(); ++channel)
//         const int chans = track->type() == MusECore::Track::AUDIO_SOFTSYNTH ? track->totalInChannels() : track->channels();
//         for(int channel = 0; channel < chans; ++channel)
//         {
//           const MusECore::Route subr(track, channel, 1);
//           subitem = findDstItem(subr);
//           if(!subitem)
//           {
//             newDstList->blockSignals(true);
//             subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) << QString() );
//             subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(subr));
//             newDstList->blockSignals(false);
//           }
//         }
//       }
//     }
    
    //
    // SOURCE section:
    //
    
    //if(md->rwFlags() & 0x01) // Writeable
//     if(md->rwFlags() & 0x02) // Readable
    //if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    
    // Show only ports that have a device, or have output routes.
    if(allMidiPortsButton->isChecked() || md || !mp->outRoutes()->empty())
    {
      const MusECore::Route src(i, -1);
      item = newSrcList->findItem(src, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << midiDevicesCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << midiPortsCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(srcCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, true, src);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }
//       for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
//       {
//         const MusECore::Route src_r(md, channel);
//         subitem = findSrcItem(src_r);
//         if(!subitem)
//         {
//           newSrcList->blockSignals(true);
//           subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel + 1) << QString() );
//           subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src_r));
//           newSrcList->blockSignals(false);
//         }
//       }
    
//     else
//     {
//       const MusECore::Route r(track, -1);
//       item = findSrcItem(r);
//       if(!item)
//       {
//         if(!srcCatItem)
//         {
//           newSrcList->blockSignals(true);
//           srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << QString("Tracks") << QString() );
//           //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//           newSrcList->blockSignals(false);
//         }
//         newSrcList->blockSignals(true);
//         item = new QTreeWidgetItem(srcCatItem, QStringList() << track->name() << QString("Track") );
//         item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//         newSrcList->blockSignals(false);
//       }
//       
//       //if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//       {
//         //for(int channel = 0; channel < track->channels(); ++channel)
//         const int chans = track->type() == MusECore::Track::AUDIO_SOFTSYNTH ? track->totalOutChannels() : track->channels();
//         for(int channel = 0; channel < chans; ++channel)
//         {
//           const MusECore::Route subr(track, channel, 1);
//           subitem = findSrcItem(subr);
//           if(!subitem)
//           {
//             newSrcList->blockSignals(true);
//             subitem = new QTreeWidgetItem(item, QStringList() << QString("ch ") + QString::number(channel + 1) << QString() );
//             subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(subr));
//             newSrcList->blockSignals(false);
//           }
//         }
//       }
//     }

      const MusECore::RouteList* rl = mp->outRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src: TrackA, Channel  4, Remote Channel  2  [dst: TrackB channel  2 Remote Channel  4]
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src: TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        //MusECore::Route src(md, -1);
        //MusECore::Route dst;
        //QString srcName = mdname;
        //QString dstName;
        switch(r->type)
        {
          case MusECore::Route::TRACK_ROUTE: 
          {
            if(!r->track || !r->track->isMidiTrack())
              continue;
            
            //const MusECore::Route dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            const MusECore::Route& dst = *r;
            QString dstName = r->name();
            QString srcName = mdname;
            //const MusECore::Route src(i, -1);
            const MusECore::Route src(i, dst.channel);

            //if(src.channel != -1)
            //  srcName += QString(" [") + QString::number(src.channel) + QString("]");
            if(dst.channel != -1)
              dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              routeList->blockSignals(false);
            }
//             if(!r->jackPort)
//               routesItem->setBackground(ROUTE_DST_COL, routesItem->background(ROUTE_DST_COL).color().darker());
          }
          break;
          
          case MusECore::Route::JACK_ROUTE:
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
            continue;
        }

  //       QString srcName = mdname;
  //       MusECore::Route src(md, -1);
  // 
  //       if(src.channel != -1)
  //         srcName += QString(" [") + QString::number(src.channel) + QString("]");
  //       if(dst.channel != -1)
  //         dstName += QString(" [") + QString::number(dst.channel) + QString("]");
        
        
        
        
        //QString srcName(track->name());
        //if(track->type() == MusECore::Track::AUDIO_OUTPUT) 
        //{
        //  const MusECore::Route s(srcName, false, r->channel);
        //  srcName = s.name();
        //}
        //if(src->channel != -1)
        //  srcName += QString(":") + QString::number(r->channel);
        //const MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
        //const MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
        //const MusECore::Route src(MusECore::Route::TRACK_ROUTE, -1, track, r->remoteChannel, r->channels, r->channel, 0);

        //MusECore::Route dst(*r);
        //if(dst.type == MusECore::Route::JACK_ROUTE)
        //  dst.channel = -1;
  //       routesItem = findRoutesItem(src, dst);
  //       if(routesItem)
  //       {
  //         // Update the text.
  //         routesItem->setText(ROUTE_SRC_COL, srcName);
  //         routesItem->setText(ROUTE_DST_COL, dstName);
  //       }
  //       else
  //       {
  //         routeList->blockSignals(true);
  //         routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
  //         routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
  //         routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
  //         routeList->blockSignals(false);
  //       }
      }
    }
  }

  
  //
  // MIDI devices:
  //
  
  dstCatItem = newDstList->findCategoryItem(midiDevicesCat);
  srcCatItem = newSrcList->findCategoryItem(midiDevicesCat);
  for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
  {
    MusECore::MidiDevice* md = *i;
    // Synth are tracks and devices. Don't list them as devices here, list them as tracks, above.
    if(md->deviceType() == MusECore::MidiDevice::SYNTH_MIDI)
      continue;
    
    QString mdname;
    if(md->midiPort() != -1)
      mdname = QString::number(md->midiPort() + 1) + QString(":");
    mdname += md->name();
    
    //
    // DESTINATION section:
    //
    
    //if(md->rwFlags() & 0x02) // Readable
    //if(md->rwFlags() & 0x01)   // Writeable
    if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    {
      const MusECore::Route dst(md, -1);
      item = newDstList->findItem(dst, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << midiDevicesCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << midiDevicesCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
          
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(dstCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, false, dst);
        //item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }
//       for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
//       {
//         const MusECore::Route sub_r(md, channel);
//         subitem = findDstItem(sub_r);
//         if(!subitem)
//         {
//           newDstList->blockSignals(true);
//           subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel + 1) << QString() );
//           subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(sub_r));
//           
//           QFont fnt(subitem->font(ROUTE_NAME_COL));
//           fnt.setPointSize(4);
//           //fprintf(stderr, "point size:%d family:%s\n", fnt.pointSize(), fnt.family().toLatin1().constData());
//           //subitem->font(ROUTE_NAME_COL).setPointSize(2);
//           //subitem->font(ROUTE_TYPE_COL).setPointSize(2);
//           subitem->setFont(ROUTE_NAME_COL, fnt);
//           subitem->setFont(ROUTE_TYPE_COL, fnt);
//           newDstList->blockSignals(false);
//         }
//       }

      const MusECore::RouteList* rl = md->inRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After: [src  TrackA, Channel  4, Remote Channel  2]  dst: TrackB channel  2 Remote Channel  4
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src  TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        //MusECore::Route src;
        //MusECore::Route dst(md, -1);
        //QString srcName;
        //QString dstName = mdname;
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE: 
          {
            //src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, r->channels, -1, r->persistentJackPortName);
            //src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, -1, -1, r->persistentJackPortName);
            const MusECore::Route src = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            //src = *r;
            //dst = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       r->channels, -1, 0);
            //dst = MusECore::Route(MusECore::Route::MIDI_DEVICE_ROUTE, -1, md,       r->channel,       1, -1, 0);
            //dst = MusECore::Route(md, r->channel);
            //dst = MusECore::Route(md, -1);
            QString srcName = r->name();
            QString dstName = mdname;
            const MusECore::Route dst(md, -1);

            if(src.channel != -1)
              srcName += QString(" [") + QString::number(src.channel) + QString("]");
            if(dst.channel != -1)
              dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              routeList->blockSignals(false);
            }
            if(!r->jackPort)
              routesItem->setBackground(ROUTE_SRC_COL, routesItem->background(ROUTE_SRC_COL).color().darker());
          }
          break;
          
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
          case MusECore::Route::TRACK_ROUTE: 
            continue;
        }
        
//         QString dstName = mdname;
//         MusECore::Route dst(md, -1);
// 
//         if(src.channel != -1)
//           srcName += QString(" [") + QString::number(src.channel) + QString("]");
//         if(dst.channel != -1)
//           dstName += QString(" [") + QString::number(dst.channel) + QString("]");
  


//         QString srcName(r->name());
//         if(r->channel != -1)
//           srcName += QString(":") + QString::number(r->channel);
//         
//         
//         MusECore::Route src(*r);
//         if(src.type == MusECore::Route::JACK_ROUTE)
//           src.channel = -1;
//         //const MusECore::Route dst(track->name(), true, r->channel, MusECore::Route::TRACK_ROUTE);
//         const MusECore::Route dst(MusECore::Route::TRACK_ROUTE, -1, track, r->remoteChannel, r->channels, r->channel, 0);
//         
//         
//         src.remoteChannel = src.channel;
//         dst.remoteChannel = dst.channel;
//         const int src_chan = src.channel;
//         src.channel = dst.channel;
//         dst.channel = src_chan;
        
        
//         routesItem = findRoutesItem(src, dst);
//         if(routesItem)
//         {
//           // Update the text.
//           routesItem->setText(ROUTE_SRC_COL, srcName);
//           routesItem->setText(ROUTE_DST_COL, dstName);
//         }
//         else
//         {
//           routeList->blockSignals(true);
//           routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
//           routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
//           routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
//           routeList->blockSignals(false);
//         }
      }
    }
//     else if(track->type() != MusECore::Track::AUDIO_AUX)
//     {
//       const MusECore::Route r(track, -1);
//       item = findDstItem(r);
//       if(!item)
//       {
//         if(!dstCatItem)
//         {
//           newDstList->blockSignals(true);
//           dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << QString("Tracks") << QString() );
//           //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//           newDstList->blockSignals(false);
//         }
//         newDstList->blockSignals(true);
//         item = new QTreeWidgetItem(dstCatItem, QStringList() << track->name() << QString("Track") );
//         item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//         newDstList->blockSignals(false);
//       }
//       //if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//       {
//         //for(int channel = 0; channel < track->channels(); ++channel)
//         const int chans = track->type() == MusECore::Track::AUDIO_SOFTSYNTH ? track->totalInChannels() : track->channels();
//         for(int channel = 0; channel < chans; ++channel)
//         {
//           const MusECore::Route subr(track, channel, 1);
//           subitem = findDstItem(subr);
//           if(!subitem)
//           {
//             newDstList->blockSignals(true);
//             subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel) << QString() );
//             subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(subr));
//             newDstList->blockSignals(false);
//           }
//         }
//       }
//     }
    
    //
    // SOURCE section:
    //
    
    //if(md->rwFlags() & 0x01) // Writeable
    //if(md->rwFlags() & 0x02) // Readable
    if(md->rwFlags() & 0x03) // Both readable and writeable need to be shown
    {
      const MusECore::Route src(md, -1);
      item = newSrcList->findItem(src, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, mdname);
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << midiDevicesCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << midiDevicesCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << mdname << midiDeviceLabel );
        item = new RouteTreeWidgetItem(srcCatItem, QStringList() << mdname, RouteTreeWidgetItem::RouteItem, true, src);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }
//       for(int channel = 0; channel < MIDI_CHANNELS; ++channel)
//       {
//         const MusECore::Route src_r(md, channel);
//         subitem = findSrcItem(src_r);
//         if(!subitem)
//         {
//           newSrcList->blockSignals(true);
//           subitem = new QTreeWidgetItem(item, QStringList() << QString::number(channel + 1) << QString() );
//           subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(src_r));
//           newSrcList->blockSignals(false);
//         }
//       }
    
//     else
//     {
//       const MusECore::Route r(track, -1);
//       item = findSrcItem(r);
//       if(!item)
//       {
//         if(!srcCatItem)
//         {
//           newSrcList->blockSignals(true);
//           srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << QString("Tracks") << QString() );
//           //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//           newSrcList->blockSignals(false);
//         }
//         newSrcList->blockSignals(true);
//         item = new QTreeWidgetItem(srcCatItem, QStringList() << track->name() << QString("Track") );
//         item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(r));
//         newSrcList->blockSignals(false);
//       }
//       
//       //if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
//       {
//         //for(int channel = 0; channel < track->channels(); ++channel)
//         const int chans = track->type() == MusECore::Track::AUDIO_SOFTSYNTH ? track->totalOutChannels() : track->channels();
//         for(int channel = 0; channel < chans; ++channel)
//         {
//           const MusECore::Route subr(track, channel, 1);
//           subitem = findSrcItem(subr);
//           if(!subitem)
//           {
//             newSrcList->blockSignals(true);
//             subitem = new QTreeWidgetItem(item, QStringList() << QString("ch ") + QString::number(channel + 1) << QString() );
//             subitem->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(subr));
//             newSrcList->blockSignals(false);
//           }
//         }
//       }
//     }

      const MusECore::RouteList* rl = md->outRoutes();
      for(MusECore::ciRoute r = rl->begin(); r != rl->end(); ++r) 
      {
        // Ex. Params:  src: TrackA, Channel  2, Remote Channel -1   dst: TrackB channel  4 Remote Channel -1
        //      After:  src: TrackA, Channel  4, Remote Channel  2  [dst: TrackB channel  2 Remote Channel  4]
        //
        // Ex.
        //     Params:  src: TrackA, Channel  2, Remote Channel -1   dst: JackAA channel -1 Remote Channel -1
        //      After: (src: TrackA, Channel -1, Remote Channel  2)  dst: JackAA channel  2 Remote Channel -1
        //MusECore::Route src(md, -1);
        //MusECore::Route dst;
        //QString srcName = mdname;
        //QString dstName;
        switch(r->type)
        {
          case MusECore::Route::JACK_ROUTE:
          {
            //src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       r->channels, -1, 0);
            //src = MusECore::Route(MusECore::Route::TRACK_ROUTE, -1, track,       r->channel,       1, -1, 0);
            //src = MusECore::Route(md, r->channel);
            //src = MusECore::Route(md, -1);
            //dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, r->channels, -1, r->persistentJackPortName);
            //dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, r->remoteChannel, -1, -1, r->persistentJackPortName);
            const MusECore::Route dst = MusECore::Route(MusECore::Route::JACK_ROUTE,  -1, r->jackPort, -1, -1, -1, r->persistentJackPortName);
            //dst = *r;
            QString dstName = r->name();
            QString srcName = mdname;
            const MusECore::Route src(md, -1);

            if(src.channel != -1)
              srcName += QString(" [") + QString::number(src.channel) + QString("]");
            if(dst.channel != -1)
              dstName += QString(" [") + QString::number(dst.channel) + QString("]");
            
            routesItem = findRoutesItem(src, dst);
            if(routesItem)
            {
              // Update the text.
              routesItem->setText(ROUTE_SRC_COL, srcName);
              routesItem->setText(ROUTE_DST_COL, dstName);
            }
            else
            {
              routeList->blockSignals(true);
              routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
              routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
              routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
              routeList->blockSignals(false);
            }
            if(!r->jackPort)
              routesItem->setBackground(ROUTE_DST_COL, routesItem->background(ROUTE_DST_COL).color().darker());
          }
          break;
          
          case MusECore::Route::MIDI_DEVICE_ROUTE: 
          case MusECore::Route::MIDI_PORT_ROUTE: 
          case MusECore::Route::TRACK_ROUTE: 
            continue;
        }

  //       QString srcName = mdname;
  //       MusECore::Route src(md, -1);
  // 
  //       if(src.channel != -1)
  //         srcName += QString(" [") + QString::number(src.channel) + QString("]");
  //       if(dst.channel != -1)
  //         dstName += QString(" [") + QString::number(dst.channel) + QString("]");
        
        
        
        
        //QString srcName(track->name());
        //if(track->type() == MusECore::Track::AUDIO_OUTPUT) 
        //{
        //  const MusECore::Route s(srcName, false, r->channel);
        //  srcName = s.name();
        //}
        //if(src->channel != -1)
        //  srcName += QString(":") + QString::number(r->channel);
        //const MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
        //const MusECore::Route src(track->name(), false, r->channel, MusECore::Route::TRACK_ROUTE);
        //const MusECore::Route src(MusECore::Route::TRACK_ROUTE, -1, track, r->remoteChannel, r->channels, r->channel, 0);

        //MusECore::Route dst(*r);
        //if(dst.type == MusECore::Route::JACK_ROUTE)
        //  dst.channel = -1;
  //       routesItem = findRoutesItem(src, dst);
  //       if(routesItem)
  //       {
  //         // Update the text.
  //         routesItem->setText(ROUTE_SRC_COL, srcName);
  //         routesItem->setText(ROUTE_DST_COL, dstName);
  //       }
  //       else
  //       {
  //         routeList->blockSignals(true);
  //         routesItem = new QTreeWidgetItem(routeList, QStringList() << srcName << dstName);
  //         routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(src));
  //         routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(dst));
  //         routeList->blockSignals(false);
  //       }
      }
    }
  }

  //
  // JACK ports:
  //
  
  if(MusEGlobal::checkAudioDevice())
  {
    //------------
    // Jack audio:
    //------------
    
    srcCatItem = newSrcList->findCategoryItem(jackCat);
    MusECore::RouteList in_rl;
    for(std::list<QString>::iterator i = tmpJackOutPorts.begin(); i != tmpJackOutPorts.end(); ++i)
    {
      const MusECore::Route in_r(*i, false, -1, MusECore::Route::JACK_ROUTE);
      item = newSrcList->findItem(in_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, in_r.name());
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << jackCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << jackCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << in_r.name() << jackLabel );
        item = new RouteTreeWidgetItem(srcCatItem, QStringList() << in_r.name(), RouteTreeWidgetItem::RouteItem, true, in_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }
      in_rl.push_back(in_r);
    }

    dstCatItem = newDstList->findCategoryItem(jackCat);
    for(std::list<QString>::iterator i = tmpJackInPorts.begin(); i != tmpJackInPorts.end(); ++i)
    {
      const MusECore::Route out_r(*i, true, -1, MusECore::Route::JACK_ROUTE);
      item = newDstList->findItem(out_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, out_r.name());
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << jackCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << jackCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << out_r.name() << jackLabel );
        item = new RouteTreeWidgetItem(dstCatItem, QStringList() << out_r.name(), RouteTreeWidgetItem::RouteItem, false, out_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }
      for(MusECore::ciRoute i = in_rl.begin(); i != in_rl.end(); ++i)
      {
        const MusECore::Route& in_r = *i;
        if(MusECore::routeCanDisconnect(in_r, out_r))
        {
          routesItem = findRoutesItem(in_r, out_r);
          if(routesItem)
          {
            // Update the text.
            routesItem->setText(ROUTE_SRC_COL, in_r.name());
            routesItem->setText(ROUTE_DST_COL, out_r.name());
          }
          else
          {
            routeList->blockSignals(true);
            routesItem = new QTreeWidgetItem(routeList, QStringList() << in_r.name() << out_r.name());
            routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
            routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
            routeList->blockSignals(false);
          }
        }
      }
    }
  
    //------------
    // Jack midi:
    //------------
    
    srcCatItem = newSrcList->findCategoryItem(jackMidiCat);
    in_rl.clear();
    for(std::list<QString>::iterator i = tmpJackMidiOutPorts.begin(); i != tmpJackMidiOutPorts.end(); ++i)
    {
      const MusECore::Route in_r(*i, false, -1, MusECore::Route::JACK_ROUTE);
      item = newSrcList->findItem(in_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, in_r.name());
      }
      else
      {
        if(!srcCatItem)
        {
          newSrcList->blockSignals(true);
          //srcCatItem = new QTreeWidgetItem(newSrcList, QStringList() << jackMidiCat << QString() );
          srcCatItem = new RouteTreeWidgetItem(newSrcList, QStringList() << jackMidiCat, RouteTreeWidgetItem::CategoryItem, true);
          srcCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = srcCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          srcCatItem->setFont(ROUTE_NAME_COL, fnt);
          srcCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          srcCatItem->setExpanded(true);
          newSrcList->blockSignals(false);
        }
        newSrcList->blockSignals(true);
        //item = new QTreeWidgetItem(srcCatItem, QStringList() << in_r.name() << jackMidiLabel );
        item = new RouteTreeWidgetItem(srcCatItem, QStringList() << in_r.name(), RouteTreeWidgetItem::RouteItem, true, in_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newSrcList->blockSignals(false);
      }
      in_rl.push_back(in_r);
    }
    
    dstCatItem = newDstList->findCategoryItem(jackMidiCat);
    for(std::list<QString>::iterator i = tmpJackMidiInPorts.begin(); i != tmpJackMidiInPorts.end(); ++i)
    {
      const MusECore::Route out_r(*i, true, -1, MusECore::Route::JACK_ROUTE);
      item = newDstList->findItem(out_r, RouteTreeWidgetItem::RouteItem);
      if(item)
      {
        // Update the text.
        item->setText(ROUTE_NAME_COL, out_r.name());
      }
      else
      {
        if(!dstCatItem)
        {
          newDstList->blockSignals(true);
          //dstCatItem = new QTreeWidgetItem(newDstList, QStringList() << jackMidiCat << QString() );
          dstCatItem = new RouteTreeWidgetItem(newDstList, QStringList() << jackMidiCat, RouteTreeWidgetItem::CategoryItem, false);
          dstCatItem->setFlags(Qt::ItemIsEnabled);
          QFont fnt = dstCatItem->font(ROUTE_NAME_COL);
          fnt.setBold(true);
          fnt.setItalic(true);
          //fnt.setPointSize(fnt.pointSize() + 2);
          dstCatItem->setFont(ROUTE_NAME_COL, fnt);
          dstCatItem->setTextAlignment(ROUTE_NAME_COL, align_flags);
          dstCatItem->setExpanded(true);
          newDstList->blockSignals(false);
        }
        newDstList->blockSignals(true);
        //item = new QTreeWidgetItem(dstCatItem, QStringList() << out_r.name() << jackMidiLabel );
        item = new RouteTreeWidgetItem(dstCatItem, QStringList() << out_r.name(), RouteTreeWidgetItem::RouteItem, false, out_r);
        //item->setData(ROUTE_NAME_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
        item->setTextAlignment(ROUTE_NAME_COL, align_flags);
        newDstList->blockSignals(false);
      }
      for(MusECore::ciRoute i = in_rl.begin(); i != in_rl.end(); ++i)
      {
        const MusECore::Route& in_r = *i;
        if(MusECore::routeCanDisconnect(in_r, out_r))
        {
          routesItem = findRoutesItem(in_r, out_r);
          if(routesItem)
          {
            // Update the text.
            routesItem->setText(ROUTE_SRC_COL, in_r.name());
            routesItem->setText(ROUTE_DST_COL, out_r.name());
          }
          else
          {
            routeList->blockSignals(true);
            routesItem = new QTreeWidgetItem(routeList, QStringList() << in_r.name() << out_r.name());
            routesItem->setData(ROUTE_SRC_COL, RouteDialog::RouteRole, QVariant::fromValue(in_r));
            routesItem->setData(ROUTE_DST_COL, RouteDialog::RouteRole, QVariant::fromValue(out_r));
            routeList->blockSignals(false);
          }
        }
      }
    }
  }
}

void MusE::startRouteDialog()
{
  if(routeDialog == 0)
    // NOTE: For deleting parentless dialogs and widgets, please add them to MusE::deleteParentlessDialogs().
    routeDialog = new MusEGui::RouteDialog;
  routeDialog->show();
  routeDialog->raise();
}


} // namespace MusEGui
