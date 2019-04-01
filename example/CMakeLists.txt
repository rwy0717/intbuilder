add_library(example
	example.cpp
	JitHelpers.cpp
	JitHelpers.hpp
	JitTypes.cpp
	JitTypes.hpp
	BytecodeMethodBuilder.cpp
	BytecodeInterpreterBuilder.cpp
	Interpreter.cpp
)

target_include_directories(example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(example
	jitbuilder
)

add_executable(example-test
	test.cpp
)

target_link_libraries(example-test
	PRIVATE
		example
		gtest
)

add_test(example-test example-test)
