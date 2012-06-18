#ifndef PROCESS_NODE_H__
#define PROCESS_NODE_H__

#include <boost/enable_shared_from_this.hpp>

#include <exceptions.h>
#include "Input.h"
#include "Inputs.h"
#include "Output.h"
#include "Logging.h"

namespace pipeline {

class ProcessNode : public boost::enable_shared_from_this<ProcessNode> {

public:

	struct NotEnoughInputs  : virtual PipelineError, virtual SizeMismatchError {};
	struct NoSuchInput      : virtual PipelineError {};
	struct NotEnoughOutputs : virtual PipelineError, virtual SizeMismatchError {};
	struct NoSuchOutput     : virtual PipelineError {};

	/**
	 * Assign the first input of this process node to the given output. A call
	 * to this function is equivalent to setInput(0, output).
	 *
	 * @param output The output of another process node.
	 * @return true, if the input and output are compatible and the assignment
	 *         has been made.
	 */
	bool setInput(OutputBase& output);

	/**
	 * Assign the ith input of this process node to the given output.
	 *
	 * @param output The output of another process node.
	 * @return true, if the input and output are compatible and the assignment
	 *         has been made.
	 */
	bool setInput(unsigned int i, OutputBase& output);

	/**
	 * Assigns a named input of this process node to the given output.
	 *
	 * @param output The output of another process node.
	 * @return true, if the input and output are compatible and the assignment
	 *         has been made.
	 */
	bool setInput(const std::string& name, OutputBase& output);

	/**
	 * Assign the first input of this process node to the given data. A call
	 * to this function is equivalent to setInput(0, data).
	 *
	 * @param data The data object as a shared pointer.
	 * @return true, if the input and data are compatible and the assignment
	 *         has been made.
	 */
	bool setInput(boost::shared_ptr<Data> data);

	/**
	 * Assign the ith input of this process node to the given data.
	 *
	 * @param data The data object as a shared pointer.
	 * @return true, if the input and data are compatible and the assignment
	 *         has been made.
	 */
	bool setInput(unsigned int i, boost::shared_ptr<Data> data);

	/**
	 * Assigns a named input of this process node to the given data.
	 *
	 * @param data The data object as a shared pointer.
	 * @return true, if the input and data are compatible and the assignment
	 *         has been made.
	 */
	bool setInput(const std::string& name, boost::shared_ptr<Data> data);

	/**
	 * Adds an output to the first multi-input of this process node. A call to
	 * this method is equivalent to addInput(0, output).
	 *
	 * @param output The output of another process node.
	 * @return true, if the input and output are compatible and the assignment
	 *         has been made.
	 */
	bool addInput(OutputBase& output);

	/**
	 * Adds an output to the ith multi-input of this process node.
	 *
	 * @param output The output of another process node.
	 * @return true, if the input and output are compatible and the assignment
	 *         has been made.
	 */
	bool addInput(unsigned int i, OutputBase& output);

	/**
	 * Adds an output to a named multi-input of this process node.
	 *
	 * @param output The output of another process node.
	 * @return true, if the input and output are compatible and the assignment
	 *         has been made.
	 */
	bool addInput(const std::string& name, OutputBase& output);

	/**
	 * Clear all the assignments of a multi-input.
	 *
	 * @param i The number of the multi-input.
	 */
	void clearInputs(unsigned int i);

	/**
	 * Clear all the assignments of a multi-input.
	 *
	 * @param name The name of the multi-input.
	 */
	void clearInputs(const std::string& name);

	/**
	 * Get the first output of this process node. A call to this method is
	 * equivalent to getOutput(0).
	 *
	 * @return The first output of this process node.
	 */
	OutputBase& getOutput();

	/**
	 * Get the ith output of this process node.
	 *
	 * @param i The number of the output.
	 * @return The ith output of this process node.
	 */
	OutputBase& getOutput(unsigned int i);

	/**
	 * Get a named output of this process node.
	 *
	 * @param name The name of an output.
	 * @return The output of this process node with the given name.
	 */
	OutputBase& getOutput(std::string name);

	/**
	 * Get a shared pointer to this process node.
	 *
	 * @return A shared pointer to this process node.
	 */
	boost::shared_ptr<ProcessNode> getSharedPtr();

	/**
	 * Get the first input of this process node. A call to this method is
	 * equivalent to getInput(0).
	 *
	 * @return The first input of this process node.
	 */
	InputBase& getInput();

	/**
	 * Get the ith input of this process node.
	 *
	 * @param i The number of the input.
	 * @return The ith input of this process node.
	 */
	InputBase& getInput(unsigned int i);

	/**
	 * Get a named input of this process node.
	 *
	 * @param name The name of an input.
	 * @return The input of this process node with the given name.
	 */
	InputBase& getInput(std::string name);

protected:

	/**
	 * Register an input with this process node.
	 *
	 * @param input The input to register.
	 * @param name The name of the input.
	 */
	void registerInput(InputBase& input, std::string name);

	/**
	 * Register a multi-input with this process node.
	 *
	 * @param multiInput The multi-input to register.
	 * @param name The name of the input.
	 */
	void registerInputs(MultiInput& multiInput, std::string name);

	/**
	 * Register an output with this process node.
	 *
	 * @param output The output to register.
	 * @param name The name of the output.
	 */
	void registerOutput(OutputBase& output, std::string name);

private:

	MultiInput& getMultiInput();

	MultiInput& getMultiInput(unsigned int i);

	MultiInput& getMultiInput(std::string name);

	std::vector<InputBase*>  _inputs;
	std::vector<MultiInput*> _multiInputs;
	std::vector<OutputBase*> _outputs;
};

} // namespace pipeline

#endif // PROCESS_NODE_H__

