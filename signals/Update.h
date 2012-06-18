#ifndef PIPELINE_SIGNALS_UPDATE_H__
#define PIPELINE_SIGNALS_UPDATE_H__

#include "PipelineSignal.h"

namespace pipeline {

/**
 * Backward signal. Requests update of input objects. Should be send prior to
 * use of input objects, when a Modified signal has been received earlier.
 */
class Update   : public PipelineSignal { public: Update() {} };

} // namespace pipeline

#endif // PIPELINE_SIGNALS_UPDATE_H__

