import QtQuick
import QtQuick.Controls

// Implements the Save / Save As rules on top of SafBridge (context property "saf").
//
//   Save    : document already has a file  -> write silently
//             untitled document            -> behaves like Save As
//   Save As : 1. system folder chooser
//             2. own file-name dialog
//             3. check the name in that folder:
//                  not existing            -> create it, write
//                  same as current doc     -> write silently (like a normal save)
//                  other existing document -> ask "Overwrite?"  (never auto-numbering)
//
// Usage:
//   DocumentSaver {
//       id: saver
//       documentBytes: function() { return editor.text }   // string or QByteArray
//       onSaved: (uri, name) => statusLabel.text = "Saved " + name
//   }
//   Button { text: "Save";    onClicked: saver.save() }
//   Button { text: "Save As"; onClicked: saver.saveAs() }
Item {
    id: root

    // ---- state of the open document ----
    property string currentUri: ""              // content:// URI of the open file, "" = untitled
    property string currentName: "untitled.txt"

    // ---- supplied by the app ----
    property var documentBytes: function() { return "" }

    signal saved(string uri, string name)
    signal failed(string message)

    // internal
    property string _folder: ""
    property string _existingUri: ""
    property string _existingName: ""

    // ------------------------------------------------------------------ API
    function save() {
        if (currentUri === "") {
            saveAs()
        } else if (saf.writeAll(currentUri, documentBytes())) {
            saved(currentUri, currentName)
        } else {
            failed(qsTr("Could not write \"%1\"").arg(currentName))
        }
    }

    function saveAs() {
        saf.pickSaveFolder()                    // answer arrives in onSaveFolderPicked
    }

    // ------------------------------------------------------------ internals
    Connections {
        target: saf
        function onSaveFolderPicked(folderUri) {
            root._folder = folderUri
            nameField.text = root.currentName
            nameDialog.open()
        }
    }

    function _mimeFor(name) {
        const n = name.toLowerCase()
        if (n.endsWith(".txt"))  return "text/plain"
        if (n.endsWith(".json")) return "application/json"
        if (n.endsWith(".csv"))  return "text/csv"
        return "application/octet-stream"
    }

    function _commit(uri, name) {
        if (saf.writeAll(uri, documentBytes())) {
            currentUri = uri
            currentName = name
            saved(uri, name)
        } else {
            failed(qsTr("Could not write \"%1\"").arg(name))
        }
    }

    function _checkAndSave(name) {
        const r = saf.resolveSaveTarget(_folder, name, currentUri)

        if (r.status === "new") {
            const uri = saf.createFileExact(_folder, _mimeFor(name), name)
            if (uri === "") {
                failed(qsTr("Could not create \"%1\"").arg(name))
                return
            }
            _commit(uri, name)

        } else if (r.status === "same") {
            _commit(currentUri, r.existingName)            // silent, exactly like Save

        } else if (r.status === "exists") {
            _existingUri = r.uri
            _existingName = r.existingName
            overwriteDialog.open()                          // another document: must ask

        } else {
            failed(qsTr("Invalid file name"))
            nameDialog.open()
        }
    }

    Dialog {
        id: nameDialog
        title: qsTr("Save as")
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        TextField {
            id: nameField
            width: 280
            selectByMouse: true
        }
        onAccepted: root._checkAndSave(nameField.text.trim())
    }

    Dialog {
        id: overwriteDialog
        title: qsTr("File already exists")
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            width: 280
            wrapMode: Text.Wrap
            text: qsTr("\"%1\" already exists in this folder.\nDo you want to overwrite it?")
                      .arg(root._existingName)
        }
        onAccepted: root._commit(root._existingUri, root._existingName)
        onRejected: {                                       // back to the name field, nothing written
            nameField.text = root._existingName
            nameDialog.open()
        }
    }
}
