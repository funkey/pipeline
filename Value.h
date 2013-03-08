#ifndef PIPELINE_VALUE_H__
#define PIPELINE_VALUE_H__

#include <pipeline/all.h>

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

	class ValueUpdater : public pipeline::SimpleProcessNode<> {

	public:

		ValueUpdater() {

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

		createUpdater();

		_updater->setInput(other._updater->getInput().getAssignedOutput());
	}

	Value(pipeline::OutputBase& output) {

		createUpdater();

		_updater->setInput(output);
	}

	Value<T>& operator=(Value<T>& other) {

		createUpdater();

		_updater->setInput(other._updater->getInput().getAssignedOutput());
	}

	Value<T>& operator=(pipeline::OutputBase& output) {

		createUpdater();

		_updater->setInput(output);
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

		return _updater->get();
	}

private:

	Value() {}

	void createUpdater() {

		_updater = boost::make_shared<ValueUpdater>();
	}

	boost::shared_ptr<ValueUpdater> _updater;
};

} // namespace pipeline

#endif // PIPELINE_VALUE_H__

