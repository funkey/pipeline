#ifndef PROCESS_NODE_CALLBACK_H__
#define PROCESS_NODE_CALLBACK_H__

#include <boost/bind.hpp>

#include <signals/Callback.h>
#include "ProcessNodeTracking.h"

namespace pipeline {

template <
	typename SignalType,
	template <typename ToType> class CastingPolicy = signals::StaticCast
>
class ProcessNodeCallback : public signals::Callback<SignalType, ProcessNodeTracking, CastingPolicy> {

public:

	ProcessNodeCallback(ProcessNode* processNode, boost::function<void(SignalType&)> callback, signals::CallbackInvocation invocation) :
		signals::Callback<SignalType, ProcessNodeTracking, CastingPolicy>(callback, invocation) {

		ProcessNodeTracking::track(processNode);
	}
};

} // namespace pipeline

#endif // PROCESS_NODE_CALLBACK_H__

