#ifndef PIPELINE_PROCESS_H__
#define PIPELINE_PROCESS_H__

#include <pipeline/all.h>

namespace pipeline {

template <class Operator>
class Process {

public:

	Process() : _operator(boost::make_shared<Operator>()) {}

	template <typename A1>
	Process(A1 a1) : _operator(boost::make_shared<Operator>(a1)) {}

	template <typename A1, typename A2>
	Process(A1 a1, A2 a2) : _operator(boost::make_shared<Operator>(a1, a2)) {}

	Operator* operator->() {

		return _operator.get();
	}

private:

	boost::shared_ptr<Operator> _operator;
};

} // namespace pipeline

#endif // PIPELINE_PROCESS_H__

