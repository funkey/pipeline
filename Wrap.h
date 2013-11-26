#ifndef PIPELINE_WRAP_H__
#define PIPELINE_WRAP_H__

#include <boost/make_shared.hpp>

#include "Data.h"

namespace pipeline {

template <typename T>
class Wrap : public Data {

public:

	Wrap(boost::shared_ptr<T> value = boost::make_shared<T>()) :
		_value(value) {}

	T& get() {

		return *_value;
	}

	boost::shared_ptr<T> get_shared() {

		return _value;
	}

	operator T&() {

		return *_value;
	}

private:

	boost::shared_ptr<T> _value;
};

} // namespace pipeline

#endif // PIPELINE_WRAP_H__

