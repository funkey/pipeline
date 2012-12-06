#include "Output.h"
#include "ProcessNode.h"

namespace pipeline {

OutputBase::OutputBase() :
	_processNode(0) {}

void
OutputBase::registerForwardSlot(signals::SlotBase& slot) {

	_forwardSender.registerSlot(slot);
}

void
OutputBase::registerForwardCallback(signals::CallbackBase& callback) {

	_forwardReceiver.registerCallback(callback);
}

signals::Sender&
OutputBase::getForwardSender() {

	return _forwardSender;
}

signals::Receiver&
OutputBase::getForwardReceiver() {

	return _forwardReceiver;
}

void
OutputBase::setProcessNode(ProcessNode* processNode) {

	// don't reassign if this output already belongs to a process node
	if (_processNode == 0)
		_processNode = processNode;
}

boost::shared_ptr<ProcessNode>
OutputBase::getProcessNode() const {

	return _processNode->getSharedPtr();
}

} // namespace pipeline
