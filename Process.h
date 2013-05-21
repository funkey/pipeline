#ifndef PIPELINE_PROCESS_H__
#define PIPELINE_PROCESS_H__

#include <pipeline/all.h>

namespace pipeline {

template <class Operator = ProcessNode>
class Process {

public:

	Process() : _operator(boost::make_shared<Operator>()) {}

	template <typename A1>
	Process(A1 a1) : _operator(boost::make_shared<Operator>(a1)) {}

	template <typename A1, typename A2>
	Process(A1 a1, A2 a2) : _operator(boost::make_shared<Operator>(a1, a2)) {}

	template <typename A1, typename A2, typename A3>
	Process(A1 a1, A2 a2, A3 a3) : _operator(boost::make_shared<Operator>(a1, a2, a3)) {}

	template <typename A1, typename A2, typename A3, typename A4>
	Process(A1 a1, A2 a2, A3 a3, A4 a4) : _operator(boost::make_shared<Operator>(a1, a2, a3, a4)) {}

	template <typename A1, typename A2, typename A3, typename A4, typename A5>
	Process(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) : _operator(boost::make_shared<Operator>(a1, a2, a3, a4, a5)) {}

	template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	Process(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) : _operator(boost::make_shared<Operator>(a1, a2, a3, a4, a5, a6)) {}

	Operator* operator->() {

		return _operator.get();
	}

	template <class OtherOperator>
	Process<Operator>& operator=(const Process<OtherOperator>& other) {

		_operator = other._operator;
		return *this;
	}

private:

	boost::shared_ptr<Operator> _operator;

	template <class OtherOperator>
	friend class Process;
};

} // namespace pipeline

#endif // PIPELINE_PROCESS_H__

