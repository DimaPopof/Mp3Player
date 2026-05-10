/****************************************************************************
** Meta object code from reading C++ file 'ThemeManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/ThemeManager.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ThemeManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_ThemeManager_t {
    uint offsetsAndSizes[14];
    char stringdata0[13];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[8];
    char stringdata4[6];
    char stringdata5[18];
    char stringdata6[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_ThemeManager_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_ThemeManager_t qt_meta_stringdata_ThemeManager = {
    {
        QT_MOC_LITERAL(0, 12),  // "ThemeManager"
        QT_MOC_LITERAL(13, 12),  // "themeChanged"
        QT_MOC_LITERAL(26, 0),  // ""
        QT_MOC_LITERAL(27, 7),  // "ThemeId"
        QT_MOC_LITERAL(35, 5),  // "theme"
        QT_MOC_LITERAL(41, 17),  // "DarkGlassmorphism"
        QT_MOC_LITERAL(59, 11)   // "PastelDream"
    },
    "ThemeManager",
    "themeChanged",
    "",
    "ThemeId",
    "theme",
    "DarkGlassmorphism",
    "PastelDream"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_ThemeManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       1,   23, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // enums: name, alias, flags, count, data
       3,    3, 0x2,    2,   28,

 // enum data: key, value
       5, uint(ThemeManager::ThemeId::DarkGlassmorphism),
       6, uint(ThemeManager::ThemeId::PastelDream),

       0        // eod
};

Q_CONSTINIT const QMetaObject ThemeManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ThemeManager.offsetsAndSizes,
    qt_meta_data_ThemeManager,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_ThemeManager_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ThemeManager, std::true_type>,
        // method 'themeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ThemeId, std::false_type>
    >,
    nullptr
} };

void ThemeManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ThemeManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->themeChanged((*reinterpret_cast< std::add_pointer_t<ThemeId>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ThemeManager::*)(ThemeId );
            if (_t _q_method = &ThemeManager::themeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *ThemeManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ThemeManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ThemeManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ThemeManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void ThemeManager::themeChanged(ThemeId _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
