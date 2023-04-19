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
        path: (Qt.application.arguments.length > 1) ? Qt.application.arguments[1] : Qt.application.arguments[0]
        onErrorHappens: {
            statusRow.errorInfo = message
            clearStatusTimer.start()
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 40
        Component {
            id: fileDelegate
            MouseArea {
                width: childrenRect.width
                height: childrenRect.height
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function (mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        filesModel.compressFile(index)
                    } else if (mouse.button === Qt.RightButton) {
                        filesModel.decompressFile(index)
                    }
                }
                Rectangle {
                    width: childrenRect.width
                    height: childrenRect.height
                    color: "#e3bbb8"
                    radius: 5
                    Row {
                        height: 40
                        spacing: 10
                        Item {
                            //spacer
                            width: 10
                            height: 10
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20
                            text: index
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 300
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
                                case FileStatus.Compressed : return "green"
                                case FileStatus.NotCompressed : return "blue"
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
            }
        }
        ListView {
            id: filesList
            width: parent.width
            model: filesModel
            delegate: fileDelegate
            anchors.fill: parent
            spacing: 10
        }
    }

    Timer {
        id: clearStatusTimer
        interval: 2000;
        onTriggered: statusRow.errorInfo = ""
    }
    Row {
        width: parent.width
        Text {
            id: statusRow
            property string errorInfo
            width: parent.width - refreshBtn.width - 20
            font.pixelSize: 20
            text: qsTr("Last error: ") + errorInfo
        }

        MouseArea {
            id: refreshBtn
            anchors.margins: 10
            width: childrenRect.width
            height: childrenRect.height
            acceptedButtons: Qt.LeftButton
            onClicked: function (mouse) {
                filesModel.update();
            }
            Rectangle {
                width: 75
                height: 26
                color: "#e3bbb8"
                radius: 5
                Text {
                    anchors.margins: 10
                    anchors.centerIn: parent
                    width: 60
                    text: qsTr("Refresh")
                    font.pixelSize: 16
                }
            }
        }
    }


}
