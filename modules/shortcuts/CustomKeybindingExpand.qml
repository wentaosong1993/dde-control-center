import QtQuick 2.0
import QtQml.Models 2.1
import Deepin.Widgets 1.0

Column {
    width: parent.width

    property alias expandId: expand
    property bool isCustom: false

    property var keyBindingIds: {
        var obj = new Object()
        for(var i in keyBindings){
            obj[keyBindings[i][0]] = keyBindings[i][1]
        }
        return obj
    }

    property url iconPath: {
        for(var i in conflictInvalid){
            if(typeof keyBindingIds[conflictInvalid[i]] != "undefined"){
                return "images/error_key.png"
            }
        }
        for(var i in conflictValid){
            if(typeof keyBindingIds[conflictValid[i]] != "undefined"){
                return "images/conflict_key.png"
            }
        }
        return ""
    }

    DBaseExpand {
        id: expand

        expanded: expandItemIndex == myIndex
        
        onExpandedChanged: {
            header.item.active = expanded
        }
        
        //header.sourceComponent: DDownArrowHeader {
            //text: name
            //width: parent.width
            ////icon: iconpath
            //leftMargin: icon ? 20 : 0
            //rightMargin: 15
            
            //Component.onCompleted: {
                //active = expand.expanded
            //}

            //onClicked: {
                //expanditemindex = active ? myindex : -1
            //}
        //}

        header.sourceComponent: DBaseLine {
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    expandItemIndex = active ? myIndex : -1
                }
            }
            leftLoader.sourceComponent: Row {
                anchors.left: parent.left
                anchors.leftMargin: 5
                spacing: 3
                Image {
                    source: iconPath
                    visible: iconPath != ""
                }

                DssH2 {
                    text: name
                }
            }

        }
        
        content.sourceComponent: Component {
            ListView {
                id: lists
                focus: true
                width: parent.width
                height: lists.count * 30
                model: keyBindings.length
                property var keyData: keyBindings
                delegate: ShortcutInput {
                    info: keyBindings[index]
                    warning: {
                        for(var i in conflictValid){
                            if (info[0] == conflictValid[i]){
                                return "conflict"
                            }
                        }
                        for(var i in conflictInvalid){
                            if (info[0] == conflictInvalid[i]){
                                return "error"
                            }
                        }
                        return ""
                    }
                }
            }
        }
    }

    DSeparatorHorizontal {}
}
