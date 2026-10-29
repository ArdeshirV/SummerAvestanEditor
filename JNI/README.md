# Qt + Android SAF via JNI

Replaces Qt's `FileDialog` / `QFileDialog` on Android with the native Storage Access Framework (SAF).
Tested design target: **Qt 6.2+**, Android API 21+ (SAF itself: API 19+, folder picker: API 21+).

```
saf-qt/
├── android/src/com/example/saf/SafHelper.java   <- Java side (static methods, no exceptions leak)
├── safbridge.h / safbridge.cpp                  <- Qt side (QJniObject wrapper, QObject with signals)
├── DocumentSaver.qml                            <- Save / Save As logic (QML), ready to use
└── README.md
```

## 1. Wire it into the build (CMake)

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Quick)   # + Widgets if needed

if(ANDROID)
    target_sources(appMyApp PRIVATE safbridge.cpp safbridge.h)
    target_link_libraries(appMyApp PRIVATE Qt6::CorePrivate)   # for qandroidextras_p.h

    # Qt's Gradle build compiles every .java found under <dir>/src
    set_property(TARGET appMyApp APPEND PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)
endif()
```

qmake equivalent:

```qmake
android {
    QT += core-private
    SOURCES += safbridge.cpp
    HEADERS += safbridge.h
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}
```

No manifest permission is needed: SAF grants access per picked URI.

If you rename the Java package, change `kHelper` in `safbridge.cpp` too
(`com.example.saf.SafHelper` -> `"com/example/saf/SafHelper"`).

## 2. How the JNI call works

```cpp
QJniObject::callStaticMethod<jint>(
    "com/example/saf/SafHelper",                    // class, '/' separated
    "openFd",                                       // Java method name
    "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",  // signature
    context.object(), jstring1, jstring2);          // arguments
```

Signature cheat-sheet: `Ljava/lang/String;` = String, `[Ljava/lang/String;` = String[],
`Z` = boolean, `I` = int, `J` = long, `V` = void, `Landroid/content/Intent;` = Intent.
Format: `(arguments)returnType`.

Rules used in this code:
- Java methods are **static** and never throw, so C++ needs no exception handling.
- Only simple types cross the boundary. Heavy logic stays in Java.
- Java returns `null` -> `QJniObject::isValid()` is false in C++.

## 3. The flow of one "Open file" call

1. C++ `openDocument()` -> JNI `SafHelper.buildOpenDocumentIntent(...)` returns an `Intent`.
2. C++ `QtAndroidPrivate::startActivity(intent, requestCode, this)` shows the system picker.
3. User picks -> Qt calls `SafBridge::handleActivityResult(...)`.
4. C++ -> JNI `SafHelper.extractUris(ctx, data, true)` returns `content://...` strings
   and takes a persistable permission.
5. C++ emits `documentsPicked(uris)`.
6. You read the file with `readAll(uri)` or `openFd(uri)` + `QFile`.

## 4. Use it in QML

`main.cpp`:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#ifdef Q_OS_ANDROID
#include "safbridge.h"
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

#ifdef Q_OS_ANDROID
    SafBridge saf;
    engine.rootContext()->setContextProperty("saf", &saf);
#endif
    engine.load(QUrl(u"qrc:/Main.qml"_qs));
    return app.exec();
}
```

`Main.qml`:

```qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 400; height: 600

    Column {
        spacing: 12
        Button { text: "Open text file"; onClicked: saf.openDocument(["text/plain"], false) }
        Button { text: "Export copy...";  onClicked: saf.createDocument("note.txt", "text/plain") }  // see section 7
        Button { text: "Pick folder";    onClicked: saf.openTree() }
        Label  { id: out; wrapMode: Text.Wrap; width: 360 }
    }

    Connections {
        target: saf

        function onDocumentsPicked(uris) {
            const uri = uris[0]
            const bytes = saf.readAll(uri)           // QByteArray -> use C++ side or convert
            out.text = saf.displayName(uri) + "  (" + saf.size(uri) + " bytes)"
        }
        function onDocumentCreated(uri) {
            saf.writeAll(uri, "hello SAF")           // string converts to QByteArray
            out.text = "saved: " + saf.displayName(uri)
        }
        function onTreePicked(uri) {
            const entries = saf.listChildren(uri)    // "uri\tname\tmime\tisDir"
            out.text = entries.join("\n")
        }
        function onCancelled() { out.text = "cancelled" }
    }
}
```

## 5. Use it in C++ / Widgets

```cpp
SafBridge saf;

QObject::connect(&saf, &SafBridge::documentsPicked, [&](const QStringList &uris) {
    const QString uri = uris.first();
    QByteArray data = saf.readAll(uri);
    qDebug() << saf.displayName(uri) << data.size();
});

QObject::connect(&saf, &SafBridge::documentCreated, [&](const QString &uri) {
    saf.writeAll(uri, "Hello from Qt\n");
});

saf.openDocument({"text/plain", "text/csv"}, /*allowMultiple=*/false);
```

Streaming big files without loading them into memory:

```cpp
int fd = saf.openFd(uri, "r");
QFile f;
f.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle);
QTextStream in(&f);
```

Library that needs a path (SQLite, ffmpeg, ...): `saf.copyToCache(uri)` returns a normal file path.

## 6. Gotchas

- **Never convert `content://` URIs to filesystem paths** (no `/storage/emulated/0/...` guessing).
  Use the fd or the cache copy.
- **Writing:** use mode `"wt"`; plain `"w"` may leave old trailing bytes on some providers.
- **Persisting access:** `extractUris(..., true)` already calls `takePersistableUriPermission`.
  Store the URI string in `QSettings`; check it with `persistedUris()` on next start.
  Android keeps max ~128 (older: 128, newer: 512) persisted grants per app.
- **Folder picker limits (Android 11+):** cannot select the storage root, `Download`, or `Android/data`.
- **Request codes** (`0x5AF1..3`) must not clash with other activity results in your app.
- **Private API:** `qandroidextras_p.h` is a Qt private header; it can change between Qt versions.
  It has been stable across Qt 6.2-6.x.
- **R8/ProGuard** (if you enable minify in release): add
  `-keep class com.example.saf.SafHelper { *; }`
- **Qt 5.15:** replace `QJniObject` with `QAndroidJniObject`, `QtAndroidPrivate::startActivity`
  with `QtAndroid::startActivity`, `QNativeInterface::QAndroidApplication::context()` with
  `QtAndroid::androidContext()`, include `<QtAndroid>` and `QT += androidextras`.
- **Qt 6 bonus:** `QFile` can already open `content://...` URIs for plain read/write, but only the
  picker + persisted permission (this code) makes the URI usable later.

## 7. Save As: why `createDocument` was wrong, and the fix

**The problem.** `ACTION_CREATE_DOCUMENT` (`saf.createDocument`) always creates a *new* document.
If the name is taken, the provider silently makes `note (1).txt`. The app is never told, cannot
detect that the file existed, cannot recognise "this is the document I already have open", and
cannot overwrite. No intent flag changes this, so it cannot be repaired on top of that intent.

**The fix.** Split Save As into steps the app controls:

1. `saf.pickSaveFolder()`: the system chooser (starts in the last used folder).
   Signal `saveFolderPicked(folderUri)`.
2. The app shows its own file-name dialog (pre-filled with the current name).
3. `saf.resolveSaveTarget(folderUri, name, currentUri)` looks the name up in that folder and returns
   `{ status, uri, existingName }`:

| status   | meaning                                   | what the app does                         |
|----------|-------------------------------------------|-------------------------------------------|
| `new`    | no such file in the folder                | `createFileExact()` then `writeAll()`     |
| `same`   | exists and IS the document open in the app| `writeAll(currentUri)` silently           |
| `exists` | exists, but it is another document        | ask "Overwrite?"; Yes -> `writeAll(uri)`, No -> back to the name field |
| `error`  | empty / invalid name, folder unreadable   | show an error                             |

"Same document" is decided by provider + document id (`isSameDocument`), not by comparing strings,
so it works even if the open file came from the file picker and the target comes from the folder tree.

`createFileExact()` guarantees the exact name: if the provider appends an extension or changes
the name it renames the file, and if that fails it deletes the empty file and reports an error.
Nothing is ever auto-numbered.

**Use the ready-made controller:**

```qml
DocumentSaver {
    id: saver
    documentBytes: function() { return editor.text }
    onSaved: (uri, name) => statusLabel.text = "Saved " + name
    onFailed: (msg) => errorLabel.text = msg
}
Button { text: "Save";    onClicked: saver.save() }
Button { text: "Save As"; onClicked: saver.saveAs() }
```

When you open a file with `saf.openDocument(...)`, set `saver.currentUri = uri` and
`saver.currentName = saf.displayName(uri)` so "Save" and the same-file check work.

**The same in C++:**

```cpp
connect(&saf, &SafBridge::saveFolderPicked, this, [&](const QString &folder) {
    QString name = askFileName(currentName);                       // your QInputDialog etc.
    QVariantMap r = saf.resolveSaveTarget(folder, name, currentUri);
    const QString status = r["status"].toString();

    if (status == "new") {
        QString uri = saf.createFileExact(folder, "text/plain", name);
        if (!uri.isEmpty() && saf.writeAll(uri, data)) currentUri = uri;
    } else if (status == "same") {
        saf.writeAll(currentUri, data);                            // silent save
    } else if (status == "exists" && confirmOverwrite(r["existingName"].toString())) {
        if (saf.writeAll(r["uri"].toString(), data)) currentUri = r["uri"].toString();
    }
});
saf.pickSaveFolder();
```

Notes:
- The folder picker is a *folder* chooser: the user cannot type the name inside the system dialog.
  That is the price of being able to detect and handle collisions; every SAF app that offers real
  overwrite confirmation works this way.
- Overwrite uses mode `"wt"`. SAF has no atomic replace, so if the app is killed mid-write the old
  content is already truncated. For critical data, write to a temp copy first and verify.
- Android 11+ forbids choosing the storage root, `Download`, and `Android/data` as a folder.
- `lastFolder` is stored in `QSettings`, so set `QCoreApplication::setOrganizationName/ApplicationName`.
