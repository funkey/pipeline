#ifndef PIPELINE_VALUE_H__
#define PIPELINE_VALUE_H__

#include <pipeline/all.h>
#include <pipeline/Process.h>

namespace pipeline {

/**
 * A class to represent the value of the output of a process node. The value 
 * will be automatically updated whenever it is used.
 *
 * Usage example:
 *
 *   Value<Image> image = imageReader->getOutput();
 *
 *   int width = image->getWidth();
 */
template <typename T>
class Value {

private:

	class UpdateValue : public pipeline::SimpleProcessNode<> {

	public:

		UpdateValue() {

			registerInput(_data, "data");
		}

		boost::shared_ptr<T> get() {

			updateInputs();

			return _data.get();
		}

	private:

		void updateOutputs() {}

		pipeline::Input<T> _data;
	};

public:

	/**
	 * Default constructur. Creates an unassigned pipeline value.
	 */
	Value() {}

	/**
	 * Copy constructor. Creates a pipeline value that points to the same data 
	 * as the other value.
	 *
	 * @param other
	 *             Another pipeline value whos data can be cast to this value's 
	 *             data.
	 */
	template <typename S>
	Value(Value<S>& other) {

		_updateValue->setInput(other._updateValue->getInput().getAssignedOutput());
	}

	/**
	 * Creates a pipeline value from the output of a process node.
	 *
	 * Usage example:
	 *
	 *   Value<Image> image = imageReader->getOutput();
	 *
	 *   int width = image->getWidth();
	 *
	 * @param output The output of a process node.
	 */
	Value(pipeline::OutputBase& output) {

		_updateValue->setInput(output);
	}

	/**
	 * Assigns the data of another pipeline value to this pipeline value.
	 *
	 * @param other
	 *             Another pipeline value whos data can be cast to this value's 
	 *             data.
	 */
	template <typename S>
	Value<T>& operator=(Value<S>& other) {

		_updateValue->setInput(other._updateValue->getInput().getAssignedOutput());
	}

	/**
	 * Assigns the output of a process node to this pipeline value.
	 *
	 * @param output The output of a process node.
	 */
	Value<T>& operator=(pipeline::OutputBase& output) {

		_updateValue->setInput(output);
	}

	/**
	 * Sets the data of this pipeline value to a fixed value.
	 *
	 * @param data The data to set this value to.
	 */
	Value<T>& operator=(const T& data) {

		_updateValue->setInput(boost::make_shared<T>(data));
	}

	/**
	 * Dereferencation.
	 *
	 * @return A reference to the stored data.
	 */
	T& operator*() {

		return *get();
	}

	/**
	 * Transparent dereferencation on the -> operator.
	 */
	T* operator->() {

		return get().operator->();
	}

	/**
	 * Conversion operator to boost::shared_ptr<T>.
	 *
	 * @return A boost::shared_ptr to the stored data.
	 */
	operator boost::shared_ptr<T>() {

		return get();
	}

private:

	// ProcessNode is allowed to access a value's shared pointer
	friend class ProcessNode;

	/**
	 * Get a boost::shared_ptr to the data stored by this pipeline value.
	 */
	boost::shared_ptr<T> get() {

		return _updateValue->get();
	}

	Process<UpdateValue> _updateValue;
};

} // namespace pipeline

#endif // PIPELINE_VALUE_H__

