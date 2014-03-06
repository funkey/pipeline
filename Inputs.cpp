#include "Inputs.h"

namespace pipeline {

MultiInput::MultiInput() {}

void
MultiInput::registerSlots(signals::SlotsBase& slots) {

	_slots.push_back(&slots);
}

} // namespace pipeline
