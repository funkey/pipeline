#include <util/typename.h>
#include "Logging.h"
#include "Input.h"

namespace pipeline {

InputBase::InputBase() :
	_assignedOutput(0) {}

InputBase::InputBase(std::string name) :
	_name(name),
	_assignedOutput(0) {}

void
InputBase::setName(std::string name) {

	_name = name;
}

const std::string&
InputBase::getName() {

	return _name;
}

bool
InputBase::accept(OutputBase& output) {

	LOG_ALL(pipelinelog) << "[" << typeName(this) << "] trying to accept output " << typeName(output) << std::endl;

	return tryToAccept(output);
}

void
InputBase::registerBackwardSlot(signals::SlotBase& slot) {

	_backwardSender.registerSlot(slot);
}

void
InputBase::registerBackwardCallback(signals::CallbackBase& callback) {

	_backwardReceiver.registerCallback(callback);
}

bool
InputBase::hasAssignedOutput() {

	return _assignedOutput != 0;
}

OutputBase&
InputBase::getAssignedOutput() {

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
