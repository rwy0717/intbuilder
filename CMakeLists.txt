cmake_minimum_required(VERSION 3.2)

list(APPEND CMAKE_MODULE_PATH
	${CMAKE_CURRENT_LIST_DIR}/omr/cmake/modules/
)

project(intbuilder
	LANGUAGES C CXX
	VERSION 1
)

include(OmrPlatform)

omr_platform_global_setup()

# OMR Configuration

set(OMR_COMPILER   ON  CACHE INTERNAL "Enable the Compiler. Needed for JitBuilder")
set(OMR_DDR        OFF CACHE INTERNAL "Disable DDR.")
set(OMR_EXAMPLE    ON  CACHE INTERNAL "HACK HACK HACK ")
set(OMR_FVTEST     OFF CACHE INTERNAL "Disable OMR's internal test suite, it's incompatible with b9")
set(OMR_GC         OFF CACHE INTERNAL "Disable the GC")
set(OMR_JITBUILDER ON  CACHE INTERNAL "Enable JITBuilder")
set(OMR_OM         OFF CACHE INTERNAL "Disable OMR Om, a GC object model")
set(OMR_OMRSIG     OFF CACHE INTERNAL "Disable the OMR signal compatibility library")
set(OMR_PORT       OFF CACHE INTERNAL "Disable the port library")
set(OMR_THREAD     OFF CACHE INTERNAL "Disable thread library")

set(OMR_WARNINGS_AS_ERRORS OFF CACHE INTERNAL "OMR doesn't compile cleanly on my laptop :p")

add_subdirectory(omr) # must be first
add_subdirectory(intbuilder)
add_subdirectory(example)

