#ifndef DATA_H__
#define DATA_H__

#include <boost/thread/shared_mutex.hpp>

namespace pipeline {

class Data {

public:

	// default constructor
	Data() {}

	// overwrite default copy constructor
	Data(const Data&) {}

	// overwrite default assignment operator
	Data& operator=(const Data&) { return *this; }

	virtual ~Data() {}

	boost::shared_mutex& getMutex() { return _mutex; }

private:

	// a mutex to prevent concurrent access
	boost::shared_mutex _mutex;
};

} // namespace pipeline

#endif // DATA_H__

