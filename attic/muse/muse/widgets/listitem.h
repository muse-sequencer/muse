//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: listitem.h,v 1.1.2.1 2008/08/18 00:15:26 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __LISTITEM_H__
#define __LISTITEM_H__

//#include <qstring.h>
//#include <qlistbox.h>
#include <qlistview.h>

class QString;
class QListBox;
class QListBoxText;
//class QListView;
//class QListViewItem;
class ListBoxData;
class ListViewData;

//---------------------------------------------------------
//   ListBoxData
//---------------------------------------------------------

class ListBoxData : public QListBoxText 
{
    void* _data;
    
  public:
    ListBoxData(QListBox* listbox, const QString& text = QString::null)
                : QListBoxText(listbox, text)  { _data = 0; }
    ListBoxData(const QString& text = QString::null) : QListBoxText(text)  { _data = 0; }
    ListBoxData(QListBox* listbox, const QString& text, ListBoxData* after) 
                : QListBoxText(listbox, text, (QListBoxText*)after)  { _data = 0; }
    ~ListBoxData() { }
    
    virtual void setText(const QString& text) { QListBoxText::setText(text); }
    void* data() { return _data; }
    void setData(void* dat) { _data = dat; }
};

//---------------------------------------------------------
//   ListViewData
//---------------------------------------------------------

class ListViewData : public QListViewItem 
{
    void* _data;
    
  public:
    ListViewData(QListView* parent) : QListViewItem(parent) { _data = 0; }
    //ListViewData(ListViewData* parent) : QListViewItem((QListViewItem*)parent) { _data = 0; }
    ListViewData(ListViewData* parent) : QListViewItem(parent) { _data = 0; }
    //ListViewData(QListView* parent, ListViewData* after) : QListViewItem(parent, (QListViewItem*)after) { _data = 0; }
    ListViewData(QListView* parent, ListViewData* after) : QListViewItem(parent, after) { _data = 0; }
    //ListViewData(ListViewData* parent, ListViewData* after) : QListViewItem((QListViewItem*)parent, (QListViewItem*)after) { _data = 0; }
    ListViewData(ListViewData* parent, ListViewData* after) : QListViewItem(parent, after) { _data = 0; }
    ListViewData(QListView* parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, 
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                : QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }
    ListViewData(ListViewData* parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, 
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                //: QListViewItem((QListViewItem*)parent, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
                : QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
    ListViewData(QListView* parent, ListViewData* after, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, 
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                //: QListViewItem(parent, (QListViewItem*)after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
                : QListViewItem(parent, after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
    ListViewData(ListViewData* parent, ListViewData* after, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null,
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                //: QListViewItem((QListViewItem*)parent, (QListViewItem*)after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
                : QListViewItem(parent, after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
    ~ListViewData() { }
    
    void* data() { return _data; }
    void setData(void* dat) { _data = dat; }
};



#endif
 
