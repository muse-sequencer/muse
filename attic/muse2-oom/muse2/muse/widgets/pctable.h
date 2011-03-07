#ifndef _PCTABLE_
#define _PCTABLE_

#include <QTableView>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QPaintEvent>
#include <QList>

class ProgramChangeTable : public QTableView
{
	Q_OBJECT
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dragMoveEvent(QDragMoveEvent*);
	virtual void paintEvent(QPaintEvent*);
	QRect dropSite;

	signals:
		void rowOrderChanged();

	public:
		ProgramChangeTable(QWidget *parent = 0);
		void dropEvent(QDropEvent *evt);
		void mousePressEvent(QMouseEvent* evt);

	public slots:
		QList<int> getSelectedRows();
};
#endif
