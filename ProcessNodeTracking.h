#ifndef PROCESS_NODE_TRACKING_H__
#define PROCESS_NODE_TRACKING_H__

#include <boost/shared_ptr.hpp>
#include <signals/CallbackInvoker.h>

namespace pipeline {

// forward declaration
class ProcessNode;

class ProcessNodeTracking {

public:

	void track(ProcessNode* holder) const {

		_holder = holder;
	}

protected:

	boost::shared_ptr<ProcessNode> getHolder();

private:

	mutable ProcessNode* _holder;
};

/**
 * Weak pointer tracking strategy for callbacks. For callbacks that use this 
 * strategy, a connected slot will keep a weak pointer to the callback's holder 
 * (set via track() on the callback). The weak pointer is locked whenever a 
 * signal needs to be sent. If locking fails, i.e., the holder does not live 
 * anymore, the callback gets automatically removed from the slot.
 */
class WeakProcessNodeTracking : public ProcessNodeTracking {

public:

	template <typename SignalType>
	signals::CallbackInvoker<SignalType> createInvoker(boost::reference_wrapper<boost::function<void(SignalType&)> > callback) {

		signals::CallbackInvoker<SignalType> invoker(callback);
		invoker.setWeakTracking(getHolder());

		return invoker;
	}
};

/**
 * Shared pointer tracking for callbacks. For callbacks that use this strategy, 
 * a connected slot will keep a shared pointer to the callback's holder and thus 
 * makes sure that the holder will live at least as long as the connection to 
 * the slot is established.
 */
class SharedProcessNodeTracking : public ProcessNodeTracking {

public:

	template <typename SignalType>
	signals::CallbackInvoker<SignalType> createInvoker(boost::reference_wrapper<boost::function<void(SignalType&)> > callback) {

		signals::CallbackInvoker<SignalType> invoker(callback);
		invoker.setSharedTracking(getHolder());

		return invoker;
	}
};

} // namespace pipeline

#endif // PROCESS_NODE_TRACKING_H__

