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
class ValueImpl {

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
	ValueImpl() {}

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

protected:

	Process<UpdateValue>& getUpdateProcessNode() { return _updateValue; }

	// ProcessNode is allowed to access a value's shared pointer
	friend class ProcessNode;

	/**
	 * Set the content of this value to the static content of a shared pointer.
	 */
	void set(boost::shared_ptr<T> value) {

		_updateValue->setInput(value);
	}

	/**
	 * Get a boost::shared_ptr to the data stored by this pipeline value.
	 */
	boost::shared_ptr<T> get() {

		return _updateValue->get();
	}

private:

	Process<UpdateValue> _updateValue;
};

template <bool, typename T>
class ValueTypeDispatch {};

template <typename T>
class ValueTypeDispatch<true, T> : public ValueImpl<T> {};

template <typename T>
class ValueTypeDispatch<false, T> : public ValueImpl<Wrap<T> > {

private:

	typedef ValueImpl<Wrap<T> > parent_type;

public:

	// transparent unwrapper
	T& operator*() {

		return ValueImpl<Wrap<T> >::get()->get();
	}

	// transparent -> operator
	T* operator->() const {

		return &(ValueImpl<Wrap<T> >::get()->get());
	}

	void set(boost::shared_ptr<T> p) {

		parent_type::set(boost::make_shared<Wrap<T> >(p));
	}
};

template <typename T>
class Value : public ValueTypeDispatch<boost::is_base_of<Data, T>::value, T> {

	typedef ValueTypeDispatch<boost::is_base_of<Data, T>::value, T> parent_type;

private:

	using parent_type::getUpdateProcessNode;

public:

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

		getUpdateProcessNode()->setInput(other.getUpdateProcessNode()->getInput());
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

		getUpdateProcessNode()->setInput(output);
	}

	/**
	 * Creates a pipeline value from the input of a process node.
	 *
	 * Usage example:
	 *
	 *   Value<Image> inputImage = watershed->getInput("image");
	 *
	 *   int width = inputImage->getWidth();
	 */
	Value(const pipeline::Input<T>& input) {

		getUpdateProcessNode()->setInput(input);
	}

	/**
	 * Create a pipeline value from an existing object.
	 */
	Value(const T& value) {

		parent_type::set(boost::make_shared<T>(value));
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

		getUpdateProcessNode()->setInput(other.getUpdateProcessNode()->getInput());
	}

	/**
	 * Assigns the output of a process node to this pipeline value.
	 *
	 * @param output The output of a process node.
	 */
	Value<T>& operator=(pipeline::OutputBase& output) {

		getUpdateProcessNode()->setInput(output);
	}

	/**
	 * Sets the data of this pipeline value to a fixed value.
	 *
	 * @param data The data to set this value to.
	 */
	Value<T>& operator=(const T& data) {

		set(boost::make_shared<T>(data));
	}

	/**
	 * Implicit conversion to a pipeline input, so that Value can also be used 
	 * in an input:
	 *
	 *   pipeline::Value<int> x(5);
	 *   processNode->setInput("a value", x);
	 */
	operator InputBase&() {

		return getUpdateProcessNode()->getInput();
	}
};

} // namespace pipeline

#endif // PIPELINE_VALUE_H__

