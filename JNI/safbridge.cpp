#include "safbridge.h"

#include <QCoreApplication>
#include <QFile>
#include <QJniEnvironment>
#include <QJniObject>
#include <QDebug>
#include <QSettings>

namespace {

// JNI name of the Java class: package with '/' instead of '.'
constexpr const char *kHelper = "com/example/saf/SafHelper";

// Activity (or application) context needed by the Java helper.
QJniObject appContext()
{
    return QNativeInterface::QAndroidApplication::context();
}

// QString -> java.lang.String
QJniObject jstr(const QString &s)
{
    return QJniObject::fromString(s);
}

// QStringList -> java.lang.String[]
QJniObject toJavaStringArray(const QStringList &list)
{
    QJniEnvironment env;
    jclass strCls = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(list.size(), strCls, nullptr);
    for (int i = 0; i < list.size(); ++i) {
        QJniObject s = QJniObject::fromString(list.at(i));
        env->SetObjectArrayElement(arr, i, s.object<jstring>());
    }
    env->DeleteLocalRef(strCls);
    QJniObject result(arr);          // QJniObject creates its own global ref
    env->DeleteLocalRef(arr);
    return result;
}

// java.lang.String[] -> QStringList
QStringList toQStringList(const QJniObject &javaArray)
{
    QStringList out;
    if (!javaArray.isValid())
        return out;
    QJniEnvironment env;
    auto arr = javaArray.object<jobjectArray>();
    const jsize n = env->GetArrayLength(arr);
    for (jsize i = 0; i < n; ++i) {
        QJniObject s = QJniObject::fromLocalRef(env->GetObjectArrayElement(arr, i));
        out << s.toString();
    }
    return out;
}

} // namespace

SafBridge::SafBridge(QObject *parent) : QObject(parent) {}

// ============================================================================
// Pickers
// ============================================================================

void SafBridge::startPicker(const QJniObject &intent, int requestCode)
{
    if (!intent.isValid()) {
        emit cancelled();
        return;
    }
    // Qt routes onActivityResult(requestCode, ...) to handleActivityResult() of 'this'
    QtAndroidPrivate::startActivity(intent, requestCode, this);
}

void SafBridge::openDocument(const QStringList &mimeTypes, bool allowMultiple)
{
    QJniObject types = toJavaStringArray(mimeTypes);
    QJniObject intent = QJniObject::callStaticObjectMethod(
        kHelper, "buildOpenDocumentIntent",
        "([Ljava/lang/String;Z)Landroid/content/Intent;",
        types.object<jobjectArray>(), jboolean(allowMultiple));
    startPicker(intent, ReqOpen);
}

void SafBridge::createDocument(const QString &suggestedName, const QString &mimeType)
{
    QJniObject intent = QJniObject::callStaticObjectMethod(
        kHelper, "buildCreateDocumentIntent",
        "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
        jstr(mimeType).object<jstring>(), jstr(suggestedName).object<jstring>());
    startPicker(intent, ReqCreate);
}

void SafBridge::openTree()
{
    QJniObject intent = QJniObject::callStaticObjectMethod(
        kHelper, "buildOpenTreeIntent", "()Landroid/content/Intent;");
    startPicker(intent, ReqTree);
}

void SafBridge::handleActivityResult(int requestCode, int resultCode, const QJniObject &data)
{
    if (requestCode != ReqOpen && requestCode != ReqCreate
        && requestCode != ReqTree && requestCode != ReqSaveFolder)
        return;                                   // not ours

    constexpr int RESULT_OK = -1;                 // android.app.Activity.RESULT_OK
    if (resultCode != RESULT_OK || !data.isValid()) {
        emit cancelled();
        return;
    }

    QJniObject arr = QJniObject::callStaticObjectMethod(
        kHelper, "extractUris",
        "(Landroid/content/Context;Landroid/content/Intent;Z)[Ljava/lang/String;",
        appContext().object(), data.object(), jboolean(true) /* persist permission */);

    const QStringList uris = toQStringList(arr);
    if (uris.isEmpty()) {
        emit cancelled();
        return;
    }

    switch (requestCode) {
    case ReqOpen:   emit documentsPicked(uris);       break;
    case ReqCreate: emit documentCreated(uris.first()); break;
    case ReqTree:   emit treePicked(uris.first());    break;
    case ReqSaveFolder:
        QSettings().setValue(QStringLiteral("SafBridge/lastFolder"), uris.first());
        emit saveFolderPicked(uris.first());
        break;
    }
}

// ============================================================================
// Synchronous helpers
// ============================================================================

QString SafBridge::displayName(const QString &uri) const
{
    QJniObject r = QJniObject::callStaticObjectMethod(
        kHelper, "getDisplayName",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        appContext().object(), jstr(uri).object<jstring>());
    return r.isValid() ? r.toString() : QString();
}

qint64 SafBridge::size(const QString &uri) const
{
    return QJniObject::callStaticMethod<jlong>(
        kHelper, "getSize",
        "(Landroid/content/Context;Ljava/lang/String;)J",
        appContext().object(), jstr(uri).object<jstring>());
}

QString SafBridge::mimeType(const QString &uri) const
{
    QJniObject r = QJniObject::callStaticObjectMethod(
        kHelper, "getMimeType",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        appContext().object(), jstr(uri).object<jstring>());
    return r.isValid() ? r.toString() : QString();
}

int SafBridge::openFd(const QString &uri, const QString &mode) const
{
    return QJniObject::callStaticMethod<jint>(
        kHelper, "openFd",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",
        appContext().object(), jstr(uri).object<jstring>(), jstr(mode).object<jstring>());
}

QByteArray SafBridge::readAll(const QString &uri) const
{
    const int fd = openFd(uri, QStringLiteral("r"));
    if (fd < 0)
        return {};
    QFile f;
    // AutoCloseHandle: QFile closes the detached fd for us
    if (!f.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle))
        return {};
    return f.readAll();
}

bool SafBridge::writeAll(const QString &uri, const QByteArray &data) const
{
    const int fd = openFd(uri, QStringLiteral("wt"));   // "wt" = write + truncate
    if (fd < 0)
        return false;
    QFile f;
    if (!f.open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle))
        return false;
    const bool ok = f.write(data) == data.size();
    f.close();
    return ok;
}

QString SafBridge::copyToCache(const QString &uri) const
{
    QJniObject r = QJniObject::callStaticObjectMethod(
        kHelper, "copyToCache",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        appContext().object(), jstr(uri).object<jstring>());
    return r.isValid() ? r.toString() : QString();
}

QStringList SafBridge::persistedUris() const
{
    QJniObject arr = QJniObject::callStaticObjectMethod(
        kHelper, "getPersistedUris",
        "(Landroid/content/Context;)[Ljava/lang/String;",
        appContext().object());
    return toQStringList(arr);
}

void SafBridge::releasePermission(const QString &uri)
{
    QJniObject::callStaticMethod<void>(
        kHelper, "releasePermission",
        "(Landroid/content/Context;Ljava/lang/String;)V",
        appContext().object(), jstr(uri).object<jstring>());
}

QStringList SafBridge::listChildren(const QString &uri) const
{
    QJniObject arr = QJniObject::callStaticObjectMethod(
        kHelper, "listChildren",
        "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
        appContext().object(), jstr(uri).object<jstring>());
    return toQStringList(arr);
}

QString SafBridge::createFileInTree(const QString &folderUri, const QString &mime,
                                    const QString &name) const
{
    QJniObject r = QJniObject::callStaticObjectMethod(
        kHelper, "createFileInTree",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)"
        "Ljava/lang/String;",
        appContext().object(), jstr(folderUri).object<jstring>(),
        jstr(mime).object<jstring>(), jstr(name).object<jstring>());
    return r.isValid() ? r.toString() : QString();
}

bool SafBridge::deleteDocument(const QString &uri) const
{
    return QJniObject::callStaticMethod<jboolean>(
        kHelper, "deleteDocument",
        "(Landroid/content/Context;Ljava/lang/String;)Z",
        appContext().object(), jstr(uri).object<jstring>());
}

// ============================================================================
// Save As
// ============================================================================

QString SafBridge::lastFolder() const
{
    return QSettings().value(QStringLiteral("SafBridge/lastFolder")).toString();
}

void SafBridge::pickSaveFolder()
{
    QJniObject intent = QJniObject::callStaticObjectMethod(
        kHelper, "buildPickFolderIntent",
        "(Ljava/lang/String;)Landroid/content/Intent;",
        jstr(lastFolder()).object<jstring>());
    startPicker(intent, ReqSaveFolder);
}

QVariantMap SafBridge::resolveSaveTarget(const QString &folderUri, const QString &fileName,
                                         const QString &currentDocumentUri) const
{
    QJniObject arr = QJniObject::callStaticObjectMethod(
        kHelper, "resolveSaveTarget",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)"
        "[Ljava/lang/String;",
        appContext().object(), jstr(folderUri).object<jstring>(),
        jstr(fileName).object<jstring>(), jstr(currentDocumentUri).object<jstring>());

    const QStringList r = toQStringList(arr);
    QVariantMap map;
    map[QStringLiteral("status")]       = r.value(0, QStringLiteral("error"));
    map[QStringLiteral("uri")]          = r.value(1);   // existing document (for "same"/"exists")
    map[QStringLiteral("existingName")] = r.value(2);   // its real name, may differ in letter case
    return map;
}

QString SafBridge::createFileExact(const QString &folderUri, const QString &mimeType,
                                   const QString &fileName) const
{
    QJniObject r = QJniObject::callStaticObjectMethod(
        kHelper, "createFileExact",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)"
        "Ljava/lang/String;",
        appContext().object(), jstr(folderUri).object<jstring>(),
        jstr(mimeType).object<jstring>(), jstr(fileName).object<jstring>());
    return r.isValid() ? r.toString() : QString();
}

bool SafBridge::isSameDocument(const QString &uriA, const QString &uriB) const
{
    return QJniObject::callStaticMethod<jboolean>(
        kHelper, "isSameDocument",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
        appContext().object(), jstr(uriA).object<jstring>(), jstr(uriB).object<jstring>());
}
