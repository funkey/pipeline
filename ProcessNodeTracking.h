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

	template <typename CallbackType>
	typename boost::signals2::signal<void(typename CallbackType::signal_type&)>::slot_type wrap(CallbackType& callback) {

		typedef typename CallbackType::signal_type          signal_type;
		typedef boost::signals2::signal<void(signal_type&)> boost_signal_type;
		typedef typename boost_signal_type::slot_type       boost_slot_type;

		return boost_slot_type(boost::ref(callback)).track(getHolder());
	}

protected:

	boost::shared_ptr<ProcessNode> getHolder();

private:

	mutable ProcessNode* _holder;
};

} // namespace pipeline

#endif // PROCESS_NODE_TRACKING_H__

