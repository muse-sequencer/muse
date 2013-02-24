#ifndef COLORFRAME_H
#define COLORFRAME_H

#include <QWidget>

class ColorFrame : public QWidget
{
    Q_OBJECT
    virtual void paintEvent(QPaintEvent*);

public:
    explicit ColorFrame(QWidget *parent = 0);
    void setColor(QColor c) {color = c; update();}
    
signals:
    
public slots:

private:
    QColor color;
};

#endif // COLORFRAME_H
