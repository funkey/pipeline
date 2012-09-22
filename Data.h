#ifndef DATA_H__
#define DATA_H__

#include <boost/thread/shared_mutex.hpp>

namespace pipeline {

class Data {

public:

	virtual ~Data() {}

	boost::shared_mutex& getMutex() { return _mutex; }

private:

	// a mutex to prevent concurrent access
	boost::shared_mutex _mutex;
};

} // namespace pipeline

#endif // DATA_H__

