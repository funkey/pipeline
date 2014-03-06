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
