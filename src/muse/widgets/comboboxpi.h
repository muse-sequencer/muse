#ifndef COMBOBOXPI_H
#define COMBOBOXPI_H

#include <QComboBox>


namespace MusEGui {


class ComboBoxPI : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY( int id READ id WRITE setId )

    int pid;

     protected:
        void mousePressEvent(QMouseEvent *e);

     signals:
        void rightClicked(const QPoint &, int);

public:
    ComboBoxPI(QWidget* parent, int i, const char* name = 0);

    int id() const { return pid; };
    void setId(int i) { pid = i; };
};

}

#endif // COMBOBOXPI_H

