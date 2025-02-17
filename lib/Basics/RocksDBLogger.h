////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2024 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Business Source License 1.1 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     https://github.com/arangodb/arangodb/blob/devel/LICENSE
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdarg.h>
#include <atomic>

#include <rocksdb/env.h>

namespace arangodb {

class RocksDBLogger final : public rocksdb::Logger {
 public:
  explicit RocksDBLogger(rocksdb::InfoLogLevel level);
  ~RocksDBLogger();

  void disable() { _enabled = false; }
  void enable() { _enabled = true; }

  // intentionally do not log header information here
  // as this does not seem to honor the log level correctly
  void LogHeader(const char* format, va_list ap) override {}

  void Logv(char const* format, va_list ap) override;
  void Logv(const rocksdb::InfoLogLevel, char const* format,
            va_list ap) override;

  // nothing to do here, as ArangoDB logger infrastructure takes care of
  // flushing itself
  void Flush() override {}

 private:
  std::atomic<bool> _enabled;
};

}  // namespace arangodb
