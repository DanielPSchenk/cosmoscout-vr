////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace operation (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

// The TimeNode is pretty simple as it only has a single output socket. The component serves as
// a kind of factory. Whenever a new node is created, the builder() method is called.
class SentinelComponent extends Rete.Component {

  constructor() {
    // This name must match the WCSSourceNode::sName defined in WCSSource.cpp.
    super("Sentinel");

    // This specifies the submenu from which this node can be created in the node editor.
    this.category = "Data Extraction";
  }

  // Called whenever a new node of this type needs to be constructed.
  builder(node) {

    // This node has a single output. The first parameter is the name of this output and must be
    // unique amongst all sockets. It is also used in the WCSImageLoader::process() to write the
    // output of this node. The second parameter is shown as name on the node. The last
    // parameter references a socket type which has been registered with the node factory
    // before. It is required that the class is called <NAME>Component.

    let coverageInput =
        new Rete.Input('coverageIn', "Coverage", CosmoScout.socketTypes['Coverage']);
    node.addInput(coverageInput);

    let boundsIn = new Rete.Input('boundsIn', "Long/Lat Bounds", CosmoScout.socketTypes['RVec4']);
    node.addInput(boundsIn);

    let timeInput = new Rete.Input('wcsTimeIn', "Time", CosmoScout.socketTypes['WCSTime']);
    node.addInput(timeInput);

    let resolutionInput =
        new Rete.Input('resolutionIn', "Maximum Resolution", CosmoScout.socketTypes['Int']);
    node.addInput(resolutionInput);

    let imageOutput = new Rete.Output('imageOut', 'Image 2D', CosmoScout.socketTypes['Image2D']);
    node.addOutput(imageOutput);

    const dropDownCallback = (selection) => CosmoScout.sendMessageToCPP(selection, node.id);

    let operationControl =
        new DropDownControl('operation', dropDownCallback, "Operation", [{value: 0, text: 'None'}]);
    node.addControl(operationControl);

    node.onMessageFromCPP = (message) => operationControl.setOptions(
        message.map((operationName, index) => ({value: index, text: operationName})));

    node.onInit =
        (nodeDiv) => {
          operationControl.init(nodeDiv, {
            options:
                node.data.operations?.map((operation, index) => ({value: index, text: operation})),
            selectedValue: node.data.selectedOperation
          });
        }

    return node;
  }
}