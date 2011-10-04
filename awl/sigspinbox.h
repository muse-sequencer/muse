#ifndef SIGSPINBOX_H
#define SIGSPINBOX_H

#include <QSpinBox>

class SigSpinBox : public QSpinBox
{
    Q_OBJECT

protected:
    virtual void keyPressEvent(QKeyEvent*);
public:
    explicit SigSpinBox(QWidget *parent = 0);

signals:
    void returnPressed();
    void moveFocus();

public slots:

};

#endif // SIGSPINBOX_H
