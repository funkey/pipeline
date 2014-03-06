#include "Output.h"
#include "ProcessNode.h"

namespace pipeline {

OutputBase::~OutputBase() {

	// destruct all process node callbacks, that have been registered for
	// this output
	for (std::vector<signals::CallbackBase*>::iterator i = _callbacks.begin();
		 i != _callbacks.end(); i++)
		delete *i;
}

void
OutputBase::registerSlot(signals::SlotBase& slot) {

	_sender.registerSlot(slot);
}

void
OutputBase::registerCallback(signals::CallbackBase& callback) {

	_receiver.registerCallback(callback);
}

signals::Sender&
OutputBase::getSender() {

	return _sender;
}

signals::Receiver&
OutputBase::getReceiver() {

	return _receiver;
}

void
OutputBase::addDependency(ProcessNode* processNode) {

	_dependencies.push_back(processNode);
}

std::vector<boost::shared_ptr<ProcessNode> >
OutputBase::getDependencies() const {

	std::vector<boost::shared_ptr<ProcessNode> > dependencies;

	foreach (ProcessNode* processNode, _dependencies)
		dependencies.push_back(processNode->getSelfSharedPointer());

	return dependencies;
}

void
OutputBase::notifyPointerSet() {

	(*_pointerSet)();
}

} // namespace pipeline
