
#ifndef _COM_GeglNode_h_
#define _COM_GeglNode_h_

#include "COM_Node.h"

class GmicNode : public Node {
public:
	GmicNode(bNode *editorNode);
	void convertToOperations(NodeConverter &converter, const CompositorContext &context) const;
};

#endif
