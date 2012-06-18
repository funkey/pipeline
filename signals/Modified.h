#ifndef PIPELINE_SIGNALS_MODIFIED_H__
#define PIPELINE_SIGNALS_MODIFIED_H__

#include "PipelineSignal.h"

namespace pipeline {

/**
 * Forward signal. Indicates that an output object has been modified.
 */
class Modified : public PipelineSignal { public: Modified() {} };

} // namespace pipeline

#endif // PIPELINE_SIGNALS_MODIFIED_H__

