#ifndef PIPELINE_SIMPLE_PROCESS_NODE_H__
#define PIPELINE_SIMPLE_PROCESS_NODE_H__

#include <boost/thread/mutex.hpp>

#include <pipeline/signals/all.h>

namespace pipeline {

enum InputType {

	Required,
	Optional
};

/**
 * Don't perform input/output locking on updateOutputs. Use this strategy if you
 * want to control which inputs and outputs to lock yourself.
 */
class NoLockingStrategy {

public:

	void lockInput(InputBase&, boost::function<void()> next) {

		next();
	}

	void lockOutput(OutputBase&, boost::function<void()> next) {

		next();
	}
};

/**
 * Lock only the inputs. Allocates read locks on all inputs.
 */
class InputLockingStrategy : public NoLockingStrategy {

public:

	void lockInput(InputBase& input, boost::function<void()> next) {

		if (input.hasAssignedOutput()) {

			boost::shared_lock<boost::shared_mutex> lock(input.getAssignedSharedPtr()->getMutex());

			next();

		} else {

			next();
		}
	}

	using NoLockingStrategy::lockOutput;
};

/**
 * Lock only the outputs. Allocates write locks on all outputs.
 */
class OutputLockingStrategy : public NoLockingStrategy {

public:

	void lockOutput(OutputBase& output, boost::function<void()> next) {

		if (!output)
			output.createData();

		// Instantiation here ensures that the output data survives, even if the 
		// owning Output decides to replace it. Since we are using the mutex of 
		// the data and not of the output, we have to make sure the data does 
		// not get destructed before we are done.
		boost::shared_ptr<Data> data = output.getData();

		boost::unique_lock<boost::shared_mutex> lock(data->getMutex());

		next();
	}

	using NoLockingStrategy::lockInput;
};

/**
 * Full input/output locking strategy. Safe, but potentially inefficient,
 * locking mechanism for output updates. Allocates read locks on all inputs and
 * write locks on all outputs before calling updateOutputs().
 */
class FullLockingStrategy : public InputLockingStrategy, public OutputLockingStrategy {

public:

	using InputLockingStrategy::lockInput;
	using OutputLockingStrategy::lockOutput;
};

template <class LockingStrategy = FullLockingStrategy>
class SimpleProcessNode : public LockingStrategy, public ProcessNode {

public:

	SimpleProcessNode(std::string name = "");

	virtual ~SimpleProcessNode();

	const std::string& getName() { return _name; }

protected:

	/**
	 * Overwritten from ProcessNode.
	 */
	void registerInput(InputBase& input, std::string name, InputType = Required);

	/**
	 * Overwritten from ProcessNode.
	 */
	void registerInputs(MultiInput& input, std::string name);

	/**
	 * Overwritten from ProcessNode.
	 */
	void registerOutput(OutputBase& output, std::string name);

	/**
	 * Overwrite this method in derived classes to (re)compute the output.
	 * Within this method you can assume that all inputs are up-to-date.
	 *
	 * Thread save (by locking).
	 */
	virtual void updateOutputs() = 0;

	/**
	 * Explicitly update the inputs of this process node. Usually, you don't
	 * need to call this function yourself. It will be called automatically
	 * whenever another process node is asking for an update of your output.
	 * However, if this is a sink node, e.g. file writer, and you want to make
	 * sure all the inputs are up-to-date before writing, you would call this
	 * method in your write method.
	 *
	 * Thread save.
	 */
	void updateInputs();

	/**
	 * Explicitly set one of the outputs dirty. This will cause other process
	 * nodes to be informed accordingly. Use this method whenever you change the
	 * internal state of your process node without using the pipeline
	 * architecture. For example, if your process node has a value 'factor' that
	 * is not an Input but can be changed by the user directly, then for all
	 * outputs that depend on 'factor' you would call this method.
	 *
	 * @param output The output to set dirty.
	 */
	void setDirty(OutputBase& output);

private:

	void onInputModified(const Modified& signal, int numInput);

	void onInputSet(const InputSetBase& signal, int numInput);

	void onInputSetToSharedPointer(const InputSetToSharedPointerBase& signal, int numInput);

	void onInputAdded(const InputAddedBase& signal, int numMultiInput);

	void onInputsCleared(const InputsCleared& signal, int numMultiInput);

	void onMultiInputModified(const Modified& signal, int numInput, int numMultiInput);

	void lockInputs(int i) {

		if (i == _numInputs) {

			lockOutputs(0);
			return;
		}

		LockingStrategy::lockInput(getInput(i), boost::bind(&SimpleProcessNode::lockInputs, this, i + 1));
	}

	void lockOutputs(int i) {

		if (i == _numOutputs) {

			updateOutputs();
			return;
		}

		LockingStrategy::lockOutput(getOutput(i), boost::bind(&SimpleProcessNode::lockOutputs, this, i + 1));
	}

	void onUpdate(const Update& signal, int numOutput);

	// thread save (by locking)
	void sendUpdateSignals();

	void sendModifiedSignals();

	bool haveDirtyInput();

	unsigned int numDirtyInputs();

	bool haveDirtyOutput();

	void setOutputsDirty(bool dirty = true);

	bool requiredInputsPresent();

	std::string getLogPrefix() { return std::string("[") + typeName(*this) + (_name.size() ? std::string("(") + _name + ")]" : "]"); }

	// one boolean for each input
	std::vector<bool> _inputDirty;

	// a vector of booleans for each multi-input
	std::vector<std::vector<bool> > _multiInputDirty;

	// one update slot for each input
	signals::Slots<Update>  _inputUpdate;

	// a vector of slots for each multi-input
	std::vector<signals::Slots<Update>*> _multiInputUpdates;

	// one modified slot for each output
	signals::Slots<Modified> _modified;

	// the current number of inputs
	int _numInputs;

	// the current number of multi-inputs
	int _numMultiInputs;

	// the current number of outputs
	int _numOutputs;

	// indicates that an output has to be recomputed
	std::vector<bool> _outputDirty;

	// a look-up table from outputs to their number
	std::map<OutputBase*, unsigned int> _outputNums;

	// indicates that an input is required for the output update
	std::vector<bool> _inputRequired;

	// a mutex to protect concurrent updates
	boost::mutex _updateMutex;

	// a mutex to protect access to the _[multiI|i]nputDirty vectors
	boost::mutex _inputDirtyMutex;

	// the maximal number of threads
	static int _numThreads;

	static boost::mutex _threadCountMutex;

	// name to identify this process node in the logs
	std::string _name;
};

}

#endif // PIPELINE_SIMPLE_PROCESS_NODE_H__

