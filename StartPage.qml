import QtQuick

Rectangle {
    id: startpage
    height: Math.min(parent.height * 9 / 10, parent.height - 40)
    anchors{
        bottom:parent.bottom
        left:parent.left
        right:parent.right
    }
    color:"#777777"

    Row{
        id:startpage_organizer
        anchors.fill: parent
        anchors.margins: 8
        spacing:8
        SSBox{
            id:load_config_box
            SSBText{
                id:load_config_title
                text:"Open your config ..."
                font.pointSize: (36.0 * (parent.width / (450)))
            }
        }
        Rectangle{
            id:startpage_separator
            color:"white"
            anchors.verticalCenter: parent.verticalCenter
            width:1
            height:parent.height - 16
        }
        SSBox{
            id:preset_config_box
            SSBText{
                id:preset_config_title
                text:"... or start with a preset:"
                font.pointSize: (36 * (parent.width / (450)))
            }
        }
    }

}
