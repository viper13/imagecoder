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

    Item {
        anchors.fill: parent
        anchors.margins: 40
        Component {
            id: fileDelegate
            Row {
                height: 20
                spacing: 10
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

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 10
                    color: {
                        switch (status) {
                        case FileStatus.Unknown : return "black"
                        case FileStatus.Unsupported : return "red"
                        case FileStatus.Converted : return "green"
                        case FileStatus.Processing : return "yellow"
                        }
                    }
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
