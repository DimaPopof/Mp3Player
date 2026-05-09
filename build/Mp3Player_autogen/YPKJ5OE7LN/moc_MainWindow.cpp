/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/MainWindow.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[42];
    char stringdata0[11];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[19];
    char stringdata4[15];
    char stringdata5[11];
    char stringdata6[14];
    char stringdata7[18];
    char stringdata8[16];
    char stringdata9[13];
    char stringdata10[6];
    char stringdata11[15];
    char stringdata12[11];
    char stringdata13[15];
    char stringdata14[11];
    char stringdata15[10];
    char stringdata16[9];
    char stringdata17[12];
    char stringdata18[13];
    char stringdata19[11];
    char stringdata20[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 16),  // "openFolderDialog"
        QT_MOC_LITERAL(28, 0),  // ""
        QT_MOC_LITERAL(29, 18),  // "openConstantFolder"
        QT_MOC_LITERAL(48, 14),  // "togglePlayback"
        QT_MOC_LITERAL(63, 10),  // "toggleMute"
        QT_MOC_LITERAL(74, 13),  // "playNextTrack"
        QT_MOC_LITERAL(88, 17),  // "playPreviousTrack"
        QT_MOC_LITERAL(106, 15),  // "onTrackFinished"
        QT_MOC_LITERAL(122, 12),  // "changeVolume"
        QT_MOC_LITERAL(135, 5),  // "value"
        QT_MOC_LITERAL(141, 14),  // "updateDuration"
        QT_MOC_LITERAL(156, 10),  // "durationMs"
        QT_MOC_LITERAL(167, 14),  // "updatePosition"
        QT_MOC_LITERAL(182, 10),  // "positionMs"
        QT_MOC_LITERAL(193, 9),  // "seekAudio"
        QT_MOC_LITERAL(203, 8),  // "position"
        QT_MOC_LITERAL(212, 11),  // "skipForward"
        QT_MOC_LITERAL(224, 12),  // "skipBackward"
        QT_MOC_LITERAL(237, 10),  // "zoomInText"
        QT_MOC_LITERAL(248, 11)   // "zoomOutText"
    },
    "MainWindow",
    "openFolderDialog",
    "",
    "openConstantFolder",
    "togglePlayback",
    "toggleMute",
    "playNextTrack",
    "playPreviousTrack",
    "onTrackFinished",
    "changeVolume",
    "value",
    "updateDuration",
    "durationMs",
    "updatePosition",
    "positionMs",
    "seekAudio",
    "position",
    "skipForward",
    "skipBackward",
    "zoomInText",
    "zoomOutText"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x08,    1 /* Private */,
       3,    0,  105,    2, 0x08,    2 /* Private */,
       4,    0,  106,    2, 0x08,    3 /* Private */,
       5,    0,  107,    2, 0x08,    4 /* Private */,
       6,    0,  108,    2, 0x08,    5 /* Private */,
       7,    0,  109,    2, 0x08,    6 /* Private */,
       8,    0,  110,    2, 0x08,    7 /* Private */,
       9,    1,  111,    2, 0x08,    8 /* Private */,
      11,    1,  114,    2, 0x08,   10 /* Private */,
      13,    1,  117,    2, 0x08,   12 /* Private */,
      15,    1,  120,    2, 0x08,   14 /* Private */,
      17,    0,  123,    2, 0x08,   16 /* Private */,
      18,    0,  124,    2, 0x08,   17 /* Private */,
      19,    0,  125,    2, 0x08,   18 /* Private */,
      20,    0,  126,    2, 0x08,   19 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::LongLong,   12,
    QMetaType::Void, QMetaType::LongLong,   14,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'openFolderDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openConstantFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'togglePlayback'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleMute'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playNextTrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playPreviousTrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTrackFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'changeVolume'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'updateDuration'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'updatePosition'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'seekAudio'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'skipForward'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'skipBackward'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'zoomInText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'zoomOutText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->openFolderDialog(); break;
        case 1: _t->openConstantFolder(); break;
        case 2: _t->togglePlayback(); break;
        case 3: _t->toggleMute(); break;
        case 4: _t->playNextTrack(); break;
        case 5: _t->playPreviousTrack(); break;
        case 6: _t->onTrackFinished(); break;
        case 7: _t->changeVolume((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->updateDuration((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 9: _t->updatePosition((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 10: _t->seekAudio((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->skipForward(); break;
        case 12: _t->skipBackward(); break;
        case 13: _t->zoomInText(); break;
        case 14: _t->zoomOutText(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
