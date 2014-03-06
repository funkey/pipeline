#ifndef PROCESS_NODE_CALLBACK_H__
#define PROCESS_NODE_CALLBACK_H__

#include <boost/bind.hpp>

#include <signals/Callback.h>
#include "ProcessNodeTracking.h"

namespace pipeline {

/**
 * Callback for inputs. Slots that use callbacks of this type track the owning 
 * process node with a weak pointer to make sure the callback is still valid.
 */
template <
	typename SignalType,
	template <typename ToType> class CastingPolicy = signals::StaticCast
>
class WeakProcessNodeCallback : public signals::Callback<SignalType, WeakProcessNodeTracking, CastingPolicy> {

public:

	WeakProcessNodeCallback(ProcessNode* processNode, boost::function<void(SignalType&)> callback, signals::CallbackInvocation invocation) :
		signals::Callback<SignalType, WeakProcessNodeTracking, CastingPolicy>(callback, invocation) {

		WeakProcessNodeTracking::track(processNode);
	}
};

/**
 * Callback for outputs. Slots that use callbacks of this type track the owning 
 * process node with a shared pointer and therefore prevent it from destruction 
 * as long as they are still connected to the callback.
 */
template <
	typename SignalType,
	template <typename ToType> class CastingPolicy = signals::StaticCast
>
class SharedProcessNodeCallback : public signals::Callback<SignalType, SharedProcessNodeTracking, CastingPolicy> {

public:

	SharedProcessNodeCallback(ProcessNode* processNode, boost::function<void(SignalType&)> callback, signals::CallbackInvocation invocation) :
		signals::Callback<SignalType, SharedProcessNodeTracking, CastingPolicy>(callback, invocation) {

		SharedProcessNodeTracking::track(processNode);
	}
};

} // namespace pipeline

#endif // PROCESS_NODE_CALLBACK_H__

