#include "ProcessNode.h"

namespace pipeline {

bool
ProcessNode::setInput(OutputBase& output) {

	return getInput().accept(output);
}

bool
ProcessNode::setInput(unsigned int i, OutputBase& output) {

	return getInput(i).accept(output);
}

bool
ProcessNode::setInput(const std::string& name, OutputBase& output) {

	return getInput(name).accept(output);
}

bool
ProcessNode::setInput(boost::shared_ptr<Data> data) {

	return getInput().accept(data);
}

bool
ProcessNode::setInput(unsigned int i, boost::shared_ptr<Data> data) {

	return getInput(i).accept(data);
}

bool
ProcessNode::setInput(const std::string& name, boost::shared_ptr<Data> data) {

	return getInput(name).accept(data);
}

void
ProcessNode::unsetInput() {

	return getInput().unset();
}

void
ProcessNode::unsetInput(unsigned int i) {

	return getInput(i).unset();
}

void
ProcessNode::unsetInput(const std::string& name) {

	return getInput(name).unset();
}

bool
ProcessNode::setInput(const InputBase& input) {

	if (input.hasAssignedOutput())
		return getInput().accept(input.getAssignedOutput());
	else
		return getInput().accept(input.getSharedDataPointer());
}

bool
ProcessNode::setInput(unsigned int i, const InputBase& input) {

	if (input.hasAssignedOutput())
		return getInput(i).accept(input.getAssignedOutput());
	else
		return getInput(i).accept(input.getSharedDataPointer());
}

bool
ProcessNode::setInput(const std::string& name, const InputBase& input) {

	if (input.hasAssignedOutput())
		return getInput(name).accept(input.getAssignedOutput());
	else
		return getInput(name).accept(input.getSharedDataPointer());
}

bool
ProcessNode::addInput(OutputBase& output) {

	return getMultiInput().accept(output);
}

bool
ProcessNode::addInput(unsigned int i, OutputBase& output) {

	return getMultiInput(i).accept(output);
}

bool
ProcessNode::addInput(const std::string& name, OutputBase& output) {

	return getMultiInput(name).accept(output);
}

bool
ProcessNode::addInput(const InputBase& input) {

	if (input.hasAssignedOutput())
		return getMultiInput().accept(input.getAssignedOutput());
	else
		return getMultiInput().accept(input.getSharedDataPointer());

	return false;
}

bool
ProcessNode::addInput(unsigned int i, const InputBase& input) {

	if (input.hasAssignedOutput())
		return getMultiInput(i).accept(input.getAssignedOutput());
	else
		return getMultiInput(i).accept(input.getSharedDataPointer());

	return false;
}

bool
ProcessNode::addInput(const std::string& name, const InputBase& input) {

	if (input.hasAssignedOutput())
		return getMultiInput(name).accept(input.getAssignedOutput());
	else
		return getMultiInput(name).accept(input.getSharedDataPointer());

	return false;
}

void
ProcessNode::clearInputs(unsigned int i) {

	getMultiInput(i).clear();
}

void
ProcessNode::clearInputs(const std::string& name) {

	getMultiInput(name).clear();
}

OutputBase&
ProcessNode::getOutput() {

	return getOutput(0);
}

OutputBase&
ProcessNode::getOutput(unsigned int i) {

	if (_outputs.size() <= i)
		UTIL_THROW_EXCEPTION(
				NotEnoughOutputs,
				"invalid output number " << i << ", this process node has only " << _outputs.size() " outputs");

	return *_outputs[i];
}

OutputBase&
ProcessNode::getOutput(std::string name) {

	LOG_ALL(pipelinelog) << "[ProcessNode] searching for output with name " << name << std::endl;

	if (!_outputNames.count(name)) {

		UTIL_THROW_EXCEPTION(
				NoSuchOutput,
				"no such output: " << name);

	} else {

		return *_outputNames[name];
	}
}

boost::shared_ptr<ProcessNode>
ProcessNode::getSelfSharedPointer() {

	return this->shared_from_this();
}

void
ProcessNode::registerInput(InputBase& input, std::string name) {

	_inputs.push_back(&input);

	_inputNames[name] = &input;
}

void
ProcessNode::registerInputs(MultiInput& input, std::string name) {

	_multiInputs.push_back(&input);

	_multiInputNames[name] = &input;
}

void
ProcessNode::registerOutput(OutputBase& output, std::string name) {

	output.addDependency(this);

	_outputs.push_back(&output);

	_outputNames[name] = &output;
}

InputBase&
ProcessNode::getInput() {

	return getInput(0);
}

InputBase&
ProcessNode::getInput(unsigned int i) {

	if (_inputs.size() <= i)
		UTIL_THROW_EXCEPTION(
				NotEnoughInputs
				"invalid input number " << i << ", this process node has only " << _inputs.size() " inputs");

	return *_inputs[i];
}

InputBase&
ProcessNode::getInput(std::string name) {

	if (!_inputNames.count(name)) {

		UTIL_THROW_EXCEPTION(
				NoSuchInput,
				"no such input: " << name);

	} else {

		return *_inputNames[name];
	}
}

MultiInput&
ProcessNode::getMultiInput() {

	return getMultiInput(0);
}

MultiInput&
ProcessNode::getMultiInput(unsigned int i) {

	if (_multiInputs.size() <= i)
		UTIL_THROW_EXCEPTION(
				NotEnoughInputs
				"invalid multi-input number " << i << ", this process node has only " << _multiInputs.size() " multi-inputs");

	return *_multiInputs[i];
}

MultiInput&
ProcessNode::getMultiInput(std::string name) {

	if (!_multiInputNames.count(name)) {

		UTIL_THROW_EXCEPTION(
				NoSuchInput,
				"no such input: " << name);

	} else {

		return *_multiInputNames[name];
	}
}

} // namespace pipeline
