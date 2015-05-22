#include <util/ProgramOptions.h>
#include "InputSignals.h"
#include "ProcessNode.h"
#include "SimpleProcessNode.h"

logger::LogChannel simpleprocessnodelog("simpleprocessnodelog");

namespace pipeline {

util::ProgramOption optionNumThreads(
		util::_module           = "pipeline",
		util::_long_name        = "numThreads",
		util::_description_text = "Set the number of additional threads to parallelize independent processes.",
		util::_default_value    = 0);

template <typename LockingStrategy>
int SimpleProcessNode<LockingStrategy>::_numThreads = 0;

template <typename LockingStrategy>
boost::mutex SimpleProcessNode<LockingStrategy>::_threadCountMutex;

template <typename LockingStrategy>
SimpleProcessNode<LockingStrategy>::SimpleProcessNode(std::string name) :
	_numInputs(0),
	_numMultiInputs(0),
	_numOutputs(0),
	_name(name) {

	_numThreads = optionNumThreads;
}

template <typename LockingStrategy>
SimpleProcessNode<LockingStrategy>::~SimpleProcessNode() {

	foreach (signals::SlotsBase* slot, _multiInputUpdates)
		delete slot;
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::registerInput(InputBase& input, std::string name, InputType inputType) {

	boost::mutex::scoped_lock inputLock(_inputMutex);

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " got a new input " << name << std::endl;

	ProcessNode::registerInput(input, name);

	int numInput = _numInputs;

	_inputDirty.push_back(true);
	_inputDirtys.push_back(std::vector<int>());

	_inputUpdate.addSlot();

	// create signal callbacks that store the number of the input with them
	boost::function<void(Modified&)> funOnModified = boost::bind(&SimpleProcessNode<LockingStrategy>::onInputModified, this, _1, numInput);

	// register the callbacks and setup process node tracking
	input.registerCallback(funOnModified, this, signals::Transparent);

	if (inputType == Optional) {

		// Optional inputs are non-dirty by default (such that the output will
		// be computed, regardless of their presence).
		_inputDirty[numInput] = false;

		// optional inputs need not be present to update the output
		_inputRequired.push_back(false);

		// However, if an optional input is set, it has to be marked dirty,
		boost::function<void(InputSetBase&)> funOnInputSet = boost::bind(&SimpleProcessNode<LockingStrategy>::onInputSet, this, _1, numInput);
		input.registerCallback(funOnInputSet, this, signals::Transparent);

	} else {

		// non-optional inputs have to be present before we can update the 
		// output
		_inputRequired.push_back(true);
	}

	// Regardless of the type of input -- if it was set to a shared pointer it 
	// has to be set dirty and Modified has to be sent.
	boost::function<void(InputSetToSharedPointerBase&)> funOnInputSetToSharedPointer = boost::bind(&SimpleProcessNode<LockingStrategy>::onInputSetToSharedPointer, this, _1, numInput);
	input.registerCallback(funOnInputSetToSharedPointer, this, signals::Transparent);

	// register the appropriate update signal for this input
	input.registerSlot(_inputUpdate[numInput]);

	_inputNums[&input] = numInput;

	_numInputs++;

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::registerInputs(MultiInput& input, std::string name) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " got a new multi-input " << name << std::endl;

	ProcessNode::registerInputs(input, name);

	int numMultiInput = _numMultiInputs;

	_multiInputDirty.push_back(std::vector<bool>());
	_multiInputDirtys.push_back(std::vector<int>());
	_multiInputUpdates.push_back(new signals::Slots<Update>());

	// create signal callbacks that store the number of the multi-input with them
	boost::function<void(InputAddedBase&)>         funOnInputAdded    = boost::bind(&SimpleProcessNode<LockingStrategy>::onInputAdded,         this, _1, numMultiInput);
	boost::function<void(InputsCleared&)>          funOnInputsCleared = boost::bind(&SimpleProcessNode<LockingStrategy>::onInputsCleared,      this, _1, numMultiInput);
	boost::function<void(Modified&, unsigned int)> funOnModified      = boost::bind(&SimpleProcessNode<LockingStrategy>::onMultiInputModified, this, _1, _2, numMultiInput);

	// register the callbacks and setup process node tracking
	input.registerCallback(funOnInputAdded, this, signals::Transparent);
	input.registerCallback(funOnInputsCleared, this, signals::Transparent);
	input.registerCallbacks(funOnModified, this, signals::Transparent);

	// register the appropriate update signal for this input
	input.registerSlots(*_multiInputUpdates[numMultiInput]);

	_multiInputNums[&input] = numMultiInput;

	_numMultiInputs++;

	setOutputsDirty();
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::registerOutput(OutputBase& output, std::string name) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " got a new output " << name << std::endl;

	ProcessNode::registerOutput(output, name);

	int numOutput = _numOutputs;

	_outputDirty.push_back(true);

	_modified.addSlot();

	// create a signal callbacks that stores the number of the output with it
	boost::function<void(Update&)> funOnUpdate = boost::bind(&SimpleProcessNode<LockingStrategy>::onUpdate, this, _1, numOutput);

	output.registerCallback(funOnUpdate, this, signals::Transparent);

	// register the appropriate update signal for this output
	output.registerSlot(_modified[numOutput]);

	// store the output number in a look-up table
	_outputNums[&output] = numOutput;

	_numOutputs++;
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::setDependency(InputBase& input, OutputBase& output) {

	int inputNum  = _inputNums[&input];
	int outputNum = _outputNums[&output];

	_inputDirtys[inputNum].push_back(outputNum);
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::setDependency(MultiInput& input, OutputBase& output) {

	int multiInputNum = _multiInputNums[&input];
	int outputNum     = _outputNums[&output];

	_multiInputDirtys[multiInputNum].push_back(outputNum);
}

/**
 * Explicitly update this process node.
 */
template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::updateInputs() {

	boost::mutex::scoped_lock lock(_updateMutex);

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " input update requested by user" << std::endl;

	sendUpdateSignals();
};

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::setDirty(OutputBase& output) {

	/* Now, here we can have a race condition: While updating our outputs, right
	 * before setting _outputDirty to false for every output, some other thread
	 * might call setDirty(). In this case, this call will have no effect.
	 *
	 * What if we set _outputDirty to false *before* we start updating the
	 * outputs? In the worst case, we don't see the effect of setDirty(), which
	 * doesn't matter, since we update the outputs anyway. Without a race
	 * condition, the output will be set dirty during the update and stay dirty
	 * after it. Since simultaneously we also send a Modified signal this is
	 * equivalent to a setDirty() call after the update. Handling is delegated
	 * to the next process node.
	 */

	if (_outputNums.count(&output) == 0)
		LOG_ERROR(simpleprocessnodelog)
				<< getLogPrefix() << " invalid request to set dirty an unknown output" << std::endl;

	unsigned int outputNum = _outputNums[&output];

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " user set dirty output " << outputNum << std::endl;

	_outputDirty[outputNum] = true;

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " sending modified to output " << outputNum << std::endl;

	_modified[outputNum]();
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onInputModified(const Modified& /*signal*/, int numInput) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " input " << numInput << " was modified" << std::endl;

	boost::mutex::scoped_lock lock(_inputDirtyMutex);

	_inputDirty[numInput] = true;

	sendModifiedSignals(numInput);
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onInputSet(const InputSetBase& /*signal*/, int numInput) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " input " << numInput << " got a new input" << std::endl;

	boost::mutex::scoped_lock lock(_inputDirtyMutex);

	_inputDirty[numInput] = true;

	// since InputSet* signals are modified signals, we have to treat them as 
	// such as well and propagate the Modified signal
	sendModifiedSignals(numInput);
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onInputSetToSharedPointer(const InputSetToSharedPointerBase& /*signal*/, int numInput) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " input " << numInput << " got a new input (shared pointer)" << std::endl;

	boost::mutex::scoped_lock lock(_inputDirtyMutex);

	// shared pointer inputs are never dirty
	_inputDirty[numInput] = false;

	// therefore, we have to set the outputs dirty explicitly
	setOutputsDirty();

	// shared pointers can't talk, so send the modified signal ourselves
	sendModifiedSignals(numInput);
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onInputAdded(const InputAddedBase& /*signal*/, int numMultiInput) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " multi-input " << numMultiInput << " got a new input" << std::endl;

	boost::mutex::scoped_lock lock(_inputDirtyMutex);

	// add a new dirty flag for this multi-input's new input
	_multiInputDirty[numMultiInput].push_back(true);
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onInputsCleared(const InputsCleared& /*signal*/, int numMultiInput) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " multi-input " << numMultiInput << " was cleared" << std::endl;

	boost::mutex::scoped_lock lock(_inputDirtyMutex);

	// clear all flags for this multi-input
	_multiInputDirty[numMultiInput].clear();
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onMultiInputModified(const Modified& /*signal*/, int numInput, int numMultiInput) {

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " multi-input " << numMultiInput << " was modified in input " << numInput << std::endl;

	boost::mutex::scoped_lock lock(_inputDirtyMutex);

	_multiInputDirty[numMultiInput][numInput] = true;

	sendModifiedSignals(numInput, numMultiInput);
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::onUpdate(const Update& /*signal*/, int numOutput) {

	boost::mutex::scoped_lock lock(_updateMutex);

	PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " input update requested by another process node via output " << numOutput << std::endl;

	{
		boost::mutex::scoped_lock inputDirtyLock(_inputDirtyMutex);

		if (haveDirtyInput()) {

			inputDirtyLock.unlock();

			// our inputs changed -- need to recompute the output
			setOutputsDirty();

			PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " I have some dirty inputs -- sending update signals" << std::endl;

			sendUpdateSignals(numOutput);
		}
	}

	/* Here a race condition can occur: While we are sending the update signals
	 * to the inputs, a Modified signal might have been sent by another thread,
	 * resulting in a dirty input right after the update. This is okay, since we
	 * also send Modified to the next node. But with the following code, we set
	 * _outputDirty to false and thus overwrote the setting of the Modified
	 * signal we received earlier. The result is that we don't update our
	 * output, since we don't know that it is dirty.
	 *
	 * One solution would be to set _outputDirty to true whenever we enter this
	 * function and haveDirtyInput() is true. In this case, do we need
	 * _outputDirty at all? Yes, the user can set the output dirty even if the
	 * inputs didn't change.
	 */

	if (haveDirtyOutput() && requiredInputsPresent()) {

		/* Here, the setDirty() race condition can occur. However, it won't hurt
		 * since we are about to update the outputs anyway.
		 */

		setOutputsDirty(false);

		// lock inputs, outputs, and update outputs
		lockInputs(0);

	} else {

		if (!requiredInputsPresent()) {

			LOG_ERROR(simpleprocessnodelog) << getLogPrefix() << " asking for update, but not all required inputs are present!" << std::endl;
		}

		PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " outputs are still up-to-date" << std::endl;
	}
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::sendUpdateSignals(int numOutput) {

	boost::mutex::scoped_lock inputLock(_inputMutex);

	boost::thread_group workers;

	// TODO: this number can be subject to race conditions
	unsigned int numDirties = numDirtyInputs();

	// ask all dirty inputs for updates
	for (int i = 0; i < _numInputs; i++) {

		if (!inputOutputDepends(i, numOutput))
			continue;

		// lock access to _inputDirty to avoid race conditions
		boost::mutex::scoped_lock inputDirtyLock(_inputDirtyMutex);

		if (_inputDirty[i]) {

			PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " sending update signal to input " << i << std::endl;

			_inputDirty[i] = false;

			// release lock on _inputDirty, since the subsequent calls can cause 
			// Modified signals to be sent to us, which will try to aquire the 
			// lock as well
			inputDirtyLock.unlock();

			bool doItYourself = false;

			if (numDirties <= 1) {

				doItYourself = true;

			} else {

				boost::mutex::scoped_lock lock(_threadCountMutex);

				if (_numThreads == 0) {

					PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " no more free threads available, will do it myself" << std::endl;
					doItYourself = true;

				} else {

					_numThreads--;

					PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " launching worker thread" << std::endl;
					workers.create_thread(boost::ref(_inputUpdate[i]));
				}
			}

			numDirties--;

			if (doItYourself) {

				PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " asking for update myself" << std::endl;
				_inputUpdate[i]();
				PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " input " << i << " updated" << std::endl;
			}
		}
	}

	// ask all dirty multi-inputs for updates
	for (int i = 0; i < _numMultiInputs; i++) {

		if (!multiInputOutputDepends(i, numOutput))
			continue;

		for (unsigned int j = 0; j < _multiInputDirty[i].size(); j++) {

			// lock access to _multiInputDirty to avoid race conditions
			boost::mutex::scoped_lock inputDirtyLock(_inputDirtyMutex);

			if (_multiInputDirty[i][j]) {

				PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " sending update signal to multi-input " << i << ", input " << j << std::endl;

				_multiInputDirty[i][j] = false;

				// release lock on _inputDirty, since the subsequent calls can 
				// cause Modified signals to be sent to us, which will try to 
				// aquire the lock as well
				inputDirtyLock.unlock();

				bool doItYourself = false;

				if (numDirties <= 1) {

					doItYourself = true;

				} else {

					boost::mutex::scoped_lock lock(_threadCountMutex);

					if (_numThreads == 0) {

						PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " no more free threads available, will do it myself" << std::endl;
						doItYourself = true;

					} else {

						_numThreads--;

						PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " launching worker thread" << std::endl;
						workers.create_thread(boost::ref((*_multiInputUpdates[i])[j]));
					}
				}

				numDirties--;

				if (doItYourself) {

					PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " asking for update myself" << std::endl;
					(*_multiInputUpdates[i])[j]();
					PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " multi-input " << i << ", input " << j << " updated" << std::endl;
				}
			}
		}
	}

	if (workers.size() > 0) {

		PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " waiting for all workers to finish..." << std::endl;
		workers.join_all();
		PIPELINE_LOG_ALL(simpleprocessnodelog) << getLogPrefix() << " workers finished" << std::endl;

		boost::mutex::scoped_lock lock(_threadCountMutex);

		_numThreads += workers.size();
	}
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::sendModifiedSignals(int numInput, int numMultiInput) {

	// first, check if the user has set an input-output dirty mapping and use 
	// it, if present

	if (numMultiInput == -1) {

		if (_inputDirtys[numInput].size() > 0) {

			foreach (int i, _inputDirtys[numInput])
				_modified[i]();

			return;
		}

	} else {

		if (_multiInputDirtys[numMultiInput].size() > 0) {

			foreach (int i, _multiInputDirtys[numMultiInput])
				_modified[i]();

			return;
		}
	}

	// otherwise, send modified to all outputs
	for (int i = 0; i < _numOutputs; i++)
		_modified[i]();
}

template <typename LockingStrategy>
bool
SimpleProcessNode<LockingStrategy>::haveDirtyInput() {

	// check inputs
	for (int i = 0; i < _numInputs; i++)
		if (_inputDirty[i])
			return true;

	// check multi-inputs
	for (int i = 0; i < _numMultiInputs; i++)
		for (unsigned int j = 0; j < _multiInputDirty[i].size(); j++)
			if (_multiInputDirty[i][j])
				return true;

	return false;
}

template <typename LockingStrategy>
unsigned int
SimpleProcessNode<LockingStrategy>::numDirtyInputs() {

	unsigned int numDirties = 0;

	// check inputs
	for (int i = 0; i < _numInputs; i++)
		if (_inputDirty[i])
			numDirties++;

	// check multi-inputs
	for (int i = 0; i < _numMultiInputs; i++)
		for (unsigned int j = 0; j < _multiInputDirty[i].size(); j++)
			if (_multiInputDirty[i][j])
				numDirties++;

	return numDirties;
}

template <typename LockingStrategy>
bool
SimpleProcessNode<LockingStrategy>::haveDirtyOutput() {

	for (int i = 0; i < _numOutputs; i++)
		if (_outputDirty[i])
			return true;

	return false;
}

template <typename LockingStrategy>
void
SimpleProcessNode<LockingStrategy>::setOutputsDirty(bool dirty) {

	for (unsigned int i = 0; i < _outputDirty.size(); i++)
		_outputDirty[i] = dirty;
}

template <typename LockingStrategy>
bool
SimpleProcessNode<LockingStrategy>::requiredInputsPresent() {

	// check inputs
	for (int i = 0; i < _numInputs; i++)
		if (!(getInput(i).isSet() || getInput(i).hasAssignedOutput()) && _inputRequired[i]) {

			PIPELINE_LOG_ALL(simpleprocessnodelog)
					<< getLogPrefix() << " required input " << i << ": isSet() == "
					<< getInput(i).isSet() << ", hasAssignedOutput() == "
					<< getInput(i).hasAssignedOutput() << std::endl;
			return false;
		}

	return true;
}

template <typename LockingStrategy>
bool
SimpleProcessNode<LockingStrategy>::inputOutputDepends(int numInput, int numOutput) {

	if (_inputDirtys[numInput].size() == 0 || numOutput == -1) {

		LOG_ALL(simpleprocessnodelog) << getLogPrefix() << "output " << numOutput << " does implicitly depend on " << numInput << std::endl;
		return true;
	}

	for (int i = 0; i < _inputDirtys[numInput].size(); i++)
		if (_inputDirtys[numInput][i] == numOutput) {

			LOG_ALL(simpleprocessnodelog) << getLogPrefix() << "output " << numOutput << " does explicitly depend on " << numInput << std::endl;
			return true;
		}

	LOG_ALL(simpleprocessnodelog) << getLogPrefix() << "output " << numOutput << " does not depend on " << numInput << std::endl;
	return false;
}

template <typename LockingStrategy>
bool
SimpleProcessNode<LockingStrategy>::multiInputOutputDepends(int numMultiInput, int numOutput) {

	if (_multiInputDirtys[numMultiInput].size() == 0 || numOutput == -1) {

		LOG_ALL(simpleprocessnodelog) << getLogPrefix() << "output " << numOutput << " does implicitly depend on multi-input " << numMultiInput << std::endl;
		return true;
	}

	for (int i = 0; i < _multiInputDirtys[numMultiInput].size(); i++)
		if (_multiInputDirtys[numMultiInput][i] == numOutput) {

			LOG_ALL(simpleprocessnodelog) << getLogPrefix() << "output " << numOutput << " does explicitly depend on multi-input " << numMultiInput << std::endl;
			return true;
		}

	LOG_ALL(simpleprocessnodelog) << getLogPrefix() << "output " << numOutput << " does not depend on multi-input " << numMultiInput << std::endl;
	return false;
}

// compile these specializations
template class SimpleProcessNode<FullLockingStrategy>;
template class SimpleProcessNode<InputLockingStrategy>;
template class SimpleProcessNode<OutputLockingStrategy>;
template class SimpleProcessNode<NoLockingStrategy>;

}
