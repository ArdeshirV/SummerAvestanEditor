// Receives ".ave" files on every platform from one place. Header-only, no moc needed.
//   Android / macOS : the OS sends a QFileOpenEvent (Android gives a content:// URI)
//   Windows / Linux : the file path arrives as a command-line argument
//
// In main(), after creating your window:
//   OpenFileSupport open;
//   open.onFile = [&](const QString& p) { window.openDocument(p); };   // QFile can read content:// URIs
//   app.installEventFilter(&open);
//   open.handleArguments(app.arguments());
#pragma once
#include <QEvent>
#include <QFileOpenEvent>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <functional>

class OpenFileSupport : public QObject {
public:
    std::function<void(const QString&)> onFile;

    void handleArguments(const QStringList& args) {
        for (int i = 1; i < args.size(); ++i)
            if (!args[i].startsWith(QLatin1Char('-')) && onFile) onFile(args[i]);
    }

protected:
    bool eventFilter(QObject*, QEvent* e) override {
        if (e->type() != QEvent::FileOpen) return false;
        const QUrl url = static_cast<QFileOpenEvent*>(e)->url();
        if (onFile) onFile(url.isLocalFile() ? url.toLocalFile() : url.toString());
        return true;
    }
};
