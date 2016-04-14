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
    QColor color() const { return _color; }
    void setColor(QColor c) {_color = c; update();}
    
signals:
    void clicked();
    
public slots:

private:
    QColor _color;
};

#endif // COLORFRAME_H
