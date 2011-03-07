
#ifndef __TRANSPOSE_H__
#define __TRANSPOSE_H__

#include "transposebase.h"

//---------------------------------------------------------
//   transpose widget
//---------------------------------------------------------

class Transpose : public TransposeDialogBase {
      Q_OBJECT

   private slots:
      virtual void accept();

   public:
      Transpose(QWidget* parent=0, const char* name=0);
      };

#endif
