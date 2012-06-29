#ifndef OUTPUT_H__
#define OUTPUT_H__

#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <signals/Sender.h>
#include <signals/Receiver.h>
#include <signals/Slot.h>
#include <signals/Callback.h>
#include "Data.h"
#include "Logging.h"
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
	OutputBase();

	/**
	 * Create a new OutputBase.
	 *
	 * @param name The name of this output.
	 */
	OutputBase(std::string name);

	virtual ~OutputBase() {

		// destruct all process node callbacks, that have been registered for
		// this output
		for (std::vector<signals::CallbackBase*>::iterator i = _callbacks.begin();
		     i != _callbacks.end(); i++)
			delete *i;
	}

	/**
	 * Set the name of this output.
	 *
	 * @param name The name of this output.
	 */
	void setName(std::string name);

	/**
	 * Get the name of this output.
	 *
	 * @return The name of this output.
	 */
	std::string getName() const;

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
	 *     _time.registerForwardSlot(_modified);
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
	void registerForwardSlot(signals::SlotBase& slot);

	/**
	 * Register a ProcessNode method as a forward callback on an output. This is
	 * a convenience wrapper that creates a ProcessNodeCallback object of the
	 * appropriate signal type and adds it to the output's forward receiver.
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
	 *     _output.registerForwardCallback(&UpdateLogger::onUpdate, this);
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
	void registerForwardCallback(void (T::*callback)(SignalType&), T* processNode) {

		ProcessNodeCallback<SignalType>* processNodeCallback = new ProcessNodeCallback<SignalType>(processNode, boost::bind(callback, static_cast<T*>(processNode), _1));

		registerForwardCallback(*processNodeCallback);

		_callbacks.push_back(processNodeCallback);
	}

	/**
	 * Register an arbitrary callback as forward callback on this output.
	 *
	 * @param A Callback object.
	 */
	void registerForwardCallback(signals::CallbackBase& callback);

	/**
	 * Set the ProcessNode this output belongs to. This is called by the
	 * ProcessNode when the output is registered via
	 * ProcessNode::registerOutput().
	 *
	 * @param processNode The ProcessNode this output belongs to.
	 */
	void setProcessNode(ProcessNode* processNode);

	/**
	 * Get a shared pointer to the ProcessNode this output belongs to.
	 *
	 * @return A shared pointer to the ProcessNode this output belongs to.
	 */
	boost::shared_ptr<ProcessNode> getProcessNode() const;

	signals::Sender& getForwardSender();

	signals::Receiver& getForwardReceiver();

	virtual void createData() = 0;

	virtual boost::shared_ptr<Data> getData() const = 0;

private:

	std::string _name;

	signals::Sender   _forwardSender;
	signals::Receiver _forwardReceiver;

	// (weak-like) pointer to the ProcessNode this output belongs to
	ProcessNode* _processNode;

	// list of registered process node callbacks created by input
	// (exclusive ownership)
	std::vector<signals::CallbackBase*> _callbacks;
};

template <typename DataType>
class OutputImpl : public OutputBase {

public:

	OutputImpl() {}

	template <typename S>
	OutputImpl(const S& data) :
		OutputBase(),
		_data(data) {}

	void createData() {

		if (!_data) {

			LOG_ALL(pipelinelog) << "[" << typeName(this) << "] creating data" << std::endl;

			// TODO: this should be replaced by a factory
			_data = boost::make_shared<DataType>();

			LOG_ALL(pipelinelog) << "[" << typeName(this) << "] created data" << std::endl;
		}
	}

	boost::shared_ptr<Data> getData() const {

		return get();
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
	 * Return true if the data for this output has been created.
	 */
	operator bool() {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

template <bool, typename T>
class OutputTypeDispatch {};

template <typename T>
class OutputTypeDispatch<true, T> : public OutputImpl<T> {

public:

	OutputTypeDispatch() {};

	template <typename S>
	OutputTypeDispatch(const S& data) :
		OutputImpl<T>(data) {};
};

template <typename T>
class OutputTypeDispatch<false, T> : public OutputImpl<Wrap<T> > {

public:

	OutputTypeDispatch() {};

	template <typename S>
	OutputTypeDispatch(const S& data) :
		OutputImpl<Wrap<T> >(new Wrap<T>(data)) {};

	// transparent unwrapper
	T& operator*() {

		return OutputImpl<Wrap<T> >::get()->get();
	}
};

template <typename T>
class Output : public OutputTypeDispatch<boost::is_base_of<Data, T>::value, T> {

public:

	Output() {};

	template <typename S>
	Output(const S& data) :
		OutputTypeDispatch<boost::is_base_of<Data, T>::value, T>(data) {}
};

} // namespace pipeline

#endif // OUTPUT_H__

