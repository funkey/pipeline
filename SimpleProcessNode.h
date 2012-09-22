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
 * Full input/output locking strategy. Safe, but potentially inefficient,
 * locking mechanism for output updates. Allocates read locks on all inputs and
 * write locks on all outputs before calling updateOutputs().
 */
class FullLockingStrategy {

public:

	void lock(InputBase& input, boost::function<void()> next) {

		if (input.hasAssignedOutput()) {

			boost::shared_lock<boost::shared_mutex> lock(input.getAssignedSharedPtr()->getMutex());

			next();

		} else {

			next();
		}
	}

	void lock(OutputBase& output, boost::function<void()> next) {

		boost::unique_lock<boost::shared_mutex> lock(output.getData()->getMutex());

		next();
	}
};

/**
 * Don't perform input/output locking on updateOutputs. Use this strategy if you
 * want to control which inputs and outputs to lock yourself.
 */
class NoLockingStrategy {

public:

	void lock(InputBase& input, boost::function<void()> next) {

		next();
	}

	void lock(OutputBase& output, boost::function<void()> next) {

		next();
	}
};

template <class LockingStrategy = FullLockingStrategy>
class SimpleProcessNodeImpl : public LockingStrategy, public ProcessNode {

public:

	SimpleProcessNodeImpl();

	virtual ~SimpleProcessNodeImpl();

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
	 *
	 * Thread save (by locking). Don't call from updateOutputs().
	 */
	void setDirty(OutputBase& output);

private:

	void onInputModified(const Modified& signal, int numInput);

	void onInputSet(const InputSetBase& signal, int numInput);

	void onInputSetToSharedPointer(const InputSetBase& signal, int numInput);

	void onInputAdded(const InputAddedBase& signal, int numMultiInput);

	void onMultiInputModified(const Modified& signal, int numInput, int numMultiInput);

	void lockInputs(int i) {

		if (i == _numInputs) {

			lockOutputs(0);
			return;
		}

		LockingStrategy::lock(getInput(i), boost::bind(&SimpleProcessNodeImpl::lockInputs, this, i + 1));
	}

	void lockOutputs(int i) {

		if (i == _numOutputs) {

			updateOutputs();
			return;
		}

		LockingStrategy::lock(getOutput(i), boost::bind(&SimpleProcessNodeImpl::lockOutputs, this, i + 1));
	}

	void onUpdate(const Update& signal, int numOutput);

	// thread save (by locking)
	void sendUpdateSignals();

	void sendModifiedSignals();

	bool haveDirtyInput();

	bool haveDirtyOutput();

	void setOutputsDirty(bool dirty = true);

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

	// a mutex to protect concurrent updates
	boost::mutex _updateMutex;
};

typedef SimpleProcessNodeImpl<FullLockingStrategy> SimpleProcessNode;

}

#endif // PIPELINE_SIMPLE_PROCESS_NODE_H__

