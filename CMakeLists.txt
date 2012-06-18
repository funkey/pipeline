# find all files constituting this library
file(GLOB pipelinefiles
  *.cpp)

# this is a library
add_library(pipeline ${pipelinefiles})
