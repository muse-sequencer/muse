//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
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

#ifndef __ALLQT_H__
#define __ALLQT_H__

#include <stdio.h>
#ifndef __APPLE__
#include <values.h>
#endif
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <cmath>
#include <list>
#include <vector>
#include <map>

#include <QtCore/qplugin.h>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QBuffer>
#include <QtCore/QLocale>
#include <QtCore/QTimer>
#include <QtCore/QSocketNotifier>
#include <QtCore/QSignalMapper>
#include <QtCore/QTime>
#include <QtCore/QMetaProperty>
#include <QtCore/QEvent>
#include <QtCore/QTranslator>
#include <QtCore/qatomic.h>
#include <QtCore/QTemporaryFile>

#include <QtDesigner/QDesignerCustomWidgetInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QFormBuilder>

#include <QtXml/QDomNode>

#include <QtGui/QScrollBar>
#include <QtGui/QToolBar>
#include <QtGui/QButtonGroup>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QRadioButton>
#include <QtGui/QScrollArea>
#include <QtGui/QListWidgetItem>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QPainterPath>
#include <QtGui/QStackedWidget>
#include <QtGui/QShortcut>
#include <QtGui/QTableView>
#include <QtGui/QTextEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QProgressDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QStyle>
#include <QtGui/QWindowsStyle>
#include <QtCore/QProcess>
#include <QtGui/QWhatsThis>
#include <QtGui/QDial>
#include <QtGui/QPaintEvent>
#include <QtGui/QPen>
#include <QtGui/QPainter>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QApplication>
#include <QtGui/QSpinBox>
#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QPrinter>
#include <QtGui/QGridLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QAction>
#include <QtGui/QColor>
#include <QtGui/QColorDialog>
#include <QtGui/QCursor>
#include <QtGui/QFont>
#include <QtGui/QFontInfo>
#include <QtGui/QFontMetrics>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>
#include <QtGui/QMenu>
#include <QtGui/QFontDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QSplitter>
#include <QtGui/QMenuBar>
#include <QtGui/QSizeGrip>
#include <QtGui/QClipboard>
#include <QtGui/QCloseEvent>
#include <QtGui/QSplashScreen>
#include <QtGui/QStyleFactory>
#include <QtGui/QDockWidget>

#endif

