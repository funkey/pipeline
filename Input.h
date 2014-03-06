#ifndef PIPELINE_INPUT_H__
#define PIPELINE_INPUT_H__

#include <boost/type_traits.hpp>

#include <signals/Callback.h>
#include <util/deprecated.h>
#include "exceptions.h"
#include "Data.h"
#include "Output.h"
#include "InputSignals.h"
#include "ProcessNodeCallback.h"

namespace pipeline {

class InputBase {

public:

	struct AssignmentError : virtual PipelineError {};

	InputBase();

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

		boost::shared_ptr<ProcessNodeCallback<SignalType> > processNodeCallback = boost::make_shared<ProcessNodeCallback<SignalType> >(processNode, boost::bind(callback, static_cast<T*>(processNode), _1), invocation);

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

		boost::shared_ptr<ProcessNodeCallback<SignalType> > processNodeCallback = boost::make_shared<ProcessNodeCallback<SignalType> >(processNode, callback, invocation);

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
	bool hasAssignedOutput() const;

	/**
	 * Get a reference to the currently assigned output to this input.
	 *
	 * @return The currently assigned output.
	 */
	OutputBase& getAssignedOutput() const;

	/**
	 * Get a shared pointer to the currently assigned data.
	 *
	 * @return The currently assigned shared pointer.
	 */
	virtual boost::shared_ptr<Data> getSharedDataPointer() const = 0;

	/**
	 * Try to accept an output.
	 */
	virtual bool accept(OutputBase& output) = 0;

	/**
	 * Try to accept a data pointer.
	 */
	virtual bool accept(boost::shared_ptr<Data> data) = 0;

	/**
	 * Unset this input.
	 */
	virtual void unset() = 0;

	/**
	 * Returns true, if this input is assigned.
	 */
	virtual operator bool() const = 0;

	signals::Sender& getBackwardSender();

	signals::Receiver& getBackwardReceiver();

protected:

	void setAssignedOutput(OutputBase& output);

	void unsetAssignedOutput();

private:

	// inputs only send and receive backwards
	signals::Sender   _backwardSender;
	signals::Receiver _backwardReceiver;

	// list of registered process node callbacks created by input
	// (exclusive ownership)
	std::vector<boost::shared_ptr<signals::CallbackBase> > _callbacks;

	// the currently assigned output to this input (null, if not assigned)
	OutputBase* _assignedOutput;
};

template <typename DataType>
class InputImpl : public InputBase {

public:

	InputImpl() :
		_inputSet(boost::make_shared<signals::Slot<const InputSet<DataType> > >()),
		_inputSetToSharedPointer(boost::make_shared<signals::Slot<const InputSetToSharedPointer<DataType> > >()),
		_inputUnset(boost::make_shared<signals::Slot<const InputUnset<DataType> > >()),
		_outputPointerSetCallback(new signals::Callback<OutputPointerSet>(boost::bind(&InputImpl<DataType>::onOutputPointerSet, this, _1))) {

		_internalSender.registerSlot(*_inputSet);
		_internalSender.registerSlot(*_inputSetToSharedPointer);
		_internalSender.registerSlot(*_inputUnset);

		getBackwardReceiver().registerCallback(*_outputPointerSetCallback);
	}

	bool accept(OutputBase& output) {

		// establish input-output signalling connections
		output.getForwardSender().connect(getBackwardReceiver());
		getBackwardSender().connect(output.getForwardReceiver());

		// establish the internal signalling connections
		_internalSender.connect(getBackwardReceiver());

		// remember what output we are using
		setAssignedOutput(output);

		// if there is already data on the output
		if (output.getSharedDataPointer())
			setDataFromOutput(output.getSharedDataPointer());

		return true;
	}

	bool accept(boost::shared_ptr<Data> data) {

		// establish the internal signalling connections
		_internalSender.connect(getBackwardReceiver());

		// remember that we are not bound to an output
		unsetAssignedOutput();

		setDataFromPointer(data);

		return true;
	}

	void unset() {

		// get a shared pointer to the data for the signal
		boost::shared_ptr<DataType> oldData = _data;

		// reset shared pointer to data
		_data.reset();

		if (hasAssignedOutput()) {

			// tear-down input-output signalling connections
			getAssignedOutput().getForwardSender().disconnect(getBackwardReceiver());
			getBackwardSender().disconnect(getAssignedOutput().getForwardReceiver());

			// we are not assigned to any output any more
			unsetAssignedOutput();
		}

		// inform about unset of input
		(*_inputUnset)(InputUnset<DataType>(oldData));
	}

	/**
	 * Get a shared pointer to the Data object assigned to this input.
	 */
	boost::shared_ptr<Data> getSharedDataPointer() const {

		return _data;
	}

	/**
	 * Get a shared pointer to the concrete DataType object assigned to this 
	 * input.
	 */
	boost::shared_ptr<DataType> getSharedPointer() const {

		return _data;
	}

	/**
	 * Get the data assigned to this input.
	 */
	DataType* get() const {

		return _data.get();
	}

	/**
	 * Member access to the data assigned to this input.
	 */
	DataType* operator->() const {

		return _data.operator->();
	}

	/**
	 * Dereferencation of the data assigned to this input.
	 */
	DataType& operator*() const {

		return *_data;
	}

	/**
	 * Return true if this input points to data.
	 */
	DEPRECATED(operator bool() const) {

		return _data;
	}

	/**
	 * For convencience, implicit conversion to shared pointer to DataType.
	 */
	operator boost::shared_ptr<DataType>() const {

		return _data;
	}

private:

	void setDataFromOutput(boost::shared_ptr<Data> data) {

		boost::shared_ptr<DataType> castedData = boost::dynamic_pointer_cast<DataType>(data);

		if (!castedData) {

			std::stringstream error;
			error << "output of type " << typeName(*data) << " can not be assigned to input of type " << typeName(*this) << std::endl;

			BOOST_THROW_EXCEPTION(AssignmentError() << error_message(error.str()) << STACK_TRACE);
		}

		// share ownership to make sure the input data keeps alive
		_data = castedData;

		// inform about new input
		(*_inputSet)(InputSet<DataType>(castedData));
	}

	void setDataFromPointer(boost::shared_ptr<Data> data) {

		boost::shared_ptr<DataType> castedData = boost::dynamic_pointer_cast<DataType>(data);

		if (!castedData) {

			std::stringstream error;
			error << "pointer of type " << typeName(*data) << " can not be assigned to input of type " << typeName(*this) << std::endl;

			BOOST_THROW_EXCEPTION(AssignmentError() << error_message(error.str()) << STACK_TRACE);
		}

		// share ownership to make sure the input data keeps alive
		_data = castedData;

		// inform about new input
		(*_inputSetToSharedPointer)(InputSetToSharedPointer<DataType>(castedData));
	}

	void onOutputPointerSet(const OutputPointerSet&) {

		setDataFromOutput(getAssignedOutput().getSharedDataPointer());
	}

	// inputs share ownership of input data
	boost::shared_ptr<DataType> _data;

	// slot to send a signal when the input was set
	boost::shared_ptr<signals::Slot<const InputSet<DataType> > > _inputSet;

	// slot to send a signal when the input was set to a shared pointer
	boost::shared_ptr<signals::Slot<const InputSetToSharedPointer<DataType> > > _inputSetToSharedPointer;

	// slot to send a signal when the input was unset
	boost::shared_ptr<signals::Slot<const InputUnset<DataType> > > _inputUnset;

	// internally used sender for the slots defined above
	signals::Sender _internalSender;

	// callback for OutputPointerSet signals
	boost::shared_ptr<signals::Callback<OutputPointerSet> > _outputPointerSetCallback;
};

template <bool, typename T>
class InputTypeDispatch {};

template <typename T>
class InputTypeDispatch<true, T> : public InputImpl<T> {};

template <typename T>
class InputTypeDispatch<false, T> : public InputImpl<Wrap<T> > {

	typedef InputImpl<Wrap<T> > parent_type;

public:

	/**
	 * Get the data assigned to this input.
	 */
	T* get() const {

		return parent_type::getSharedPointer()->getSharedPointer().get();
	}

	/**
	 * Member access to the data assigned to this input.
	 */
	T* operator->() const {

		return parent_type::getSharedPointer()->getSharedPointer().operator->();
	}

	/**
	 * Dereferencation of the data assigned to this input.
	 */
	T& operator*() const {

		return *parent_type::getSharedPointer()->get();
	}
};

template <typename T>
class Input : public InputTypeDispatch<boost::is_base_of<Data, T>::value, T> {};

} // namespace pipeline

#endif // PIPELINE_INPUT_H__

