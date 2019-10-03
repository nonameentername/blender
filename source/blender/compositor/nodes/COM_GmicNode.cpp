
#include "COM_GmicNode.h"

#include "COM_GmicOperation.h"
#include "COM_ExecutionSystem.h"

GmicNode::GmicNode(bNode *editorNode) : Node(editorNode)
{
	/* pass */
}

void GmicNode::convertToOperations(NodeConverter &converter, const CompositorContext &context) const
{
	GmicOperation *operation = new GmicOperation();

	const NodeGmic* rna = (NodeGmic*)this->getbNode()->storage;
	operation->setData(rna);
	
	converter.addOperation(operation);

	for (int i = 0; i < (1 + 5); i++) {
		converter.mapInputSocket(this->getInputSocket(i), operation->getInputSocket(i));
	}
	converter.mapOutputSocket(this->getOutputSocket(0), operation->getOutputSocket(0));
}
