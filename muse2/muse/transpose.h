
#ifndef __TRANSPOSE_H__
#define __TRANSPOSE_H__

#include "ui_transposebase.h"

class QButtonGroup;

//---------------------------------------------------------
//   transpose widget
//---------------------------------------------------------

class Transpose : public QDialog, public Ui::TransposeDialogBase {
      Q_OBJECT

     QButtonGroup* buttonGroup1;
     QButtonGroup* buttonGroup2;

   private slots:
      virtual void accept();

   public:
      Transpose(QWidget* parent=0);
      };

#endif
