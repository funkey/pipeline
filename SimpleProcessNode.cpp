#include "InputSignals.h"
#include "ProcessNode.h"
#include "SimpleProcessNode.h"

logger::LogChannel simpleprocessnodelog("simpleprocessnodelog");

namespace pipeline {

SimpleProcessNode::SimpleProcessNode() :
	_numInputs(0),
	_numMultiInputs(0),
	_outputsDirty(true) {}

SimpleProcessNode::~SimpleProcessNode() {

	foreach (signals::SlotsBase* slot, _multiInputUpdates)
		delete slot;
}

void
SimpleProcessNode::registerInput(InputBase& input, std::string name, InputType inputType) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] got a new input " << name << std::endl;

	ProcessNode::registerInput(input, name);

	int numInput = _numInputs;

	_inputDirty.push_back(true);

	_inputUpdate.addSlot();

	// create signal callbacks that store the number of the input with them
	boost::function<void(Modified&)> funOnModified = boost::bind(&SimpleProcessNode::onInputModified, this, _1, numInput);
	boost::function<void(Updated&)>  funOnUpdated  = boost::bind(&SimpleProcessNode::onInputUpdated,  this, _1, numInput);

	// register the callbacks and setup process node tracking
	input.registerBackwardCallback(funOnModified, this);
	input.registerBackwardCallback(funOnUpdated,  this);

	if (inputType == Optional) {

		// Optional inputs are non-dirty by default (such that the output will
		// be computed, regardless of their presence).
		_inputDirty[numInput] = false;

		// However, if an optional input is set, it has to be marked dirty --
		// this is taken care of with the following callback.
		boost::function<void(InputSetBase&)> funOnInputSet = boost::bind(&SimpleProcessNode::onInputSet, this, _1, numInput);

		input.registerBackwardCallback(funOnInputSet, this);
	}

	// register the appropriate update signal for this input
	input.registerBackwardSlot(_inputUpdate[numInput]);

	_numInputs++;

	_outputsDirty = true;
}

void
SimpleProcessNode::registerInputs(MultiInput& input, std::string name) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] got a new multi-input " << name << std::endl;

	ProcessNode::registerInputs(input, name);

	int numMultiInput = _numMultiInputs;

	_multiInputDirty.push_back(std::vector<bool>());
	_multiInputUpdates.push_back(new signals::Slots<Update>());

	// create signal callbacks that store the number of the multi-input with them
	boost::function<void(InputAddedBase&)>         funOnInputAdded = boost::bind(&SimpleProcessNode::onInputAdded,         this, _1, numMultiInput);
	boost::function<void(Modified&, unsigned int)> funOnModified   = boost::bind(&SimpleProcessNode::onMultiInputModified, this, _1, _2, numMultiInput);
	boost::function<void(Updated&,  unsigned int)> funOnUpdated    = boost::bind(&SimpleProcessNode::onMultiInputUpdated,  this, _1, _2, numMultiInput);

	// register the callbacks and setup process node tracking
	input.registerBackwardCallback(funOnInputAdded, this);
	input.registerBackwardCallbacks(funOnModified, this);
	input.registerBackwardCallbacks(funOnUpdated,  this);

	// register the appropriate update signal for this input
	input.registerBackwardSlots(*_multiInputUpdates[numMultiInput]);

	_numMultiInputs++;

	_outputsDirty = true;
}

void
SimpleProcessNode::registerOutput(OutputBase& output, std::string name) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] got a new output " << name << std::endl;

	ProcessNode::registerOutput(output, name);

	output.registerForwardCallback(&SimpleProcessNode::onUpdate, this);
	output.registerForwardSlot(_modified);
	output.registerForwardSlot(_updated);

	_outputsDirty = true;
}

/**
 * Explicitly update this process node.
 */
void
SimpleProcessNode::updateInputs() {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input update requested by user" << std::endl;

	sendUpdateSignals();

	// TODO (multithreading): block and wait for all the inputs to update
};

void
SimpleProcessNode::setDirty(OutputBase& output) {

	_outputsDirty = true;

	_modified();

	// TODO: send modified only for the outputs that actually changed
}

void
SimpleProcessNode::onInputModified(const Modified& signal, int numInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input " << numInput << " was modified" << std::endl;

	_inputDirty[numInput] = true;

	_outputsDirty = true;

	_modified();
}

void
SimpleProcessNode::onInputUpdated(const Updated& signal, int numInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input " << numInput << " was updated" << std::endl;

	// this input is up-to-date now
	_inputDirty[numInput] = false;

	// don't do anything if not all inputs have been updated
	if (haveDirtyInput()) {

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] some inputs are still dirty" << std::endl;
		return;
	}

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] all inputs are up-to-date, updating outputs" << std::endl;

	// all inputs are up-to-date -- call user's update function if needed
	if (_outputsDirty) {

		updateOutputs();
		_outputsDirty = false;

	} else
		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] outputs are still up-to-date" << std::endl;

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending updated signal" << std::endl;

	// send Updated signal
	_updated();
}

void
SimpleProcessNode::onInputSet(const InputSetBase& signal, int numInput) {

	_inputDirty[numInput] = true;

	_outputsDirty = true;
}

void
SimpleProcessNode::onInputAdded(const InputAddedBase& signal, int numMultiInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] multi-input " << numMultiInput << " got a new input" << std::endl;

	// add a new dirty flag for this multi-input's new input
	_multiInputDirty[numMultiInput].push_back(true);

	_outputsDirty = true;
}

void
SimpleProcessNode::onMultiInputModified(const Modified& signal, int numInput, int numMultiInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] multi-input " << numMultiInput << " was modified in input " << numInput << std::endl;

	_multiInputDirty[numMultiInput][numInput] = true;

	_outputsDirty = true;

	_modified();
}

void
SimpleProcessNode::onMultiInputUpdated(const Updated& signal, int numInput, int numMultiInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] multi-input " << numMultiInput << " was updated in input " << numInput << std::endl;

	// this input is up-to-date now
	_multiInputDirty[numMultiInput][numInput] = false;

	// don't do anything if not all inputs have been updated
	if (haveDirtyInput()) {

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] some inputs are still dirty" << std::endl;
		return;
	}

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] all inputs are up-to-date, updating outputs" << std::endl;

	// all inputs are up-to-date -- call user's update function
	if (_outputsDirty) {

		updateOutputs();
		_outputsDirty = false;

	} else
		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] outputs are still up-to-date" << std::endl;

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending updated signal" << std::endl;

	// send Updated signal
	_updated();
}

void
SimpleProcessNode::onUpdate(const Update& signal) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input update requested by another process node" << std::endl;

	if (haveDirtyInput()) {

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] I have some dirty inputs -- sending update signals" << std::endl;

		sendUpdateSignals();

	} else {

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] I have no dirty input" << std::endl;

		if (_outputsDirty) {

			updateOutputs();
			_outputsDirty = false;

		} else
			LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] outputs are still up-to-date" << std::endl;

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending updated signal" << std::endl;

		// send Updated signal
		_updated();
	}
}

void
SimpleProcessNode::sendUpdateSignals() {

	// ask all dirty inputs for updates
	for (int i = 0; i < _numInputs; i++)
		if (_inputDirty[i]) {

			LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending update signal to input " << i << std::endl;
			_inputUpdate[i]();
		}

	// ask all dirty multi-inputs for updated
	for (int i = 0; i < _numMultiInputs; i++)
		for (int j = 0; j < _multiInputDirty[i].size(); j++)
			if (_multiInputDirty[i][j]) {

				LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending update signal to multi-input " << i << ", input " << j << std::endl;
				(*_multiInputUpdates[i])[j]();
			}
}

bool
SimpleProcessNode::haveDirtyInput() {

	// check inputs
	for (int i = 0; i < _numInputs; i++)
		if (_inputDirty[i])
			return true;

	// check multi-inputs
	for (int i = 0; i < _numMultiInputs; i++)
		for (int j = 0; j < _multiInputDirty[i].size(); j++)
			if (_multiInputDirty[i][j])
				return true;

	return false;
}

}
