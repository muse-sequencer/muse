#include "pctablemodel.h"
#include <QTableView>

ProgramChangeTableModel::ProgramChangeTableModel(QObject *parent) : QStandardItemModel(parent)
{
}

QStringList ProgramChangeTableModel::mimeTypes() 
{
	QStringList list;
	list << "text/plain";
	return list;
}

void ProgramChangeTableModel::emit_layoutChanged()
{
	emit layoutChanged();
}
