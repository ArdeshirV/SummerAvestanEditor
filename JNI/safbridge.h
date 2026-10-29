#pragma once

// Qt 6 (6.2+), Android only.  Add this file to your target only when ANDROID.
// Needs:  target_link_libraries(app PRIVATE Qt6::CorePrivate)

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QVariantMap>
#include <QtCore/private/qandroidextras_p.h>   // QAndroidActivityResultReceiver, QtAndroidPrivate

class SafBridge : public QObject, public QAndroidActivityResultReceiver
{
    Q_OBJECT
public:
    explicit SafBridge(QObject *parent = nullptr);

    // ---- asynchronous: show the system picker, result arrives via signals ----
    Q_INVOKABLE void openDocument(const QStringList &mimeTypes = {}, bool allowMultiple = false);
    Q_INVOKABLE void createDocument(const QString &suggestedName,
                                    const QString &mimeType = QStringLiteral("application/octet-stream"));
    Q_INVOKABLE void openTree();

    // ---- proper "Save As" (replaces createDocument for this purpose) ----
    // 1) pickSaveFolder()           -> signal saveFolderPicked(folderUri)
    // 2) your own file-name dialog
    // 3) resolveSaveTarget(...)     -> { status: "new"|"same"|"exists"|"error", uri, existingName }
    // 4) "new"    -> createFileExact(...) then writeAll(newUri, data)
    //    "same"   -> writeAll(currentDocumentUri, data)            (silent save)
    //    "exists" -> ask the user; if yes writeAll(map.uri, data)  (overwrite)
    Q_INVOKABLE void        pickSaveFolder();                 // opens at the last used folder
    Q_INVOKABLE QString     lastFolder() const;
    Q_INVOKABLE QVariantMap resolveSaveTarget(const QString &folderUri, const QString &fileName,
                                              const QString &currentDocumentUri) const;
    Q_INVOKABLE QString     createFileExact(const QString &folderUri, const QString &mimeType,
                                            const QString &fileName) const;
    Q_INVOKABLE bool        isSameDocument(const QString &uriA, const QString &uriB) const;

    // ---- synchronous helpers working on content:// URIs ----
    Q_INVOKABLE QString     displayName(const QString &uri) const;
    Q_INVOKABLE qint64      size(const QString &uri) const;
    Q_INVOKABLE QString     mimeType(const QString &uri) const;
    Q_INVOKABLE QByteArray  readAll(const QString &uri) const;
    Q_INVOKABLE bool        writeAll(const QString &uri, const QByteArray &data) const;
    Q_INVOKABLE QString     copyToCache(const QString &uri) const;   // returns a real file path
    Q_INVOKABLE int         openFd(const QString &uri, const QString &mode = QStringLiteral("r")) const;

    Q_INVOKABLE QStringList persistedUris() const;
    Q_INVOKABLE void        releasePermission(const QString &uri);

    // folder helpers: each entry = "uri\tname\tmime\tisDir"
    Q_INVOKABLE QStringList listChildren(const QString &treeOrFolderUri) const;
    Q_INVOKABLE QString     createFileInTree(const QString &folderUri, const QString &mime,
                                             const QString &name) const;
    Q_INVOKABLE bool        deleteDocument(const QString &uri) const;

signals:
    void documentsPicked(const QStringList &uris);   // after openDocument()
    void documentCreated(const QString &uri);        // after createDocument()
    void treePicked(const QString &uri);             // after openTree()
    void saveFolderPicked(const QString &folderUri); // after pickSaveFolder()
    void cancelled();                                // user backed out / error

protected:
    void handleActivityResult(int requestCode, int resultCode, const QJniObject &data) override;

private:
    enum Request { ReqOpen = 0x5AF1, ReqCreate = 0x5AF2, ReqTree = 0x5AF3, ReqSaveFolder = 0x5AF4 };
    void startPicker(const QJniObject &intent, int requestCode);
};
