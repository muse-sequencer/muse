#ifndef COLORFRAME_H
#define COLORFRAME_H

#include <QWidget>

class ColorFrame : public QWidget
{
    Q_OBJECT
protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void mousePressEvent(QMouseEvent*);

public:
    explicit ColorFrame(QWidget *parent = 0);
    void setColor(QColor c) {color = c; update();}
    
signals:
    void clicked();
    
public slots:

private:
    QColor color;
};

#endif // COLORFRAME_H
