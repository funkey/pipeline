#include <util/typename.h>
#include "Logging.h"
#include "Input.h"

namespace pipeline {

InputBase::InputBase() :
	_assignedOutput(0) {}

void
InputBase::registerBackwardSlot(signals::SlotBase& slot) {

	_backwardSender.registerSlot(slot);
}

void
InputBase::registerBackwardCallback(signals::CallbackBase& callback) {

	_backwardReceiver.registerCallback(callback);
}

bool
InputBase::hasAssignedOutput() const {

	return _assignedOutput != 0;
}

OutputBase&
InputBase::getAssignedOutput() const {

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
InputBase::getBackwardSender() {

	return _backwardSender;
}

signals::Receiver&
InputBase::getBackwardReceiver() {

	return _backwardReceiver;
}

} // namespace pipeline
