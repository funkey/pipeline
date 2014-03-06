#ifndef OUTPUT_H__
#define OUTPUT_H__

#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <util/exceptions.h>
#include <signals/Sender.h>
#include <signals/Receiver.h>
#include <signals/Slot.h>
#include <signals/Callback.h>
#include "Data.h"
#include "Logging.h"
#include "OutputSignals.h"
#include "ProcessNodeCallback.h"
#include "Wrap.h"

namespace pipeline {

// forward declaration
class ProcessNode;

class OutputBase {

public:

	/**
	 * Create a new OutputBase.
	 */
	OutputBase() :
		_pointerSet(new signals::Slot<OutputPointerSet>()) {

		_sender.registerSlot(*_pointerSet);
	}

	virtual ~OutputBase();

	/**
	 * Register a slot for forward signals with this output.
	 *
	 * Example usage:
	 * <code>
	 * class Clock : public ProcessNode {
	 *
	 *   Output<Time>         _time;
	 *   Slot<const Modified> _modified;
	 *
	 * public:
	 *
	 *   Clock() {
	 *
	 *     registerOutput(_time);
	 *
	 *     // register the forward signal slot
	 *     _time.registerSlot(_modified);
	 *
	 *     (*_time) = 0;
	 *   }
	 *
	 *   void start() {
	 *
	 *     while (true) {
	 *
	 *       usleep(1000);
	 *
	 *       (*_time)++;
	 *
	 *       // send a Modified signal forward through the output
	 *       _modified(Modified());
	 *     }
	 *   }
	 * }
	 * </code>
	 *
	 * @param slot The signal slot to register.
	 */
	void registerSlot(signals::SlotBase& slot);

	/**
	 * Register a ProcessNode method as a forward callback on an output. This is
	 * a convenience wrapper that creates a SharedProcessNodeCallback object of 
	 * the appropriate signal type and adds it to the output's forward receiver.
	 *
	 * Example usage:
	 * <code>
	 * class UpdateLogger : public ProcessNode {
	 *
	 *   Output<Data> _output;
	 *
	 * public:
	 *
	 *   UpdateLogger() {
	 *
	 *     registerOutput(_output);
	 *
	 *     _output.registerCallback(&UpdateLogger::onUpdate, this);
	 *   }
	 *
	 * private:
	 *
	 *   void onUpdate(const Update& signal) {
	 *
	 *     std::cout << "update request received!" << std::endl;
	 *   }
	 * }
	 * </code>
	 *
	 * @param callback    A function pointer to a callback method.
	 * @param processNode Pointer to the object, on which the callback method
	 *                    should be called.
	 */
	template <class T, typename SignalType>
	void registerCallback(void (T::*callback)(SignalType&), T* processNode, signals::CallbackInvocation invocation = signals::Exclusive) {

		SharedProcessNodeCallback<SignalType>* processNodeCallback = new SharedProcessNodeCallback<SignalType>(processNode, boost::bind(callback, processNode, _1), invocation);

		registerCallback(*processNodeCallback);

		_callbacks.push_back(processNodeCallback);
	}

	/**
	 * Register an arbitrary callback as forward callback on this output. Make
	 * sure that the callback will only be called as long as the given
	 * ProcessNode is still alive.
	 *
	 * @param callback A boost function object.
	 * @param processNode A ProcessNode to track.
	 */
	template <typename SignalType>
	void registerCallback(boost::function<void(SignalType&)> callback, ProcessNode* processNode, signals::CallbackInvocation invocation = signals::Exclusive) {

		SharedProcessNodeCallback<SignalType>* processNodeCallback = new SharedProcessNodeCallback<SignalType>(processNode, callback, invocation);

		registerCallback(*processNodeCallback);

		_callbacks.push_back(processNodeCallback);
	}

	/**
	 * Register an arbitrary callback as forward callback on this output.
	 *
	 * @param A Callback object.
	 */
	void registerCallback(signals::CallbackBase& callback);

	/**
	 * Add a process node as a dependency of this output.
	 */
	void addDependency(ProcessNode* processNode);

	/**
	 * Get shared pointers to the process nodes this output depends on.
	 */
	std::vector<boost::shared_ptr<ProcessNode> > getDependencies() const;

	signals::Sender& getSender();

	signals::Receiver& getReceiver();

	/**
	 * Get a shared pointer to the Data instance held by this output.
	 */
	virtual boost::shared_ptr<Data> getSharedDataPointer() const = 0;

protected:

	/**
	 * Send a signal to the connected inputs to inform them about a data pointer 
	 * change.
	 */
	void notifyPointerSet();

private:

	signals::Sender   _sender;
	signals::Receiver _receiver;

	// (weak-like) pointers to the ProcessNode this output depends on
	std::vector<ProcessNode*> _dependencies;

	// list of registered process node callbacks created by input
	// (exclusive ownership)
	std::vector<signals::CallbackBase*> _callbacks;

	// a slot to send a signal on data pointer changes
	boost::shared_ptr<signals::Slot<OutputPointerSet> > _pointerSet;
};

template <typename DataType>
class OutputImpl : public OutputBase {

public:

	/**
	 * Default constructor.
	 */
	OutputImpl() {}

	OutputImpl(DataType* data) : _data(data) {}

	OutputImpl(boost::shared_ptr<DataType> data) : _data(data) {}

	/**
	 * Set the data of this output. The lifetime of the given object will be 
	 * managed by a shared pointer.
	 */
	OutputImpl& operator=(DataType* data) {

		_data = boost::shared_ptr<DataType>(data);
		notifyPointerSet();

		return *this;
	}

	/**
	 * Set the data of this output.
	 */
	OutputImpl& operator=(boost::shared_ptr<DataType> data) {

		_data = data;
		notifyPointerSet();

		return *this;
	}

	/**
	 * Unset the data of this output. The data will be destructed, if no other 
	 * object holds a shared pointer to it.
	 */
	void reset() {

		_data.reset();
	}

	/**
	 * Get the data held by this output.
	 */
	DataType* get() const {

		return _data.get();
	}

	/**
	 * Member access to the data held by this output.
	 */
	DataType* operator->() const {

		return _data.operator->();
	}

	/**
	 * Dereferencation of the data held by this output.
	 */
	DataType& operator*() const {

		return *_data;
	}

	/**
	 * Returns true, if this output holds data.
	 */
	operator bool() const {

		return _data;
	}

	/**
	 * Get a shared pointer to the Data instance held by this output.
	 */
	boost::shared_ptr<Data> getSharedDataPointer() const {

		return _data;
	}

	/**
	 * Get a shared pointer to the concrete data type object held by this 
	 * output.
	 */
	boost::shared_ptr<DataType> getSharedPointer() const {

#ifndef NDEBUG
		if (!_data)
			BOOST_THROW_EXCEPTION(NullPointer() << error_message("This output does not point to valid data") << STACK_TRACE);
#endif

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

template <bool, typename T>
class OutputTypeDispatch {};

/**
 * Dispatch class for Outputs of type T, where T is derived from Data.
 */
template <typename T>
class OutputTypeDispatch<true, T> : public OutputImpl<T> {

	typedef OutputImpl<T> parent_type;

public:

	OutputTypeDispatch() {}

	OutputTypeDispatch(T* data) : parent_type(data) {}

	OutputTypeDispatch(boost::shared_ptr<T> data) : parent_type(data) {}

	using parent_type::operator=;
};

/**
 * Dispatch class for Outputs of type T, where T is not derived from Data. In 
 * this case, T will be wrapped into Wrap<T>, which is derived from Data.
 */
template <typename T>
class OutputTypeDispatch<false, T> : public OutputImpl<Wrap<T> > {

	typedef OutputImpl<Wrap<T> > parent_type;

public:

	OutputTypeDispatch() {}

	OutputTypeDispatch(T* data) : parent_type(new Wrap<T>(boost::shared_ptr<T>(data))) {}

	OutputTypeDispatch(boost::shared_ptr<T> data) : parent_type(new Wrap<T>(data)) {}

	/**
	 * Set the data of this output. The lifetime of the given object will be 
	 * managed by a shared pointer.
	 */
	OutputTypeDispatch& operator=(T* data) {

		parent_type::operator=(new Wrap<T>(boost::shared_ptr<T>(data)));
		return *this;
	}

	/**
	 * Set the data of this output.
	 */
	OutputTypeDispatch& operator=(boost::shared_ptr<T> data) {

		parent_type::operator=(new Wrap<T>(data));
		return *this;
	}

	/**
	 * Get the data held by this output.
	 */
	T* get() const {

		return parent_type::getSharedPointer()->getSharedPointer().get();
	}

	/**
	 * Member access to the data held by this output.
	 */
	T* operator->() const {

		return parent_type::getSharedPointer()->getSharedPointer().operator->();
	}

	/**
	 * Dereferencation of the data held by this output.
	 */
	T& operator*() const {

		return *parent_type::getSharedPointer()->get();
	}

	using parent_type::operator=;
};

template <typename T>
class Output : public OutputTypeDispatch<boost::is_base_of<Data, T>::value, T> {

	typedef OutputTypeDispatch<boost::is_base_of<Data, T>::value, T> parent_type;

public:

	/**
	 * Default constructur. Creates an output instance with an uninitialized 
	 * data pointer.
	 */
	Output() {}

	/**
	 * Creates an output instance from a data object pointer. The lifetime of 
	 * the data object will be managed by a shared pointer.
	 */
	Output(T* data) : parent_type(data) {}

	/**
	 * Creates an output instance from a shared data object pointer.
	 */
	Output(boost::shared_ptr<T> data) : parent_type(data) {}

	using parent_type::operator=;
};

} // namespace pipeline

#endif // OUTPUT_H__

