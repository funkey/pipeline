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

	template <typename S>
	Value(Value<S>& other) {

		_updateValue->setInput(other._updateValue->getInput().getAssignedOutput());
	}

	Value(pipeline::OutputBase& output) {

		_updateValue->setInput(output);
	}

	Value<T>& operator=(Value<T>& other) {

		_updateValue->setInput(other._updateValue->getInput().getAssignedOutput());
	}

	Value<T>& operator=(pipeline::OutputBase& output) {

		_updateValue->setInput(output);
	}

	operator boost::shared_ptr<T>() {

		return get();
	}

	T& operator*() {

		return *get();
	}

	T* operator->() {

		return get().operator->();
	}

	boost::shared_ptr<T> get() {

		return _updateValue->get();
	}

private:

	Value() {}

	Process<UpdateValue> _updateValue;
};

} // namespace pipeline

#endif // PIPELINE_VALUE_H__

