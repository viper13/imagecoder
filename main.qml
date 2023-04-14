import QtQuick 2.9
import QtQuick.Window 2.2

import Application 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Image compressor/decompressor")
    FilesModel {
        id: filesModel
        path: "d:\\images\\"
    }
    ListModel {
        id: testModel
        ListElement {
            name: "first"
        }
        ListElement {
            name: "second"
        }
        ListElement {
            name: "third"
        }
        ListElement {
            name: "forth"
        }
    }
    Item {
        anchors.fill: parent
        anchors.margins: 40
        Component {
            id: fileDelegate
            Row {
                height: 20
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    text: index
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 100
                    text: name
                    wrapMode: Text.WrapAnywhere
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 50
                    text: size
                }
            }
        }

        ListView {
            id: filesList
            width: parent.width
            model: filesModel
            delegate: fileDelegate
            anchors.fill: parent
        }
    }
}
