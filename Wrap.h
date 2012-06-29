#ifndef PIPELINE_WRAP_H__
#define PIPELINE_WRAP_H__

#include "Data.h"

namespace pipeline {

template <typename T>
class Wrap : public Data {

public:

	Wrap() {}

	Wrap(const T& value) :
		_value(value) {}

	T& get() {

		return _value;
	}

private:

	T _value;
};

} // namespace pipeline

#endif // PIPELINE_WRAP_H__

