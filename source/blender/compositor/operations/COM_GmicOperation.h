
#ifndef _COM_GeglOperation_h_
#define _COM_GeglOperation_h_

#include "COM_NodeOperation.h"
#include "COM_SingleThreadedOperation.h"

class GmicOperation : public SingleThreadedOperation {
private:
	SocketReader *m_inputProgram;
	const NodeGmic *m_data;
public:
	GmicOperation();

	void initExecution();
	void deinitExecution();
	bool determineDependingAreaOfInterest(rcti *input, ReadBufferOperation *readOperation, rcti *output);

	void setData(const NodeGmic *data) { this->m_data = data; }

protected:

	MemoryBuffer *createMemoryBuffer(rcti *rect);
};

#endif
