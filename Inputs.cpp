#include "Inputs.h"

namespace pipeline {

MultiInput::MultiInput() {}

void
MultiInput::registerBackwardSlots(signals::SlotsBase& slots) {

	_slots.push_back(&slots);
}

} // namespace pipeline
