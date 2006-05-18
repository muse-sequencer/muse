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

#ifndef __TEMPLATE_DIALOG_H__
#define __TEMPLATE_DIALOG_H__

#include "ui_templatedialog.h"

//---------------------------------------------------------
//   TemplateDialog
//---------------------------------------------------------

class TemplateDialog : public QDialog, public Ui_TemplateDialogBase {
      Q_OBJECT

      void processSubdir(QTreeWidgetItem*, const QString&, 
         const QString&, int);

      QString itemPath(QTreeWidgetItem*) const;

   private slots:
      void currentChanged(QTreeWidgetItem*, QTreeWidgetItem*);
      void itemCollapsed(QTreeWidgetItem*);
      void itemExpanded(QTreeWidgetItem*);
      void itemDoubleClicked(QTreeWidgetItem*, int);

   public:
      TemplateDialog(QWidget* parent = 0);
      QString templatePath() const;
      };

#endif

