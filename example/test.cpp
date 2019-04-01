#include <Interpreter.hpp>

#include <OMR/ByteBuffer.hpp>
#include <cstdint>
#include <inttypes.h>
#include <gtest/gtest.h>
#include <memory>
#include <JitBuilder.hpp>

template <typename T>
T* release(OMR::ByteBuffer& buffer) {
	return reinterpret_cast<T*>(buffer.release());
}

template <typename T>
std::unique_ptr<T> release_to_unique(OMR::ByteBuffer& buffer) {
	return std::unique_ptr<T>(release<T>(buffer));
}

inline std::unique_ptr<Func> release_func(OMR::ByteBuffer& buffer) {
	return release_to_unique<Func>(buffer);
}

enum class RunMode { INT, JIT };

std::string to_string(RunMode mode) {
	switch (mode) {
	case RunMode::INT:
		return std::string("int");
	case RunMode::JIT:
		return std::string("jit");
	default:
		return std::string("xxx");
	}
}

std::ostream& operator<<(std::ostream& out, RunMode mode) {
	return out << to_string(mode);
}

struct RunTest : public testing::TestWithParam<RunMode> {
protected:
	virtual void SetUp() override {}

	virtual void TearDown() override {}

	void print_debug(Interpreter& interpreter, Func* target) {
		fprintf(stderr, "int main: interpreter=%p\n", &interpreter);
		fprintf(stderr, "int main: target=%p\n", target);
		fprintf(stderr, "int main: target startpc=%p\n", &target->body[0]);
		fprintf(stderr, "int main: initial op=%hhu\n", target->body[0]);
		fprintf(stderr, "int main: initial sp=%p\n", interpreter.sp());
	}

	void run_jit(Interpreter& interpreter, Func* target) {
		print_debug(interpreter, target);
		ASSERT_EQ(target->cbody, nullptr);
		interpreter.compile(target);
		ASSERT_NE(target->cbody, nullptr);
		interpreter.run_cbody(target);
	}

	void run_int(Interpreter& interpreter, Func* target) {
		print_debug(interpreter, target);
		interpreter.interpret_body(target);
	}

	void run(Interpreter& interpreter, Func* target) {
		switch(GetParam()) {
		case RunMode::JIT:
			run_jit(interpreter, target);
			break;
		case RunMode::INT:
			run_int(interpreter, target);
			break;
		default:
			FAIL() << "Unknown Run Mode";
		}
	}

	void run(Func* func) {
		Interpreter interpreter;
		run(interpreter, func);
	}
};

std::string runmode_param_to_string(const testing::TestParamInfo<RunMode>& info) {
	return to_string(info.param);
}

TEST_P(RunTest, HaltOnly) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::HALT;
	run(release_func(buffer).get());
}

TEST_P(RunTest, NopThenHalt) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::NOP;
	buffer << Op::HALT;
	run(release_func(buffer).get());
}

TEST_P(RunTest, NopTwiceThenHalt) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::NOP;
	buffer << Op::NOP;
	buffer << Op::HALT;
	run(release_func(buffer).get());
}

TEST_P(RunTest, PushContThenHalt) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(42);
	buffer << Op::HALT;

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(), 42);
}

TEST_P(RunTest, PushTwoConsts) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::HALT;

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(0), 333);
	EXPECT_EQ(interp.peek(1), 444);
}

TEST_P(RunTest, PushThreeConsts) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::PUSH_CONST << std::int64_t(555);
	buffer << Op::HALT;

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(0), 333);
	EXPECT_EQ(interp.peek(1), 444);
	EXPECT_EQ(interp.peek(2), 555);
}

TEST_P(RunTest, AddTwoConsts) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::ADD;
	buffer << Op::HALT;

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(), 777);
}

TEST_P(RunTest, AddThreeConsts) {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::ADD;
	buffer << Op::PUSH_CONST << std::int64_t(222);
	buffer << Op::ADD;
	buffer << Op::HALT;

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(), 999);
}

TEST_P(RunTest, PopLocalThenPushLocal) {
	OMR::ByteBuffer buffer;
	buffer << Func(1, 0); // one local
	buffer << Op::PUSH_CONST << std::int64_t(999);
	buffer << Op::PUSH_CONST << std::int64_t(123);
	buffer << Op::POP_LOCAL  << std::int64_t(0);
	buffer << Op::PUSH_LOCAL << std::int64_t(0);
	buffer << Op::HALT;

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(0), 123); // local[0]
	EXPECT_EQ(interp.peek(1), 999); // stack[-1]
	EXPECT_EQ(interp.peek(2), 123); // stack[0]
}

TEST_P(RunTest, BranchIfTrue) {
	OMR::ByteBuffer buffer;
	buffer << Func(0, 0);
	buffer << Op::PUSH_CONST << std::int64_t(1);        // 00 + 1 + 8
	buffer << Op::BRANCH_IF  << std::int64_t(12);       // 09 + 1 + 8
	buffer << Op::PUSH_CONST << std::int64_t(7);        // 18 + 1 + 8
	buffer << Op::HALT;                                 // 27 + 1
	buffer << Op::HALT;                                 // 28 + 1
	buffer << Op::HALT;                                 // 29 + 1
	buffer << Op::PUSH_CONST << std::int64_t(8);        // 30 + 1 + 8
	buffer << Op::HALT;                                 // 39 + 1

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(0), 8);
}

TEST_P(RunTest, BranchIfFalse) {
	OMR::ByteBuffer buffer;
	buffer << Func(0, 0);
	buffer << Op::PUSH_CONST << std::int64_t(0);        // 00 + 1 + 8
	buffer << Op::BRANCH_IF  << std::int64_t(12);       // 09 + 1 + 8
	buffer << Op::PUSH_CONST << std::int64_t(7);        // 18 + 1 + 8
	buffer << Op::HALT;                                 // 27 + 1
	buffer << Op::HALT;                                 // 28 + 1
	buffer << Op::HALT;                                 // 29 + 1
	buffer << Op::PUSH_CONST << std::int64_t(8);        // 30 + 1 + 8
	buffer << Op::HALT;                                 // 39 + 1

	Interpreter interp;
	run(interp, release_func(buffer).get());
	EXPECT_EQ(interp.peek(0), 7);
}

INSTANTIATE_TEST_SUITE_P(
	IntAndJit,
	RunTest,
	::testing::Values(RunMode::INT, RunMode::JIT),
	runmode_param_to_string
);

extern "C" int main(int argc, char** argv) {
	initializeJit();
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
