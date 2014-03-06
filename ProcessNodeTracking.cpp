#include <boost/shared_ptr.hpp>

#include "ProcessNode.h"
#include "ProcessNodeTracking.h"

namespace pipeline {

boost::shared_ptr<ProcessNode>
ProcessNodeTracking::getHolder() {

	return _holder->getSelfSharedPointer();
}

} // namespace pipeline
