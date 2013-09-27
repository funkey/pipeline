#ifndef PIPELINE_SIGNAL_FILTER_H__
#define PIPELINE_SIGNAL_FILTER_H__

#include <signals/Slot.h>
#include <signals/Slots.h>
#include "Output.h"
#include "Input.h"
#include "Inputs.h"

/**
 * Defines the class template SignalFilter. Use this to create signal filters in 
 * the following way (assuming 'using namespace pipeline'):
 *
 *   class MyFilter : public
 *       SignalFilter<
 *           FilterSignal<A,
 *           FilterSignal<B,
 *           FilterSignalsAs<C> > > {};
 *
 * This creates a signal filter, that will call your implementation of
 *
 *   filter(C& signal)
 *
 * and
 *
 *   filter(C& signal, unsigned int i)
 *
 * for all signals of type A and B that are sent between the input and output 
 * you registered this filter with.
 *
 * Your class has to implement the filter(C& signal) and filter(C& signal, 
 * unsigned int i) method. The latter one is called for filters that are 
 * connected to multi-inputs and are sending to them. In this case, your 
 * callback will be called for every attached input, where i is the number of 
 * the input.
 */

namespace pipeline {

/**
 * Terminator of a signal list.
 */
template <typename S>
struct FilterSignalsAs {

	typedef S SignalType;
};

/**
 * A list of signals, terminated by a fallback signal type, which defaults to 
 * the most general signal::Signal.
 */
template <typename H, typename T = FilterSignalsAs<signals::Signal> >
struct FilterSignal {

	typedef H Head;
	typedef T Tail;
};

template <typename Signals>
class SignalFilterImpl : public SignalFilterImpl<typename Signals::Tail> {

	typedef SignalFilterImpl<Signals>                MyType;
	typedef typename Signals::Head                   SignalType;
	typedef SignalFilterImpl<typename Signals::Tail> ParentType;

protected:

	void filterBackward(pipeline::OutputBase& output, pipeline::InputBase& input, pipeline::ProcessNode* processNode) {

		// establish callback connection to method of this class for signal 
		// receiving
		output.registerForwardCallback(boost::function<void(SignalType&)>(boost::bind(&MyType::onSignal, this, _1)), processNode, signals::Transparent);

		// connect slot of this class for signal sending
		input.registerBackwardSlot(_slot);

		// delegate to other signals filters
		ParentType::filterBackward(output, input, processNode);
	}

	void filterBackward(pipeline::OutputBase& output, pipeline::MultiInput& inputs, pipeline::ProcessNode* processNode) {

		// establish callback connection to method of this class
		output.registerForwardCallback(boost::function<void(SignalType&)>(boost::bind(&MyType::onSignalMulti, this, _1)), processNode, signals::Transparent);

		// connect slot of this class for signal sending
		inputs.registerBackwardSlots(_slots);

		// delegate to other signals filters
		ParentType::filterBackward(output, inputs, processNode);
	}

private:

	void onSignal(SignalType& signal) {

		if (signal.processed)
			return;

		// call filter implementation
		this->filter(signal);

		// send signal to registered inputs
		_slot(signal);
	}

	void onSignalMulti(SignalType& signal) {

		if (signal.processed)
			return;

		// for each registered input to this multi-slot
		for (unsigned int i = 0; i < _slots.size(); i++) {

			// create a copy of the signal
			SignalType copy = signal;

			// call filter implementation
			this->filter(copy, i);

			// send signal to registered inputs
			_slots[i](copy);
		}
	}

	signals::Slot<SignalType>  _slot;
	signals::Slots<SignalType> _slots;
};

template <typename FallbackSignalType>
class SignalFilterImpl<FilterSignalsAs<FallbackSignalType> > {

protected:

	/**
	 * The filter method, to be implemented by subclasses.
	 *
	 * @return
	 *         Implementations should return false, if the given signal should 
	 *         not be forwarded.
	 */
	virtual bool filter(FallbackSignalType& /*signal*/) { return true; }

	/**
	 * Filter method for one-to-many filters (from one output to several 
	 * inputs). Will be called with the appropriate input number. To be 
	 * implemented by subclasses.
	 *
	 * @return
	 *         Implementations should return false, if the given signal should 
	 *         not be forwarded.
	 */
	virtual bool filter(FallbackSignalType& /*signal*/, unsigned int /*input*/) { return true; }

	void filterBackward(pipeline::OutputBase& /*output*/, pipeline::InputBase&  /*input*/,  pipeline::ProcessNode* /*processNode*/) {}
	void filterBackward(pipeline::OutputBase& /*output*/, pipeline::MultiInput& /*inputs*/, pipeline::ProcessNode* /*processNode*/) {}

};

template <typename Signals>
class SignalFilter : public SignalFilterImpl<Signals> {

public:

	void filterBackward(pipeline::OutputBase& output, pipeline::InputBase&  input,  pipeline::ProcessNode* processNode) {

		SignalFilterImpl<Signals>::filterBackward(output, input, processNode);
	}

	void filterBackward(pipeline::OutputBase& output, pipeline::MultiInput& inputs, pipeline::ProcessNode* processNode) {

		SignalFilterImpl<Signals>::filterBackward(output, inputs, processNode);
	}
};

} // namespace pipeline

#endif // PIPELINE_SIGNAL_FILTER_H__

