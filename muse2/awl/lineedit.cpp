#include <stdio.h>
#include "lineedit.h"

LineEdit::LineEdit(QWidget *parent) :
    QLineEdit(parent)
{
}
void LineEdit::focusOutEvent ( QFocusEvent * e )
{
    emit returnPressed();
}
