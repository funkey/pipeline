#ifndef PROCESS_NODE_TRACKING_H__
#define PROCESS_NODE_TRACKING_H__

#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>

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

	template <typename CallbackType>
	typename boost::signals2::signal<void(typename CallbackType::signal_type&)>::slot_type wrap(CallbackType& callback) {

		typedef typename CallbackType::signal_type          signal_type;
		typedef boost::signals2::signal<void(signal_type&)> boost_signal_type;
		typedef typename boost_signal_type::slot_type       boost_slot_type;

		return boost_slot_type(boost::ref(callback)).track(getHolder());
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

	template <typename CallbackType>
	struct SharedHolderCallback {

		SharedHolderCallback(CallbackType& callback_, boost::shared_ptr<ProcessNode> holder_) :
			callback(&callback_),
			holder(holder_) {}

		void operator()(typename CallbackType::signal_type& signal) {

			(*callback)(signal);
		}

		CallbackType* callback;
		boost::shared_ptr<ProcessNode> holder;
	};

	template <typename CallbackType>
	SharedHolderCallback<CallbackType> wrap(CallbackType& callback) {

		boost::shared_ptr<ProcessNode> sharedHolder = getHolder();

		return SharedHolderCallback<CallbackType>(callback, sharedHolder);
	}
};

} // namespace pipeline

#endif // PROCESS_NODE_TRACKING_H__

