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

#include "SortCondition.h"

#include "Aql/AstNode.h"
#include "Aql/ExecutionNode/CalculationNode.h"
#include "Aql/ExecutionPlan.h"
#include "Aql/Expression.h"
#include "Basics/Exceptions.h"

using namespace arangodb::aql;

namespace {

/// @brief whether or not an attribute is contained in a vector
bool isContained(
    std::vector<std::vector<arangodb::basics::AttributeName>> const& attributes,
    std::vector<arangodb::basics::AttributeName> const& attribute) {
  for (auto const& it : attributes) {
    if (arangodb::basics::AttributeName::isIdentical(it, attribute, false)) {
      return true;
    }
  }

  return false;
}

}  // namespace

/// @brief create an empty condition
SortCondition::SortCondition()
    : _plan(nullptr),
      _nonNullAttributes(),
      _unidirectional(false),
      _onlyAttributeAccess(false),
      _ascending(true) {}

/// @brief create the sort condition
SortCondition::SortCondition(
    ExecutionPlan* plan,
    std::vector<std::pair<Variable const*, bool>> const& sorts,
    std::vector<std::vector<arangodb::basics::AttributeName>> const&
        constAttributes,
    ::arangodb::containers::HashSet<
        std::vector<arangodb::basics::AttributeName>> const& nonNullAttributes,
    std::unordered_map<VariableId, AstNode const*> const& variableDefinitions)
    : _plan(plan),
      _constAttributes(constAttributes),
      _nonNullAttributes(nonNullAttributes),
      _unidirectional(true),
      _onlyAttributeAccess(true),
      _ascending(true) {
  // note: _plan may be a nullptr during testing!

  bool foundDirection = false;

  std::vector<arangodb::basics::AttributeName> fieldNames;
  size_t const n = sorts.size();

  for (size_t i = 0; i < n; ++i) {
    bool isConst = false;  // const attribute?
    bool handled = false;
    Variable const* variable = sorts[i].first;
    TRI_ASSERT(variable != nullptr);
    auto variableId = variable->id;

    AstNode const* rootNode = nullptr;
    auto it = variableDefinitions.find(variableId);

    if (it != variableDefinitions.end()) {
      AstNode const* node = (*it).second;
      rootNode = node;

      if (node != nullptr && node->type == NODE_TYPE_ATTRIBUTE_ACCESS) {
        fieldNames.clear();

        while (node->type == NODE_TYPE_ATTRIBUTE_ACCESS) {
          fieldNames.emplace_back(
              arangodb::basics::AttributeName(node->getString(), false));
          node = node->getMember(0);
        }

        if (node->type == NODE_TYPE_REFERENCE) {
          handled = true;

          if (fieldNames.size() > 1) {
            std::reverse(fieldNames.begin(), fieldNames.end());
          }

          _fields.emplace_back(
              SortField{static_cast<Variable const*>(node->getData()),
                        fieldNames, rootNode, sorts[i].second});

          for (auto const& it2 : constAttributes) {
            if (it2 == fieldNames) {
              // const attribute
              isConst = true;
              break;
            }
          }
        }
      }
    } else if (_plan != nullptr) {
      ExecutionNode const* n = _plan->getVarSetBy(variableId);
      if (n != nullptr && n->getType() == ExecutionNode::CALCULATION) {
        Expression const* exp =
            ExecutionNode::castTo<CalculationNode const*>(n)->expression();
        if (exp != nullptr) {
          rootNode = exp->node();
        }
      }
    }

    if (!isConst) {
      // const attributes can be ignored for sorting
      if (!foundDirection) {
        // first attribute that we found
        foundDirection = true;
        _ascending = sorts[i].second;
      } else if (_unidirectional && sorts[i].second != _ascending) {
        _unidirectional = false;
      }
    }

    if (!handled) {
      _fields.emplace_back(
          SortField{variable, std::vector<arangodb::basics::AttributeName>(),
                    rootNode, sorts[i].second});
      _onlyAttributeAccess = false;
    }
  }

  if (n == 0) {
    _onlyAttributeAccess = false;
  }
}

/// @brief destroy the sort condition
SortCondition::~SortCondition() = default;

bool SortCondition::onlyUsesNonNullSortAttributes(
    std::vector<std::vector<arangodb::basics::AttributeName>> const& attributes)
    const {
  return std::all_of(
      attributes.begin(), attributes.end(), [this](auto const& it) {
        return _nonNullAttributes.find(it) != _nonNullAttributes.end();
      });
}

/// @brief returns the number of attributes in the sort condition covered
/// by the specified index fields
size_t SortCondition::coveredAttributes(
    Variable const* reference,
    std::vector<std::vector<arangodb::basics::AttributeName>> const&
        indexAttributes) const {
  size_t numCovered = 0;
  size_t indexPosition = 0;
  size_t fieldsPosition = 0;
  // iterate over all fields of the sort condition
  while (fieldsPosition < _fields.size() &&
         indexPosition < indexAttributes.size()) {
    auto const& field = _fields[fieldsPosition];

    if (reference != field.variable) {
      break;
    }

    // check if the field is present in the index definition too
    if (arangodb::basics::AttributeName::isIdentical(
            field.attributes, indexAttributes[indexPosition], false)) {
      ++indexPosition;
      ++fieldsPosition;
      ++numCovered;
    } else if (isContained(indexAttributes, field.attributes) &&
               isContained(_constAttributes, field.attributes)) {
      ++fieldsPosition;
      ++numCovered;
    } else if (isContained(_constAttributes, indexAttributes[indexPosition])) {
      ++indexPosition;
    } else {
      break;
    }
  }

  TRI_ASSERT(numCovered <= _fields.size());
  return numCovered;
}

size_t SortCondition::coveredUnidirectionalAttributes(
    Variable const* reference,
    std::vector<std::vector<arangodb::basics::AttributeName>> const&
        indexFieldAttributes) const {
  auto [numCovered, isAscending] = coveredUnidirectionalAttributesWithDirection(
      reference, indexFieldAttributes);
  return numCovered;
}

std::tuple<size_t, bool>
SortCondition::coveredUnidirectionalAttributesWithDirection(
    Variable const* reference,
    std::vector<std::vector<arangodb::basics::AttributeName>> const&
        indexFieldAttributes) const {
  size_t numCovered = 0;
  size_t indexPosition = 0;
  size_t fieldsPosition = 0;
  bool isAscending;

  while (fieldsPosition < _fields.size() &&
         indexPosition < indexFieldAttributes.size()) {
    auto const& sortField = _fields[fieldsPosition];
    auto const& indexAttributes = indexFieldAttributes[indexPosition];
    if (fieldsPosition == 0) {
      isAscending = sortField.asc;
    } else if (sortField.asc != isAscending) {
      break;
    }

    if (reference != sortField.variable) {
      break;
    }

    // check if the field is present in the index definition too
    if (arangodb::basics::AttributeName::isIdentical(sortField.attributes,
                                                     indexAttributes, false)) {
      ++indexPosition;
      ++fieldsPosition;
      // check order
      ++numCovered;
    } else if (isContained(indexFieldAttributes, sortField.attributes) &&
               isContained(_constAttributes, sortField.attributes)) {
      ++fieldsPosition;
      // check order
      ++numCovered;
    } else if (isContained(_constAttributes, indexAttributes)) {
      ++indexPosition;
    } else {
      break;
    }
  }

  TRI_ASSERT(numCovered <= _fields.size());
  return std::make_tuple(numCovered, isAscending);
}

std::tuple<Variable const*, AstNode const*, bool> SortCondition::field(
    size_t position) const {
  if (isEmpty() || position > numAttributes()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL,
                                   "out of range access to SortCondition");
  }

  TRI_ASSERT(position < _fields.size());

  SortField const& field = _fields[position];
  return std::make_tuple(field.variable, field.node, field.asc);
}
