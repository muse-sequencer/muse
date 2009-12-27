#include "editmetadialogbase.h"

#include <qvariant.h>
#include <awl/posedit.h>
/*
 *  Constructs a EditMetaDialogBase as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
EditMetaDialogBase::EditMetaDialogBase(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
EditMetaDialogBase::~EditMetaDialogBase()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void EditMetaDialogBase::languageChange()
{
    retranslateUi(this);
}

/****************************************************************************
** Meta object code from reading C++ file 'editmetadialogbase.h'
**
** Created: Fri Sep 23 12:43:18 2005
**      by: The Qt Meta Object Compiler version 58 (Qt 4.0.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "editmetadialogbase.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'editmetadialogbase.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 58
#error "This file was generated using the moc from 4.0.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_EditMetaDialogBase[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_EditMetaDialogBase[] = {
    "EditMetaDialogBase\0\0languageChange()\0"
};

const QMetaObject EditMetaDialogBase::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_EditMetaDialogBase,
      qt_meta_data_EditMetaDialogBase, 0 }
};

const QMetaObject *EditMetaDialogBase::metaObject() const
{
    return &staticMetaObject;
}

void *EditMetaDialogBase::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_EditMetaDialogBase))
	return static_cast<void*>(const_cast<EditMetaDialogBase*>(this));
    if (!strcmp(_clname, "Ui::EditMetaDialogBase"))
	return static_cast<Ui::EditMetaDialogBase*>(const_cast<EditMetaDialogBase*>(this));
    return QDialog::qt_metacast(_clname);
}

int EditMetaDialogBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: languageChange(); break;
        }
        _id -= 1;
    }
    return _id;
}
