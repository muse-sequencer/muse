/****************************************************************************
** Meta object code from reading C++ file 'songinfo.h'
**
** Created: Wed Oct 13 19:43:37 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "songinfo.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'songinfo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SongInfo[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x0a,
      29,    9,    9,    9, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_SongInfo[] = {
    "SongInfo\0\0buttonOk_clicked()\0"
    "languageChange()\0"
};

const QMetaObject SongInfo::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_SongInfo,
      qt_meta_data_SongInfo, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SongInfo::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SongInfo::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SongInfo::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SongInfo))
        return static_cast<void*>(const_cast< SongInfo*>(this));
    if (!strcmp(_clname, "Ui::SongInfo"))
        return static_cast< Ui::SongInfo*>(const_cast< SongInfo*>(this));
    return QDialog::qt_metacast(_clname);
}

int SongInfo::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: buttonOk_clicked(); break;
        case 1: languageChange(); break;
        default: ;
        }
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
