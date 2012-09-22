#include <boost/thread/thread.hpp>

#include "InputSignals.h"
#include "ProcessNode.h"
#include "SimpleProcessNode.h"

logger::LogChannel simpleprocessnodelog("simpleprocessnodelog");

namespace pipeline {

template <typename LockingStrategy>
SimpleProcessNodeImpl<LockingStrategy>::SimpleProcessNodeImpl() :
	_numInputs(0),
	_numMultiInputs(0),
	_numOutputs(0) {}

template <typename LockingStrategy>
SimpleProcessNodeImpl<LockingStrategy>::~SimpleProcessNodeImpl() {

	foreach (signals::SlotsBase* slot, _multiInputUpdates)
		delete slot;
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::registerInput(InputBase& input, std::string name, InputType inputType) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] got a new input " << name << std::endl;

	ProcessNode::registerInput(input, name);

	int numInput = _numInputs;

	_inputDirty.push_back(true);

	_inputUpdate.addSlot();

	// create signal callbacks that store the number of the input with them
	boost::function<void(Modified&)> funOnModified = boost::bind(&SimpleProcessNodeImpl<LockingStrategy>::onInputModified, this, _1, numInput);

	// register the callbacks and setup process node tracking
	input.registerBackwardCallback(funOnModified, this, signals::Transparent);

	if (inputType == Optional) {

		// Optional inputs are non-dirty by default (such that the output will
		// be computed, regardless of their presence).
		_inputDirty[numInput] = false;

		// However, if an optional input is set, it has to be marked dirty,
		// except it was set to a shared pointer -- this is taken care of with
		// the following callbacks.
		boost::function<void(InputSetBase&)> funOnInputSet = boost::bind(&SimpleProcessNodeImpl<LockingStrategy>::onInputSet, this, _1, numInput);
		boost::function<void(InputSetBase&)> funOnInputSetToSharedPointer = boost::bind(&SimpleProcessNodeImpl<LockingStrategy>::onInputSetToSharedPointer, this, _1, numInput);

		input.registerBackwardCallback(funOnInputSet, this, signals::Transparent);
		input.registerBackwardCallback(funOnInputSetToSharedPointer, this, signals::Transparent);
	}

	// register the appropriate update signal for this input
	input.registerBackwardSlot(_inputUpdate[numInput]);

	_numInputs++;

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::registerInputs(MultiInput& input, std::string name) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] got a new multi-input " << name << std::endl;

	ProcessNode::registerInputs(input, name);

	int numMultiInput = _numMultiInputs;

	_multiInputDirty.push_back(std::vector<bool>());
	_multiInputUpdates.push_back(new signals::Slots<Update>());

	// create signal callbacks that store the number of the multi-input with them
	boost::function<void(InputAddedBase&)>         funOnInputAdded = boost::bind(&SimpleProcessNodeImpl<LockingStrategy>::onInputAdded,         this, _1, numMultiInput);
	boost::function<void(Modified&, unsigned int)> funOnModified   = boost::bind(&SimpleProcessNodeImpl<LockingStrategy>::onMultiInputModified, this, _1, _2, numMultiInput);

	// register the callbacks and setup process node tracking
	input.registerBackwardCallback(funOnInputAdded, this, signals::Transparent);
	input.registerBackwardCallbacks(funOnModified, this, signals::Transparent);

	// register the appropriate update signal for this input
	input.registerBackwardSlots(*_multiInputUpdates[numMultiInput]);

	_numMultiInputs++;

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::registerOutput(OutputBase& output, std::string name) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] got a new output " << name << std::endl;

	ProcessNode::registerOutput(output, name);

	int numOutput = _numOutputs;

	_outputDirty.push_back(true);

	_modified.addSlot();

	// create a signal callbacks that stores the number of the output with it
	boost::function<void(Update&)> funOnUpdate = boost::bind(&SimpleProcessNodeImpl<LockingStrategy>::onUpdate, this, _1, numOutput);

	output.registerForwardCallback(funOnUpdate, this, signals::Transparent);

	// register the appropriate update signal for this output
	output.registerForwardSlot(_modified[numOutput]);

	// store the output number in a look-up table
	_outputNums[&output] = numOutput;

	_numOutputs++;
}

/**
 * Explicitly update this process node.
 */
template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::updateInputs() {

	boost::mutex::scoped_lock lock(_updateMutex);

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input update requested by user" << std::endl;

	sendUpdateSignals();

	// TODO (multithreading): block and wait for all the inputs to update
};

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::setDirty(OutputBase& output) {

	// make sure we don't miss this notification by a race condition
	boost::mutex::scoped_lock lock(_updateMutex);

	if (_outputNums.count(&output) == 0)
		LOG_ERROR(simpleprocessnodelog)
				<< "[" << typeName(this) << "] invalid request to set dirty an unknown output" << std::endl;

	unsigned int outputNum = _outputNums[&output];

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] user set dirty output " << outputNum << std::endl;

	_outputDirty[outputNum] = true;

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending modified to output " << outputNum << std::endl;

	_modified[outputNum]();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::onInputModified(const Modified& signal, int numInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input " << numInput << " was modified" << std::endl;

	_inputDirty[numInput] = true;

	setOutputsDirty();

	sendModifiedSignals();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::onInputSet(const InputSetBase& signal, int numInput) {

	_inputDirty[numInput] = true;

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::onInputSetToSharedPointer(const InputSetBase& signal, int numInput) {

	// shared pointer inputs are never dirty
	_inputDirty[numInput] = false;

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::onInputAdded(const InputAddedBase& signal, int numMultiInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] multi-input " << numMultiInput << " got a new input" << std::endl;

	// add a new dirty flag for this multi-input's new input
	_multiInputDirty[numMultiInput].push_back(true);

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::onMultiInputModified(const Modified& signal, int numInput, int numMultiInput) {

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] multi-input " << numMultiInput << " was modified in input " << numInput << std::endl;

	_multiInputDirty[numMultiInput][numInput] = true;

	setOutputsDirty();

	for (int i = 0; i < _numOutputs; i++)
		_modified[i]();
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::onUpdate(const Update& signal, int numOutput) {

	boost::mutex::scoped_lock lock(_updateMutex);

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] input update requested by another process node" << std::endl;

	if (haveDirtyInput()) {

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] I have some dirty inputs -- sending update signals" << std::endl;

		sendUpdateSignals();

	}

	LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] I have no dirty input" << std::endl;

	if (haveDirtyOutput()) {

		// lock inputs, outputs, and update outputs
		lockInputs(0);

		setOutputsDirty(false);

	} else {

		LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] outputs are still up-to-date" << std::endl;
	}
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::sendUpdateSignals() {

	boost::thread_group workers;

	// ask all dirty inputs for updates
	for (int i = 0; i < _numInputs; i++)
		if (_inputDirty[i]) {

			LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending update signal to input " << i << std::endl;

			_inputDirty[i] = false;
			workers.create_thread(boost::ref(_inputUpdate[i]));
		}

	// ask all dirty multi-inputs for updates
	for (int i = 0; i < _numMultiInputs; i++)
		for (int j = 0; j < _multiInputDirty[i].size(); j++)
			if (_multiInputDirty[i][j]) {

				LOG_ALL(simpleprocessnodelog) << "[" << typeName(this) << "] sending update signal to multi-input " << i << ", input " << j << std::endl;

				_multiInputDirty[i][j] = false;
				workers.create_thread(boost::ref((*_multiInputUpdates[i])[j]));
			}

	if (workers.size() > 0) {

		LOG_DEBUG(simpleprocessnodelog) << "[" << typeName(this) << "] waiting for all workers to finish..." << std::endl;
		workers.join_all();
		LOG_DEBUG(simpleprocessnodelog) << "[" << typeName(this) << "] workers finished" << std::endl;
	}
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::sendModifiedSignals() {

	// send modified to all outputs
	// TODO: let the user decide, which outputs do get dirty on which input
	// change
	for (int i = 0; i < _numOutputs; i++)
		_modified[i]();
}

template <typename LockingStrategy>
bool
SimpleProcessNodeImpl<LockingStrategy>::haveDirtyInput() {

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

template <typename LockingStrategy>
bool
SimpleProcessNodeImpl<LockingStrategy>::haveDirtyOutput() {

	for (int i = 0; i < _numOutputs; i++)
		if (_outputDirty[i])
			return true;

	return false;
}

template <typename LockingStrategy>
void
SimpleProcessNodeImpl<LockingStrategy>::setOutputsDirty(bool dirty) {

	for (int i = 0; i < _outputDirty.size(); i++)
		_outputDirty[i] = dirty;
}

template class SimpleProcessNodeImpl<FullLockingStrategy>;

}
