/*
 * Copyright 2013 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: jmarantz@google.com (Joshua Marantz)
//
// Tests the overhead of the CompressedCache adapter, using 1k/1M insert sizes,
// with two different levels of entropy.  For high entropy we use a big block
// of randomly generated bytes.  For low entropy we use a smaller block of
// randomly generated bytes, concatenated together to form the total size
// we want.
//
//
// Benchmark                  Time(ns)    CPU(ns) Iterations
// ---------------------------------------------------------
// BM_Compress1MHighEntropy   38755898   38600000        100
// BM_Compress1KHighEntropy      62425      63000      10000
// BM_Compress1MLowEntropy     7175143    7100000        100
// BM_Compress1KLowEntropy       16620      16514      41176
//
// Disclaimer: comparing runs over time and across different machines
// can be misleading.  When contemplating an algorithm change, always do
// interleaved runs with the old & new algorithm.

#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/benchmark.h"
#include "pagespeed/kernel/base/cache_interface.h"
#include "pagespeed/kernel/base/null_mutex.h"
#include "pagespeed/kernel/base/scoped_ptr.h"
#include "pagespeed/kernel/base/shared_string.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/thread_system.h"
#include "pagespeed/kernel/cache/compressed_cache.h"
#include "pagespeed/kernel/cache/lru_cache.h"
#include "pagespeed/kernel/util/platform.h"
#include "pagespeed/kernel/util/simple_random.h"
#include "pagespeed/kernel/util/simple_stats.h"

namespace {

class EmptyCallback : public net_instaweb::CacheInterface::Callback {
 public:
  EmptyCallback() {}
  virtual ~EmptyCallback() {}
  virtual void Done(net_instaweb::CacheInterface::KeyState state) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(EmptyCallback);
};

void TestCachePayload(int payload_size, int chunk_size, int iters) {
  GoogleString value;
  net_instaweb::SimpleRandom random(new net_instaweb::NullMutex);
  GoogleString chunk = random.GenerateHighEntropyString(chunk_size);
  while (static_cast<int>(value.size()) < payload_size) {
    value += chunk;
  }
  net_instaweb::scoped_ptr<net_instaweb::ThreadSystem> thread_system(
      net_instaweb::Platform::CreateThreadSystem());
  net_instaweb::SimpleStats stats(thread_system.get());
  net_instaweb::CompressedCache::InitStats(&stats);
  net_instaweb::LRUCache* lru_cache =
      new net_instaweb::LRUCache(value.size() * 2);
  net_instaweb::CompressedCache compressed_cache(lru_cache, &stats);
  EmptyCallback empty_callback;
  net_instaweb::SharedString str(value);
  for (int i = 0; i < iters; ++i) {
    compressed_cache.Put("key", str);
    compressed_cache.Get("key", &empty_callback);
  }
}

static void BM_Compress1MHighEntropy(int iters) {
  TestCachePayload(1000*1000, 1000*1000, iters);
}

static void BM_Compress1KHighEntropy(int iters) {
  TestCachePayload(1000, 1000, iters);
}

static void BM_Compress1MLowEntropy(int iters) {
  TestCachePayload(1000*1000, 1000, iters);
}

static void BM_Compress1KLowEntropy(int iters) {
  TestCachePayload(1000, 50, iters);
}

}  // namespace

BENCHMARK(BM_Compress1MHighEntropy);
BENCHMARK(BM_Compress1KHighEntropy);
BENCHMARK(BM_Compress1MLowEntropy);
BENCHMARK(BM_Compress1KLowEntropy);
