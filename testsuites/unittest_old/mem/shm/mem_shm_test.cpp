/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <climits>
#include <gtest/gtest.h>

#include "it_test_shm.h"

using namespace testing::ext;
namespace OHOS {
class MemShmTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

#if defined(LOSCFG_USER_TEST_FULL)
/* *
 * @tc.name: it_test_shm_001
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm001, TestSize.Level0)
{
    ItTestShm001();
}

/* *
 * @tc.name: it_test_shm_002
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm002, TestSize.Level0)
{
    ItTestShm002();
}

/* *
 * @tc.name: it_test_shm_003
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm003, TestSize.Level0)
{
    ItTestShm003();
}

/* *
 * @tc.name: it_test_shm_004
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm004, TestSize.Level0)
{
    ItTestShm004();
}

/* *
 * @tc.name: it_test_shm_005
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm005, TestSize.Level0)
{
    ItTestShm005();
}

/* *
 * @tc.name: it_test_shm_006
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm006, TestSize.Level0)
{
    ItTestShm006();
}

/* *
 * @tc.name: it_test_shm_007
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm007, TestSize.Level0)
{
    ItTestShm007();
}

/* *
 * @tc.name: it_test_shm_008
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm008, TestSize.Level0)
{
    ItTestShm008();
}

/* *
 * @tc.name: it_test_shm_009
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm009, TestSize.Level0)
{
    ItTestShm009();
}

/* *
 * @tc.name: it_test_shm_010
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm010, TestSize.Level0)
{
    ItTestShm010();
}
#endif

#if defined(LOSCFG_USER_TEST_SMOKE)
/* *
 * @tc.name: it_test_shm_011
 * @tc.desc: function for MemShmTest
 * @tc.type: FUNC
 * @tc.require: AR000EEMQ9
 */
HWTEST_F(MemShmTest, ItTestShm011, TestSize.Level0)
{
    ItTestShm011();
}
#endif
}
// namespace OHOS
