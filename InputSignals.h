#ifndef PIPELINE_INPUT_SIGNALS_H__
#define PIPELINE_INPUT_SIGNALS_H__

#include <boost/shared_ptr.hpp>

#include "signals/Modified.h"
#include "Wrap.h"

namespace pipeline {

/**
 * Base for the specialized InputSet classes. Use this signal, if you are not
 * interested in the type of the input.
 */
class InputSetBase : public Modified { public: InputSetBase() {}; };

/**
 * Input set notification. Internally elicited, whenever one of the
 * setInput(...) methods of a ProcessNode has been called.
 */
template <typename DataType>
class InputSet : public InputSetBase {

public:

	InputSet() {};

	InputSet(boost::shared_ptr<DataType> data) :
		_data(data) {}

	boost::shared_ptr<DataType> getData() {

		return _data;
	}

	const boost::shared_ptr<DataType> getData() const {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

/**
 * Base for the specialized InputSetToSharedPointer classes. Use this signal, if 
 * you are not interested in the type of the input.

 */
class InputSetToSharedPointerBase : public InputSetBase {};

/**
 * Input set notification for shared pointer inputs. Internally elicited,
 * whenever one of the setInput(...) methods of a ProcessNode has been called
 * with a shared pointer.
 */
template <typename DataType>
class InputSetToSharedPointer : public InputSetToSharedPointerBase {

public:

	InputSetToSharedPointer() {};

	InputSetToSharedPointer(boost::shared_ptr<DataType> data) :
		_data(data) {}

	boost::shared_ptr<DataType> getData() {

		return _data;
	}

	const boost::shared_ptr<DataType> getData() const {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

/**
 * Base for the specialized InputUnset classes. Use this signal, if you are not
 * interested in the type of the input.
 */
class InputUnsetBase : public PipelineSignal {};

/**
 * Input unset notification. Internally elicited, whenever one of the
 * unsetInput(...) methods of a ProcessNode has been called.
 */
template <typename DataType>
class InputUnset : public InputUnsetBase {

public:

	InputUnset() {};

	InputUnset(boost::shared_ptr<DataType> data) :
		_data(data) {}

	boost::shared_ptr<DataType> getData() {

		return _data;
	}

	const boost::shared_ptr<DataType> getData() const {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

/**
 * Base for the specialized InputAdded classes. Use this signal, if you are not
 * interested in the type of the input.
 */
class InputAddedBase : public PipelineSignal {};

/**
 * Input add notification. Internally eliceted, whenever one of the
 * addInput(...) methods of a ProcessNode has been called.
 */
template <typename DataType>
class InputAdded : public InputAddedBase {

public:

	InputAdded() {};

	InputAdded(boost::shared_ptr<DataType> data) :
		_data(data) {}

	boost::shared_ptr<DataType> getData() {

		return _data;
	}

	const boost::shared_ptr<DataType> getData() const {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

/**
 * Base for the specialized InputSetToSharedPointer classes. Use this signal, if 
 * you are not interested in the type of the input.
 */
class InputAddedToSharedPointerBase : public InputAddedBase {};

/**
 * Input add notification. Internally eliceted, whenever one of the
 * addInput(...) methods of a ProcessNode has been called with a shared
 * pointer.
 */
template <typename DataType>
class InputAddedToSharedPointer : public InputAddedToSharedPointerBase {

public:

	InputAddedToSharedPointer() {};

	InputAddedToSharedPointer(boost::shared_ptr<DataType> data) :
		_data(data) {}

	boost::shared_ptr<DataType> getData() {

		return _data;
	}

	const boost::shared_ptr<DataType> getData() const {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

/**
 * Base for the specialized InputRemoved classes. Use this signal, if you are not
 * interested in the type of the input.
 */
class InputRemovedBase : public PipelineSignal {};

/**
 * Input remove notification. Internally eliceted, whenever one of the
 * removeInput(...) methods of a ProcessNode has been called.
 */
template <typename DataType>
class InputRemoved : public InputRemovedBase {

public:

	InputRemoved() {};

	InputRemoved(boost::shared_ptr<DataType> data) :
		_data(data) {}

	boost::shared_ptr<DataType> getData() {

		return _data;
	}

	const boost::shared_ptr<DataType> getData() const {

		return _data;
	}

private:

	boost::shared_ptr<DataType> _data;
};

/**
 * Multi-input clear notification. Internally eliceted, whenever one of the 
 * clearInputs(...) methods of a ProcessNode has been called.
 */
class InputsCleared : public InputRemovedBase { public: InputsCleared() {}; };

} // namespace pipeline

#endif // PIPELINE_INPUT_SIGNALS_H__

