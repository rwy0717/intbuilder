add_executable(intbuilder_example
	example.cpp
	JitHelpers.cpp
	JitHelpers.hpp
	JitTypes.cpp
	JitTypes.hpp
)

target_link_libraries(intbuilder_example
	jitbuilder
)
