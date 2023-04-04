// SPDX-License-Identifier: MIT
//
// Copyright 2023 alturmann1729
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gtest/gtest.h>

#include "core/core.h"

template <typename Klass, typename MemberFunc>
class MemberFuncWrapper {
 public:
  template <typename... Args>
  auto operator()(Args &&...argpack) noexcept {
    return (this_->*func_)(argpack...);
  }

 private:
  MemberFunc func_;
  Klass *this_;
};

class SAMEThingCoreBasicTest : public ::testing::Test {
 protected:
  void SampleGeneratedHandleStub(float) noexcept { ; }

  samething::SAMEGenerator<MemberFuncWrapper<
      SAMEThingCoreBasicTest,
      decltype(&SAMEThingCoreBasicTest::SampleGeneratedHandleStub)>>
      same_generator_;
};

TEST_F(SAMEThingCoreBasicTest, EnsuresAttentionSignalRangeIsValid) {
  // EXPECT_EQ(same_generator_.kAttentionSignalMin, 8);
  // EXPECT_EQ(same_generator_.kAttentionSignalMax, 25);
}

TEST_F(SAMEThingCoreBasicTest, RejectsInvalidAttentionSignalDuration) {
  EXPECT_FALSE(same_generator_.AttentionSignalDurationSet(
      same_generator_.kAttentionSignalMax + 1));
  EXPECT_FALSE(same_generator_.AttentionSignalDurationSet(
      same_generator_.kAttentionSignalMin - 1));

  EXPECT_FALSE(same_generator_.AttentionSignalDurationSet(-1));
}

TEST_F(SAMEThingCoreBasicTest, AcceptsValidAttentionSignalDuration) {
  for (int duration = same_generator_.kAttentionSignalMin;
       duration != same_generator_.kAttentionSignalMax; ++duration) {
    EXPECT_TRUE(same_generator_.AttentionSignalDurationSet(duration));
  }
}

TEST_F(SAMEThingCoreBasicTest, RejectsInvalidValidTimePeriod) {}
TEST_F(SAMEThingCoreBasicTest, AcceptsValidTimePeriod) {}
TEST_F(SAMEThingCoreBasicTest, EnsuresOriginatorCodesMapToProperNames) {}
TEST_F(SAMEThingCoreBasicTest, EnsuresEventCodesMapToProperNames) {}
TEST_F(SAMEThingCoreBasicTest, EnsuresStateCodesMapToProperNames) {}
