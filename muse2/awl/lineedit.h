#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget *parent = 0);
    void focusOutEvent ( QFocusEvent * e );
signals:

public slots:

};

#endif // LINEEDIT_H
