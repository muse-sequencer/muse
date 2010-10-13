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
#include <q3listview.h>

class QString;
class Q3ListBox;
class Q3ListBoxText;
//class QListView;
//class QListViewItem;
class ListBoxData;
class ListViewData;

//---------------------------------------------------------
//   ListBoxData
//---------------------------------------------------------

class ListBoxData : public Q3ListBoxText 
{
    void* _data;
    
  public:
    ListBoxData(Q3ListBox* listbox, const QString& text = QString::null)
                : Q3ListBoxText(listbox, text)  { _data = 0; }
    ListBoxData(const QString& text = QString::null) : Q3ListBoxText(text)  { _data = 0; }
    ListBoxData(Q3ListBox* listbox, const QString& text, ListBoxData* after) 
                : Q3ListBoxText(listbox, text, (Q3ListBoxText*)after)  { _data = 0; }
    ~ListBoxData() { }
    
    virtual void setText(const QString& text) { Q3ListBoxText::setText(text); }
    void* data() { return _data; }
    void setData(void* dat) { _data = dat; }
};

//---------------------------------------------------------
//   ListViewData
//---------------------------------------------------------

class ListViewData : public Q3ListViewItem 
{
    void* _data;
    
  public:
    ListViewData(Q3ListView* parent) : Q3ListViewItem(parent) { _data = 0; }
    //ListViewData(ListViewData* parent) : QListViewItem((QListViewItem*)parent) { _data = 0; }
    ListViewData(ListViewData* parent) : Q3ListViewItem(parent) { _data = 0; }
    //ListViewData(QListView* parent, ListViewData* after) : QListViewItem(parent, (QListViewItem*)after) { _data = 0; }
    ListViewData(Q3ListView* parent, ListViewData* after) : Q3ListViewItem(parent, after) { _data = 0; }
    //ListViewData(ListViewData* parent, ListViewData* after) : QListViewItem((QListViewItem*)parent, (QListViewItem*)after) { _data = 0; }
    ListViewData(ListViewData* parent, ListViewData* after) : Q3ListViewItem(parent, after) { _data = 0; }
    ListViewData(Q3ListView* parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, 
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                : Q3ListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }
    ListViewData(ListViewData* parent, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, 
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                //: QListViewItem((QListViewItem*)parent, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
                : Q3ListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
    ListViewData(Q3ListView* parent, ListViewData* after, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null, 
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                //: QListViewItem(parent, (QListViewItem*)after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
                : Q3ListViewItem(parent, after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
    ListViewData(ListViewData* parent, ListViewData* after, QString label1, QString label2 = QString::null, QString label3 = QString::null, QString label4 = QString::null,
                    QString label5 = QString::null, QString label6 = QString::null, QString label7 = QString::null, QString label8 = QString::null)
                //: QListViewItem((QListViewItem*)parent, (QListViewItem*)after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
                : Q3ListViewItem(parent, after, label1, label2, label3, label4, label5, label6, label7, label8) { _data = 0; }    
    ~ListViewData() { }
    
    void* data() { return _data; }
    void setData(void* dat) { _data = dat; }
};



#endif
 
