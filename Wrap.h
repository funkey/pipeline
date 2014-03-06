#ifndef PIPELINE_WRAP_H__
#define PIPELINE_WRAP_H__

#include <boost/make_shared.hpp>

#include "Data.h"

namespace pipeline {

/**
 * Wraps an arbitrary type, such that the result is derived from Data.
 */
template <typename T>
class Wrap : public Data {

public:

	Wrap() {};

	Wrap(boost::shared_ptr<T> value) :
		_value(value) {}

	T* get() {

		return _value.get();
	}

	boost::shared_ptr<T> getSharedPointer() {

		return _value;
	}

private:

	boost::shared_ptr<T> _value;
};

} // namespace pipeline

#endif // PIPELINE_WRAP_H__

