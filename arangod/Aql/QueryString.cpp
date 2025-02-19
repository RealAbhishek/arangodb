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

#include "QueryString.h"

#include "Basics/StringUtils.h"
#include "Basics/debugging.h"
#include "Basics/fasthash.h"

using namespace arangodb::aql;

bool QueryString::equal(QueryString const& other) const noexcept {
  return _hash == other._hash && _queryString == other._queryString;
}

void QueryString::append(std::string& out) const { out.append(_queryString); }

std::string QueryString::extract(size_t maxLength) const {
  if (size() <= maxLength) {
    // no truncation
    return _queryString;
  }

  // query string needs truncation
  size_t length = maxLength;

  // do not create invalid UTF-8 sequences
  while (length > 0) {
    uint8_t c = _queryString[length - 1];
    if ((c & 128) == 0) {
      // single-byte character
      break;
    }
    --length;

    // part of a multi-byte sequence
    if ((c & 192) == 192) {
      // decrease length by one more, so the string contains the
      // last part of the previous (multi-byte?) sequence
      break;
    }
  }

  std::string result;
  result.reserve(length + 15);
  result.append(data(), length);
  result.append("... (", 5);
  basics::StringUtils::itoa(size() - length, result);
  result.append(")", 1);
  return result;
}

/// @brief extract a region from the query
std::string QueryString::extractRegion(int line, int column) const {
  // note: line numbers reported by bison/flex start at 1, columns start at 0
  int currentLine = 1;
  int currentColumn = 0;

  char c;
  char const* s = _queryString.data();
  char const* p = _queryString.data();
  size_t const n = size();

  while ((static_cast<size_t>(p - s) < n) && (c = *p)) {
    if (currentLine > line ||
        (currentLine >= line && currentColumn >= column)) {
      break;
    }

    if (c == '\n') {
      ++p;
      ++currentLine;
      currentColumn = 0;
    } else if (c == '\r') {
      ++p;
      ++currentLine;
      currentColumn = 0;

      // eat a following newline
      if (*p == '\n') {
        ++p;
      }
    } else {
      ++currentColumn;
      ++p;
    }
  }

  // p is pointing at the position in the query the parse error occurred at
  TRI_ASSERT(p >= s);

  size_t offset = static_cast<size_t>(p - s);

  constexpr int snippetLength = 32;

  // copy query part, UTF-8-aware
  std::string result;
  result.reserve(snippetLength + 3 /*...*/);

  {
    char const* start = s + offset;
    char const* end = s + size();

    int charsFound = 0;

    while (start < end) {
      char c = *start;

      if ((c & 128) == 0 || (c & 192) == 192) {
        // ASCII character or start of a multi-byte sequence
        ++charsFound;
        if (charsFound > snippetLength) {
          break;
        }
      }

      result.push_back(c);
      ++start;
    }

    if (start != end) {
      result.append("...");
    }
  }

  return result;
}

uint64_t QueryString::computeHash() const noexcept {
  if (empty()) {
    return 0;
  }

  return fasthash64(data(), length(), 0x3123456789abcdef);
}

namespace arangodb {
namespace aql {

std::ostream& operator<<(std::ostream& stream, QueryString const& queryString) {
  if (queryString.empty()) {
    stream << "(empty query)";
  } else {
    stream.write(queryString.data(), queryString.length());
  }

  return stream;
}

}  // namespace aql
}  // namespace arangodb
