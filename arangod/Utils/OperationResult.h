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

#include "Basics/debugging.h"
#include "Basics/Result.h"
#include "Utils/OperationOptions.h"

#include <velocypack/Buffer.h>
#include <velocypack/Options.h>
#include <velocypack/Slice.h>

#include <unordered_map>

namespace arangodb {

struct OperationResult final {
  // create from Result
  explicit OperationResult(Result const& other, OperationOptions options)
      : result(other), options(std::move(options)) {}
  explicit OperationResult(Result&& other, OperationOptions options)
      : result(std::move(other)), options(std::move(options)) {}

  // copy
  OperationResult(OperationResult const& other) = delete;
  OperationResult& operator=(OperationResult const& other) = delete;

  // move
  OperationResult(OperationResult&& other) = default;
  OperationResult& operator=(OperationResult&& other) noexcept {
    if (this != &other) {
      result = std::move(other.result);
      buffer = std::move(other.buffer);
      options = std::move(other.options);
      countErrorCodes = std::move(other.countErrorCodes);
    }
    return *this;
  }

  // create result with details
  OperationResult(Result result, std::shared_ptr<VPackBuffer<uint8_t>> buffer,
                  OperationOptions options,
                  std::unordered_map<ErrorCode, size_t> countErrorCodes = {})
      : result(std::move(result)),
        buffer(std::move(buffer)),
        options(std::move(options)),
        countErrorCodes(std::move(countErrorCodes)) {
    if (this->result.ok()) {
      TRI_ASSERT(this->buffer != nullptr);
      TRI_ASSERT(this->buffer->data() != nullptr);
    }
  }

  ~OperationResult() = default;

  // Result-like interface
  bool ok() const noexcept { return result.ok(); }
  bool fail() const noexcept { return result.fail(); }
  ErrorCode errorNumber() const noexcept { return result.errorNumber(); }
  bool is(ErrorCode errorNumber) const noexcept {
    return result.is(errorNumber);
  }
  bool isNot(ErrorCode errorNumber) const noexcept {
    return result.isNot(errorNumber);
  }
  std::string_view errorMessage() const { return result.errorMessage(); }

  bool hasSlice() const noexcept { return buffer != nullptr; }
  velocypack::Slice slice() const noexcept {
    TRI_ASSERT(buffer != nullptr);
    return VPackSlice(buffer->data());
  }

  void reset() noexcept {
    result.reset();
    buffer.reset();
    options = OperationOptions();
    countErrorCodes.clear();
  }

  Result result;
  std::shared_ptr<velocypack::Buffer<uint8_t>> buffer;
  OperationOptions options;

  // Executive summary for baby operations: reports all errors that did occur
  // during these operations. Details are stored in the respective positions of
  // the failed documents.
  std::unordered_map<ErrorCode, size_t> countErrorCodes;
};

}  // namespace arangodb
