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

bool
ProcessNode::setInput(InputBase& input) {

	if (input.hasAssignedOutput())
		return getInput().accept(input.getAssignedOutput());
	else
		return getInput().accept(input.getAssignedSharedPtr());
}

bool
ProcessNode::setInput(unsigned int i, InputBase& input) {

	if (input.hasAssignedOutput())
		return getInput(i).accept(input.getAssignedOutput());
	else
		return getInput(i).accept(input.getAssignedSharedPtr());
}

bool
ProcessNode::setInput(const std::string& name, InputBase& input) {

	if (input.hasAssignedOutput())
		return getInput(name).accept(input.getAssignedOutput());
	else
		return getInput(name).accept(input.getAssignedSharedPtr());
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
		BOOST_THROW_EXCEPTION(
				NotEnoughOutputs() << error_message("not enough outputs")
				                   << mismatch_size1(_outputs.size())
				                   << mismatch_size2(i)
				                   << STACK_TRACE);

	return *_outputs[i];
}

OutputBase&
ProcessNode::getOutput(std::string name) {

	LOG_ALL(pipelinelog) << "[ProcessNode] searching for output with name " << name << std::endl;

	for (unsigned int i = 0; i < _outputs.size(); i++)
		if (_outputs[i]->getName() == name)
			return *_outputs[i];

	BOOST_THROW_EXCEPTION(
			NoSuchOutput() << error_message("no such output: " + name)
				           << STACK_TRACE);
}

boost::shared_ptr<ProcessNode>
ProcessNode::getSharedPtr() {

	return this->shared_from_this();
}

void
ProcessNode::registerInput(InputBase& input, std::string name) {

	input.setName(name);

	_inputs.push_back(&input);
}

void
ProcessNode::registerInputs(MultiInput& input, std::string name) {

	input.setName(name);

	_multiInputs.push_back(&input);
}

void
ProcessNode::registerOutput(OutputBase& output, std::string name) {

	output.setProcessNode(this);
	output.setName(name);

	_outputs.push_back(&output);
}

InputBase&
ProcessNode::getInput() {

	return getInput(0);
}

InputBase&
ProcessNode::getInput(unsigned int i) {

	if (_inputs.size() <= i)
		BOOST_THROW_EXCEPTION(
				NotEnoughInputs() << error_message("not enough inputs")
				                  << mismatch_size1(_inputs.size())
				                  << mismatch_size2(i)
				                  << STACK_TRACE);

	return *_inputs[i];
}

InputBase&
ProcessNode::getInput(std::string name) {

	for (unsigned int i = 0; i < _inputs.size(); i++)
		if (_inputs[i]->getName() == name)
			return *_inputs[i];

	BOOST_THROW_EXCEPTION(
			NoSuchInput() << error_message("no such input: " + name)
			              << STACK_TRACE);
}

MultiInput&
ProcessNode::getMultiInput() {

	return getMultiInput(0);
}

MultiInput&
ProcessNode::getMultiInput(unsigned int i) {

	if (_multiInputs.size() <= i)
		BOOST_THROW_EXCEPTION(
				NotEnoughInputs() << error_message("not enough inputs")
				                  << mismatch_size1(_inputs.size())
				                  << mismatch_size2(i)
				                  << STACK_TRACE);

	return *_multiInputs[i];
}

MultiInput&
ProcessNode::getMultiInput(std::string name) {

	for (unsigned int i = 0; i < _multiInputs.size(); i++)
		if (_multiInputs[i]->getName() == name)
			return *_multiInputs[i];

	BOOST_THROW_EXCEPTION(
			NoSuchInput() << error_message("no such input: " + name)
			              << STACK_TRACE);
}

} // namespace pipeline
