#include <util/typename.h>
#include "Logging.h"
#include "Input.h"

namespace pipeline {

InputBase::InputBase() :
	_assignedOutput(0) {}

void
InputBase::registerSlot(signals::SlotBase& slot) {

	_sender.registerSlot(slot);
}

void
InputBase::registerCallback(signals::CallbackBase& callback) {

	_receiver.registerCallback(callback);
}

bool
InputBase::hasAssignedOutput() const {

	return _assignedOutput != 0;
}

OutputBase&
InputBase::getAssignedOutput() const {

#ifndef NDEBUG
	if (_assignedOutput == 0)
		UTIL_THROW_EXCEPTION(NullPointer, "This input does not have an assigned output");
#endif

	return *_assignedOutput;
}

void
InputBase::setAssignedOutput(OutputBase& output) {

	_assignedOutput = &output;
}

void
InputBase::unsetAssignedOutput() {

	_assignedOutput = 0;
}

signals::Sender&
InputBase::getSender() {

	return _sender;
}

signals::Receiver&
InputBase::getReceiver() {

	return _receiver;
}

} // namespace pipeline
