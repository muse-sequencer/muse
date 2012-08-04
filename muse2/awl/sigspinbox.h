#ifndef SIGSPINBOX_H
#define SIGSPINBOX_H

#include <QSpinBox>

class SigSpinBox : public QSpinBox
{
    Q_OBJECT
    bool _denominator;
protected:
    virtual void keyPressEvent(QKeyEvent*);
    virtual void stepBy(int);
public:
    explicit SigSpinBox(QWidget *parent = 0);
    void setDenominator();

signals:
    void returnPressed();
    void escapePressed();
    void moveFocus();

public slots:

};

#endif // SIGSPINBOX_H
