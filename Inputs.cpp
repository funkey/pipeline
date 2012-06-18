#include "Inputs.h"

namespace pipeline {

MultiInput::MultiInput() {}

MultiInput::MultiInput(std::string name) :
	InputBase(name) {}

void
MultiInput::registerBackwardSlots(signals::SlotsBase& slots) {

	_slots.push_back(&slots);
}

} // namespace pipeline
