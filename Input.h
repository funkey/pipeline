#ifndef PIPELINE_INPUT_H__
#define PIPELINE_INPUT_H__

#include <boost/type_traits.hpp>

#include <signals/Callback.h>
#include <pipeline/signals/Updated.h>
#include "Data.h"
#include "Output.h"
#include "InputSignals.h"
#include "ProcessNodeCallback.h"

namespace pipeline {

class InputBase {

public:

	InputBase();

	InputBase(std::string name);

	virtual ~InputBase() {

		// destruct all process node callbacks, that have been registered for
		// this input
		for (std::vector<signals::CallbackBase*>::iterator i = _callbacks.begin();
		     i != _callbacks.end(); i++)
			delete *i;
	}

	/**
	 * Set the name of this input.
	 *
	 * @param name The name of this input.
	 */
	void setName(std::string name);

	/**
	 * Get the name of this input.
	 *
	 * @return The name of this input.
	 */
	const std::string& getName();

	/**
	 * Register a slot for backward signals with this input.
	 *
	 * Example usage:
	 * <code>
	 * class ImpatientUpdater : public ProcessNode {
	 *
	 *   Input<Data>        _input;
	 *   Slot<const Update> _update;
	 *
	 * public:
	 *
	 *   ImpatientUpdater() {
	 *
	 *     registerInput(_input);
	 *
	 *     // register the backward signal slot
	 *     _input.registerBackwardSlot(_update);
	 *   }
	 *
	 *   void start() {
	 *
	 *     while (true) {
	 *
	 *       usleep(100);
	 *
	 *       // send an Update signal backwards through the input
	 *       _update(Update());
	 *     }
	 *   }
	 * }
	 * </code>
	 *
	 * @param slot The signal slot to register.
	 */
	void registerBackwardSlot(signals::SlotBase& slot);

	/**
	 * Register a ProcessNode method as a backward callback on an input. This is
	 * a convenience wrapper that creates a ProcessNodeCallback object of the
	 * appropriate signal type and adds it to the input's backward receiver.
	 *
	 * Example usage:
	 * <code>
	 * class ModificationLogger : public ProcessNode {
	 *
	 *   Input<Data> _input;
	 *
	 * public:
	 *
	 *   ModificationLogger() {
	 *
	 *     registerInput(_input);
	 *
	 *     _input.registerBackwardCallback(&ModificationLogger::onModified, this);
	 *   }
	 *
	 * private:
	 *
	 *   void onModified(const Modified& signal) {
	 *
	 *     std::cout << "the input was modified!" << std::endl;
	 *   }
	 * }
	 * </code>
	 *
	 * @param callback    A function pointer to a callback method.
	 * @param processNode Pointer to the object, on which the callback method
	 *                    should be called.
	 */
	template <class T, typename SignalType>
	void registerBackwardCallback(void (T::*callback)(SignalType&), T* processNode, signals::CallbackInvocation invocation = signals::Exclusive) {

		ProcessNodeCallback<SignalType>* processNodeCallback = new ProcessNodeCallback<SignalType>(processNode, boost::bind(callback, static_cast<T*>(processNode), _1), invocation);

		registerBackwardCallback(*processNodeCallback);

		_callbacks.push_back(processNodeCallback);
	}

	/**
	 * Register an arbitrary callback as backward callback on this input. Make
	 * sure that the callback will only be called as long as the given
	 * ProcessNode is still alive.
	 *
	 * @param callback A boost function object.
	 * @param processNode A ProcessNode to track.
	 */
	template <typename SignalType>
	void registerBackwardCallback(boost::function<void(SignalType&)> callback, ProcessNode* processNode, signals::CallbackInvocation invocation = signals::Exclusive) {

		ProcessNodeCallback<SignalType>* processNodeCallback = new ProcessNodeCallback<SignalType>(processNode, callback, invocation);

		registerBackwardCallback(*processNodeCallback);

		_callbacks.push_back(processNodeCallback);
	}

	/**
	 * Register an arbitrary callback as backward callback on this input.
	 *
	 * @param A Callback object.
	 */
	void registerBackwardCallback(signals::CallbackBase& callback);

	/**
	 * Returns true, if this input was assigned an output (it can still have a
	 * value from a shared pointer, though.
	 */
	bool hasAssignedOutput();

	/**
	 * Get a reference to the currently assigned output to this input.
	 *
	 * @return The currently assigned output.
	 */
	OutputBase& getAssignedOutput();

	/**
	 * Get a shared pointer to the currently assigned data.
	 *
	 * @return The currently assigned shared pointer.
	 */
	virtual boost::shared_ptr<Data> getAssignedSharedPtr() const = 0;

	/**
	 * Try to accept an output.
	 */
	virtual bool accept(OutputBase& output) = 0;

	/**
	 * Try to accept a data pointer.
	 */
	virtual bool accept(boost::shared_ptr<Data> data) = 0;

	signals::Sender& getBackwardSender();

	signals::Receiver& getBackwardReceiver();

protected:

	void setAssignedOutput(OutputBase& output);

	void unsetAssignedOutput();

private:

	std::string _name;

	// inputs only send and receive backwards
	signals::Sender   _backwardSender;
	signals::Receiver _backwardReceiver;

	// list of registered process node callbacks created by input
	// (exclusive ownership)
	std::vector<signals::CallbackBase*> _callbacks;

	// the currently assigned output to this input (null, if not assigned)
	OutputBase* _assignedOutput;
};

template <typename DataType>
class InputImpl : public InputBase {

public:

	InputImpl() :
		_inputSet(boost::make_shared<signals::Slot<const InputSet<DataType> > >()),
		_inputSetToSharedPointer(boost::make_shared<signals::Slot<const InputSetToSharedPointer<DataType> > >()),
		_updated(boost::make_shared<signals::Slot<const Updated> >()) {

		_internalSender.registerSlot(*_inputSet);
		_internalSender.registerSlot(*_inputSetToSharedPointer);
		_internalSender.registerSlot(*_updated);
	}

	bool accept(OutputBase& output) {

		if (!output.getData())
			output.createData();

		boost::shared_ptr<DataType> data = boost::dynamic_pointer_cast<DataType>(output.getData());

		if (data) {

			// share ownership to make sure the input data keeps alive
			_data = data;

			// keep the process node of this output alive
			_creator = output.getProcessNode();

			// TODO: disconnect previous internal signalling connections
			// establish the internal signalling connections
			_internalSender.connect(getBackwardReceiver());

			// establish input-output signalling connections
			output.getForwardSender().connect(getBackwardReceiver());
			getBackwardSender().connect(output.getForwardReceiver());

			// remember what output we are using
			setAssignedOutput(output);

			// inform about new input
			(*_inputSet)(InputSet<DataType>(data));

			return true;
		}

		return false;
	}

	bool accept(boost::shared_ptr<Data> data) {

		boost::shared_ptr<DataType> casted_data = boost::dynamic_pointer_cast<DataType>(data);

		if (casted_data) {

			// share ownership to make sure the input data keeps alive
			_data = casted_data;

			// we are not aware of any creator
			_creator.reset();

			// TODO: disconnect previous internal signalling connections
			// establish the internal signalling connections
			_internalSender.connect(getBackwardReceiver());

			// remember that we are not bound to an output
			unsetAssignedOutput();

			// inform about new input
			(*_inputSetToSharedPointer)(InputSetToSharedPointer<DataType>(casted_data));

			// send an updated signal along to mark this input as non-dirty
			(*_updated)();

			return true;
		}

		return false;
	}

	boost::shared_ptr<Data> getAssignedSharedPtr() const {

		return _data;
	}

	boost::shared_ptr<DataType> get() const {

		return _data;
	}

	DataType* operator->() const {

		return _data.operator->();
	}

	DataType& operator*() const {

		return *_data;
	}

	operator boost::shared_ptr<DataType>() {

		return _data;
	}

	/**
	 * Return true if this input is assigned.
	 */
	operator bool() {

		return _data;
	}

private:

	// inputs share ownership of input data
	boost::shared_ptr<DataType> _data;

	// inputs share ownership of process nodes that created the output
	boost::shared_ptr<ProcessNode> _creator;

	// slot to send a signal when the input was set
	boost::shared_ptr<signals::Slot<const InputSet<DataType> > > _inputSet;

	// slot to send a signal when the input was set to a shared pointer
	boost::shared_ptr<signals::Slot<const InputSetToSharedPointer<DataType> > > _inputSetToSharedPointer;

	// updated slot, used for shared pointer inputs
	boost::shared_ptr<signals::Slot<const Updated> >             _updated;

	// internally used sender to inform about a new input
	signals::Sender _internalSender;
};

template <bool, typename T>
class InputTypeDispatch {};

template <typename T>
class InputTypeDispatch<true, T> : public InputImpl<T> {};

template <typename T>
class InputTypeDispatch<false, T> : public InputImpl<Wrap<T> > {

public:

	// transparent unwrapper
	T& operator*() {

		return InputImpl<Wrap<T> >::get()->get();
	}

	// transparent -> operator
	T* operator->() const {

		return &(InputImpl<Wrap<T> >::get()->get());
	}
};

template <typename T>
class Input : public InputTypeDispatch<boost::is_base_of<Data, T>::value, T> {};

} // namespace pipeline

#endif // PIPELINE_INPUT_H__

