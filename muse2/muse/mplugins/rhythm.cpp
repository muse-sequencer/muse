//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rhythm.cpp,v 1.1.1.1 2003/10/27 18:52:49 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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
//
//  This code is an adaption of the random rhythm generator taken
//  from "The JAZZ++ Midi Sequencer"
//  Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all
//  rights reserved.
//  Distributed under the GNU General Public License
//=========================================================

#include "rhythm.h"

namespace MusEGui {

//---------------------------------------------------------
//   RhythmGen
//---------------------------------------------------------

RhythmGen::RhythmGen(QWidget* parent, Qt::WFlags fo)
   : QMainWindow(parent, fo)
      {
      setupUi(this);
      }
RhythmGen::~RhythmGen()
      {
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void RhythmGen::closeEvent(QCloseEvent* ev)
      {
      emit hideWindow();
      QWidget::closeEvent(ev);
      }



#if 0
/****************************************************************************
** Form implementation generated from reading ui file 'rhythm.ui'
**
** Created: Tue Feb 26 13:43:04 2002
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#include "rhythm.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariant>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QLCDNumber>
#include <QPushButton>
#include <QSlider>
#include <QToolButton>
#include <QLayout>
#include <QToolTip>
#include <QImage>
#include <QPixmap>

static const char* const image0_data[] = {
"16 16 56 1",
". c None",
"F c #000000",
"L c #000101",
"E c #010304",
"C c #06101d",
"j c #071728",
"w c #07182a",
"1 c #071c2c",
"0 c #081c2d",
"e c #081e31",
"a c #0a121f",
"Z c #0a1929",
"Q c #0a263b",
"T c #0a273b",
"z c #0e97bf",
"s c #0ea0c5",
"K c #0f5d7e",
"B c #105375",
"G c #114760",
"N c #1197b9",
"x c #124c6d",
"# c #124d6f",
"u c #1290b5",
"v c #135476",
"h c #1390b4",
"Y c #14839f",
"b c #155474",
"i c #155678",
"o c #155a7a",
"J c #159abc",
"d c #17587a",
"V c #178eac",
"f c #194a6a",
"r c #19b3ce",
"k c #1b4b6a",
"p c #1b4f6f",
"A c #1ca2c3",
"D c #20374f",
"P c #2294b1",
"I c #22bad1",
"S c #2a98b3",
"U c #2cc7d5",
"n c #3cd7e1",
"O c #43d5de",
"t c #48dfe9",
"X c #58acc5",
"m c #59c3da",
"q c #71d1e0",
"W c #80c2db",
"c c #819eba",
"H c #9ddeee",
"M c #cdebf6",
"g c #ddeff8",
"y c #e2f0f9",
"R c #fdfafd",
"l c #fdfcfd",
"................",
"................",
".......#a.......",
"......bcde......",
".....fcghij.....",
"....kclmnhoj....",
"...pclqrstuvw...",
"..xcymrzzsnABC..",
".BDEFGHzIJKLFFF.",
".....BMNOPQ.....",
".....BRNOST.....",
".....BMNUVT.....",
".....BWXIYF.....",
".....BZ01FF.....",
"................",
"................"};

static const char* const image1_data[] = {
"16 16 50 1",
". c None",
"E c #010001",
"O c #03060c",
"U c #050b12",
"u c #05151e",
"S c #060f19",
"m c #06141d",
"q c #06141f",
"h c #071620",
"D c #0b293e",
"c c #0d324c",
"v c #0d3652",
"A c #0e5775",
"J c #0e8fb6",
"x c #0e94b9",
"r c #0e97bf",
"Q c #0e9ec3",
"n c #0eadcb",
"R c #0fa0c4",
"N c #106589",
"V c #10698f",
"B c #114560",
"# c #124d6f",
"z c #1299bb",
"C c #144059",
"T c #14a5c9",
"K c #15c1da",
"b c #18425f",
"y c #1bbad1",
"M c #1ca2c3",
"t c #1f95b2",
"a c #254a64",
"p c #2695b1",
"l c #2b91ae",
"F c #337f9e",
"o c #40d1db",
"s c #46dbe6",
"L c #48dfe9",
"I c #54bdd7",
"j c #5fc0d8",
"g c #64a3bd",
"k c #8fcce2",
"d c #a0cbdf",
"H c #a7dcec",
"w c #bfdfee",
"f c #d3e5f0",
"G c #dfeff8",
"i c #e0f1fb",
"e c #eff3fc",
"P c #ffffff",
"................",
"................",
".....###abc.....",
".....#defgh.....",
".....#ijklm.....",
".....#enopq.....",
".....airstu.....",
".v##aawxyzABCDE.",
"..vFGHIJrKLMNO..",
"...vFPQrRLMNS...",
"....vFPTLMNU....",
".....vFGMNU.....",
"......vVNS......",
".......vO.......",
"................",
"................"};


/*
 *  Constructs a RhythmGenerator which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
RhythmGenerator::RhythmGenerator( QWidget* parent,  const char* name, bool modal, Qt::WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    QPixmap image0( ( const char** ) image0_data );
    QPixmap image1( ( const char** ) image1_data );
    if ( !name )
	setName( "RhythmGenerator" );
    resize( 500, 777 );
    setCaption( trUtf8( "Form3" ) );
    RhythmGeneratorLayout = new Q3VBoxLayout( this, 11, 6, "RhythmGeneratorLayout");

    Frame6 = new QFrame( this);
    Frame6->setFrameShape( QFrame::StyledPanel );
    Frame6->setFrameShadow( QFrame::Raised );
    Frame6Layout = new Q3VBoxLayout( Frame6, 11, 6, "Frame6Layout");

    TextLabel1 = new QLabel( Frame6, "TextLabel1" );
    QFont TextLabel1_font(  TextLabel1->font() );
    TextLabel1_font.setBold( TRUE );
    TextLabel1_font.setUnderline( TRUE );
    TextLabel1->setFont( TextLabel1_font );
    TextLabel1->setText( trUtf8( "Instrument Settings:" ) );
    Frame6Layout->addWidget( TextLabel1 );

    Layout17 = new Q3HBoxLayout( 0, 0, 15, "Layout17");

    Layout16 = new Q3VBoxLayout( 0, 0, 6, "Layout16");

    InstrumentListLabel = new QLabel( Frame6, "InstrumentListLabel" );
    InstrumentListLabel->setText( trUtf8( "Instrument" ) );
    Layout16->addWidget( InstrumentListLabel );

    InstrumentList = new Q3ListBox( Frame6, "InstrumentList" );
    InstrumentList->insertItem( trUtf8( "Hi-Hat" ) );
    InstrumentList->setMinimumSize( QSize( 150, 70 ) );
    Layout16->addWidget( InstrumentList );

    Layout11 = new Q3HBoxLayout( 0, 0, 2, "Layout11");

    InstrumentUp = new QToolButton( Frame6, "InstrumentUp" );
    InstrumentUp->setMinimumSize( QSize( 40, 20 ) );
    InstrumentUp->setText( trUtf8( "" ) );
    InstrumentUp->setPixmap( image0 );
    Layout11->addWidget( InstrumentUp );

    InstrumentDown = new QToolButton( Frame6, "InstrumentDown" );
    InstrumentDown->setMinimumSize( QSize( 40, 20 ) );
    InstrumentDown->setText( trUtf8( "" ) );
    InstrumentDown->setPixmap( image1 );
    Layout11->addWidget( InstrumentDown );

    InstrumentAdd = new QToolButton( Frame6, "InstrumentAdd" );
    InstrumentAdd->setMinimumSize( QSize( 40, 20 ) );
    InstrumentAdd->setText( trUtf8( "add" ) );
    Layout11->addWidget( InstrumentAdd );

    InstrumentDel = new QToolButton( Frame6, "InstrumentDel" );
    InstrumentDel->setMinimumSize( QSize( 40, 20 ) );
    InstrumentDel->setText( trUtf8( "delete" ) );
    Layout11->addWidget( InstrumentDel );
    Layout16->addLayout( Layout11 );
    Layout17->addLayout( Layout16 );

    Layout20 = new Q3VBoxLayout( 0, 0, 6, "Layout20");

    StepsLabel = new QLabel( Frame6, "StepsLabel" );
    StepsLabel->setText( trUtf8( "steps/count" ) );
    Layout20->addWidget( StepsLabel );

    StepsLayout = new Q3HBoxLayout( 0, 0, 6, "StepsLayout");

    StepsSlider = new QSlider( Frame6, "StepsSlider" );
    StepsSlider->setMaxValue( 16 );
    StepsSlider->setValue( 4 );
    StepsSlider->setOrientation( Qt::Horizontal );
    StepsLayout->addWidget( StepsSlider );

    StepsNumber = new QLCDNumber( Frame6, "StepsNumber" );
    StepsNumber->setPaletteBackgroundColor( QColor( 212, 212, 192 ) );
    StepsNumber->setLineWidth( 1 );
    StepsNumber->setNumDigits( 2 );
    StepsNumber->setSegmentStyle( QLCDNumber::Flat );
    StepsNumber->setProperty( "value", 4 );
    StepsLayout->addWidget( StepsNumber );
    Layout20->addLayout( StepsLayout );

    CountLabel = new QLabel( Frame6, "CountLabel" );
    CountLabel->setText( trUtf8( "count/bar" ) );
    Layout20->addWidget( CountLabel );

    CountLayout = new Q3HBoxLayout( 0, 0, 6, "CountLayout");

    CountSlider = new QSlider( Frame6, "CountSlider" );
    CountSlider->setMaxValue( 16 );
    CountSlider->setValue( 4 );
    CountSlider->setOrientation( Qt::Horizontal );
    CountLayout->addWidget( CountSlider );

    CountNumber = new QLCDNumber( Frame6, "CountNumber" );
    CountNumber->setPaletteBackgroundColor( QColor( 212, 212, 192 ) );
    CountNumber->setNumDigits( 2 );
    CountNumber->setSegmentStyle( QLCDNumber::Flat );
    CountNumber->setProperty( "value", 4 );
    CountLayout->addWidget( CountNumber );
    Layout20->addLayout( CountLayout );

    BarsLabel = new QLabel( Frame6, "BarsLabel" );
    BarsLabel->setText( trUtf8( "# bars" ) );
    Layout20->addWidget( BarsLabel );

    BarsLayout = new Q3HBoxLayout( 0, 0, 6, "BarsLayout");

    BarsSlider = new QSlider( Frame6, "BarsSlider" );
    BarsSlider->setMaxValue( 16 );
    BarsSlider->setValue( 1 );
    BarsSlider->setOrientation( Qt::Horizontal );
    BarsLayout->addWidget( BarsSlider );

    BarsNumber = new QLCDNumber( Frame6, "BarsNumber" );
    BarsNumber->setPaletteBackgroundColor( QColor( 212, 212, 192 ) );
    BarsNumber->setNumDigits( 2 );
    BarsNumber->setSegmentStyle( QLCDNumber::Flat );
    BarsNumber->setProperty( "value", 1 );
    BarsLayout->addWidget( BarsNumber );
    Layout20->addLayout( BarsLayout );
    Layout17->addLayout( Layout20 );
    Frame6Layout->addLayout( Layout17 );
    RhythmGeneratorLayout->addWidget( Frame6 );

    Layout27 = new Q3HBoxLayout( 0, 0, 15, "Layout27");

    Frame5 = new QFrame( this );
    Frame5->setFrameShape( QFrame::StyledPanel );
    Frame5->setFrameShadow( QFrame::Raised );
    Frame5Layout = new Q3VBoxLayout( Frame5, 11, 6, "Frame5Layout");

    TextLabel2 = new QLabel( Frame5, "TextLabel2" );
    QFont TextLabel2_font(  TextLabel2->font() );
    TextLabel2_font.setBold( TRUE );
    TextLabel2_font.setUnderline( TRUE );
    TextLabel2->setFont( TextLabel2_font );
    TextLabel2->setText( trUtf8( "Group Settings:" ) );
    Frame5Layout->addWidget( TextLabel2 );

    Layout20_2 = new Q3HBoxLayout( 0, 0, 15, "Layout20_2");

    GroupListLayout = new Q3VBoxLayout( 0, 0, 6, "GroupListLayout");

    GroupListLabel = new QLabel( Frame5, "GroupListLabel" );
    GroupListLabel->setText( trUtf8( "Group" ) );
    GroupListLayout->addWidget( GroupListLabel );

    GroupList = new Q3ListBox( Frame5, "GroupList" );
    GroupList->insertItem( trUtf8( "Group 1" ) );
    GroupList->insertItem( trUtf8( "Group 2" ) );
    GroupList->insertItem( trUtf8( "Group 3" ) );
    GroupList->insertItem( trUtf8( "Group 4" ) );
    GroupList->insertItem( trUtf8( "Group 5" ) );
    GroupList->setMinimumSize( QSize( 150, 90 ) );
    GroupListLayout->addWidget( GroupList );
    Layout20_2->addLayout( GroupListLayout );

    Layout19 = new Q3VBoxLayout( 0, 0, 6, "Layout19");

    ContribLabel = new QLabel( Frame5, "ContribLabel" );
    ContribLabel->setText( trUtf8( "contrib" ) );
    Layout19->addWidget( ContribLabel );

    ContribLayout = new Q3HBoxLayout( 0, 0, 6, "ContribLayout");

    ContribSlider = new QSlider( Frame5, "ContribSlider" );
    ContribSlider->setMinimumSize( QSize( 100, 0 ) );
    ContribSlider->setMaxValue( 100 );
    ContribSlider->setOrientation( Qt::Horizontal );
    ContribLayout->addWidget( ContribSlider );

    ContribNumber = new QLCDNumber( Frame5, "ContribNumber" );
    ContribNumber->setPaletteBackgroundColor( QColor( 212, 212, 192 ) );
    ContribNumber->setNumDigits( 3 );
    ContribNumber->setSegmentStyle( QLCDNumber::Flat );
    ContribLayout->addWidget( ContribNumber );
    Layout19->addLayout( ContribLayout );

    ListenLabel = new QLabel( Frame5, "ListenLabel" );
    ListenLabel->setText( trUtf8( "listen" ) );
    Layout19->addWidget( ListenLabel );

    ListenLayout = new Q3HBoxLayout( 0, 0, 6, "ListenLayout");

    ListenSlider = new QSlider( Frame5, "ListenSlider" );
    ListenSlider->setMinimumSize( QSize( 100, 0 ) );
    ListenSlider->setMinValue( -99 );
    ListenSlider->setMaxValue( 100 );
    ListenSlider->setOrientation( Qt::Horizontal );
    ListenLayout->addWidget( ListenSlider );

    ListenNumber = new QLCDNumber( Frame5, "ListenNumber" );
    ListenNumber->setPaletteBackgroundColor( QColor( 212, 212, 192 ) );
    ListenNumber->setNumDigits( 3 );
    ListenNumber->setSegmentStyle( QLCDNumber::Flat );
    ListenLayout->addWidget( ListenNumber );
    Layout19->addLayout( ListenLayout );
    Layout20_2->addLayout( Layout19 );
    Frame5Layout->addLayout( Layout20_2 );

    RandomizeCheck = new QCheckBox( Frame5, "RandomizeCheck" );
    RandomizeCheck->setText( trUtf8( "Randomize" ) );
    Frame5Layout->addWidget( RandomizeCheck );
    Layout27->addWidget( Frame5 );

    Frame5_2 = new QFrame( this );
    Frame5_2->setFrameShape( QFrame::StyledPanel );
    Frame5_2->setFrameShadow( QFrame::Raised );
    Frame5_2Layout = new Q3VBoxLayout( Frame5_2, 11, 6, "Frame5_2Layout");

    TextLabel3 = new QLabel( Frame5_2, "TextLabel3" );
    QFont TextLabel3_font(  TextLabel3->font() );
    TextLabel3_font.setBold( TRUE );
    TextLabel3_font.setUnderline( TRUE );
    TextLabel3->setFont( TextLabel3_font );
    TextLabel3->setText( trUtf8( "Rhythm Style:" ) );
    Frame5_2Layout->addWidget( TextLabel3 );

    Layout21 = new Q3VBoxLayout( 0, 0, 2, "Layout21");

    ToolButton7 = new QToolButton( Frame5_2, "ToolButton7" );
    ToolButton7->setText( trUtf8( "Clear" ) );
    Layout21->addWidget( ToolButton7 );

    ToolButton8 = new QToolButton( Frame5_2, "ToolButton8" );
    ToolButton8->setText( trUtf8( "Open..." ) );
    Layout21->addWidget( ToolButton8 );

    ToolButton9 = new QToolButton( Frame5_2, "ToolButton9" );
    ToolButton9->setText( trUtf8( "Save" ) );
    Layout21->addWidget( ToolButton9 );

    ToolButton10 = new QToolButton( Frame5_2, "ToolButton10" );
    ToolButton10->setText( trUtf8( "Save as..." ) );
    Layout21->addWidget( ToolButton10 );
    Frame5_2Layout->addLayout( Layout21 );

    Layout22 = new Q3VBoxLayout( 0, 0, 2, "Layout22");

    ToolButton5 = new QToolButton( Frame5_2, "ToolButton5" );
    ToolButton5->setText( trUtf8( "Generate" ) );
    Layout22->addWidget( ToolButton5 );

    ToolButton6 = new QToolButton( Frame5_2, "ToolButton6" );
    ToolButton6->setText( trUtf8( "Close" ) );
    Layout22->addWidget( ToolButton6 );
    Frame5_2Layout->addLayout( Layout22 );
    Layout27->addWidget( Frame5_2 );
    RhythmGeneratorLayout->addLayout( Layout27 );

    Layout29 = new Q3HBoxLayout( 0, 0, 6, "Layout29");

    Frame6_2 = new QFrame( this );
    Frame6_2->setMinimumSize( QSize( 200, 150 ) );
    Frame6_2->setFrameShape( QFrame::StyledPanel );
    Frame6_2->setFrameShadow( QFrame::Raised );
    Layout29->addWidget( Frame6_2 );

    Frame7 = new QFrame( this );
    Frame7->setMinimumSize( QSize( 200, 150 ) );
    Frame7->setFrameShape( QFrame::StyledPanel );
    Frame7->setFrameShadow( QFrame::Raised );
    Layout29->addWidget( Frame7 );
    RhythmGeneratorLayout->addLayout( Layout29 );

    Frame8 = new QFrame( this );
    Frame8->setMinimumSize( QSize( 400, 150 ) );
    Frame8->setFrameShape( QFrame::StyledPanel );
    Frame8->setFrameShadow( QFrame::Raised );
    RhythmGeneratorLayout->addWidget( Frame8 );

    // signals and slots connections
    connect( StepsSlider, SIGNAL( valueChanged(int) ), StepsNumber, SLOT( display(int) ) );
    connect( CountSlider, SIGNAL( valueChanged(int) ), CountNumber, SLOT( display(int) ) );
    connect( BarsSlider, SIGNAL( valueChanged(int) ), BarsNumber, SLOT( display(int) ) );
    connect( ContribSlider, SIGNAL( valueChanged(int) ), ContribNumber, SLOT( display(int) ) );
    connect( ListenSlider, SIGNAL( valueChanged(int) ), ListenNumber, SLOT( display(int) ) );
    connect( InstrumentDel, SIGNAL( pressed() ), InstrumentList, SLOT( clearSelection() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
RhythmGenerator::~RhythmGenerator()
{
    // no need to delete child widgets, Qt does it all for us
}

#endif

} // namespace MusEGui
