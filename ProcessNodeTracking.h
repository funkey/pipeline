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

	template <typename SignalType>
	boost::function<void(SignalType&)> wrap(boost::function<void(SignalType&)> callback) {

		typedef boost::signals2::signal<void(SignalType&)> boost_signal_type;

		return typename boost_signal_type::slot_type(callback).track(getHolder());
	}

private:

	mutable ProcessNode* _holder;
};

} // namespace pipeline

#endif // PROCESS_NODE_TRACKING_H__

