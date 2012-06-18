#ifndef PIPELINE_CALLBACKS_H__
#define PIPELINE_CALLBACKS_H__

#include <signals/Callback.h>
#include "Input.h"

namespace pipeline {

/**
 * This class stores callbacs of the type void(SignalType&, unsigned int) for
 * using multi-inputs. Whenever a new input is assigned to a multi-input, this
 * class registeres a callback of the type void(SignalType&) that calls the
 * first version void(SignalType&, unsigned int) with the number of the input.
 */
class CallbacksBase {

public:

	virtual void registerAtInput(InputBase& input, unsigned int numInput) = 0;

	virtual void registerAtInput(InputBase& input, unsigned int numInput, ProcessNode* processNode) = 0;
};

template <typename SignalType>
class Callbacks : public CallbacksBase {

	typedef boost::function<void(SignalType&, unsigned int)> multi_callback_type;

public:

	Callbacks(multi_callback_type multiCallback) :
		_multiCallback(multiCallback) {}

	void registerAtInput(InputBase& input, unsigned int numInput) {

		signals::Callback<SignalType> callback(boost::bind(_multiCallback, _1, numInput));
		input.registerBackwardCallback(callback);
	}

	void registerAtInput(InputBase& input, unsigned int numInput, ProcessNode* processNode) {

		boost::function<void(SignalType&)> callback = boost::bind(_multiCallback, _1, numInput);
		input.registerBackwardCallback(callback, processNode);
	}

private:

	multi_callback_type _multiCallback;
};

} // namespace pipeline

#endif // PIPELINE_CALLBACKS_H__

