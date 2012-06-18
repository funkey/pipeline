#ifndef PIPELINE_SIGNALS_UPDATED_H__
#define PIPELINE_SIGNALS_UPDATED_H__

#include "PipelineSignal.h"

namespace pipeline {

/**
 * Forward signal. States that an output has been updated as a reaction to an
 * Update signal.
 */
class Updated : public PipelineSignal { public: Updated() {} };

} // namespace pipeline


#endif // PIPELINE_SIGNALS_UPDATED_H__

