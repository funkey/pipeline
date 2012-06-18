#include "Output.h"
#include "ProcessNode.h"

namespace pipeline {

OutputBase::OutputBase() {}

void
OutputBase::setName(std::string name) {

	_name = name;
}

std::string
OutputBase::getName() const {

	return _name;
}

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

	_processNode = processNode;
}

boost::shared_ptr<ProcessNode>
OutputBase::getProcessNode() const {

	return _processNode->getSharedPtr();
}

} // namespace pipeline
