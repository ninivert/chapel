/*
 * Copyright 2021-2023 Hewlett Packard Enterprise Development LP
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CHPL_UAST_FORALL_H
#define CHPL_UAST_FORALL_H

#include "chpl/framework/Location.h"
#include "chpl/uast/BlockStyle.h"
#include "chpl/uast/IndexableLoop.h"
#include "chpl/uast/WithClause.h"

namespace chpl {
namespace uast {


/**
  This class represents a forall loop. For example:

  \rst
  .. code-block:: chapel

      // Example 1:
      var x: atomic int;
      forall i in myRange {
        x.fetchAdd(i);
      }

  \endrst

 */
class Forall final : public IndexableLoop {
 private:
  Forall(AstList children, int8_t indexChildNum,
         int8_t iterandChildNum,
         int8_t withClauseChildNum,
         BlockStyle blockStyle,
         int loopBodyChildNum,
         bool isExpressionLevel)
    : IndexableLoop(asttags::Forall, std::move(children),
                    indexChildNum,
                    iterandChildNum,
                    withClauseChildNum,
                    blockStyle,
                    loopBodyChildNum,
                    isExpressionLevel,
                    NO_CHILD /*attributeGroup*/) {
  }

  Forall(Deserializer& des)
    : IndexableLoop(asttags::Forall, des) {
  }

  bool contentsMatchInner(const AstNode* other) const override {
    return indexableLoopContentsMatchInner(other->toIndexableLoop());
  }

  void markUniqueStringsInner(Context* context) const override {
    indexableLoopMarkUniqueStringsInner(context);
  }

 public:
  ~Forall() override = default;

  /**
    Create and return a forall loop.
  */
  static owned<Forall> build(Builder* builder, Location loc,
                             owned<Decl> index,
                             owned<AstNode> iterand,
                             owned<WithClause> withClause,
                             BlockStyle blockStyle,
                             owned<Block> body,
                             bool isExpressionLevel);

  void serialize(Serializer& ser) const override {
    IndexableLoop::serialize(ser);
  }

  DECLARE_STATIC_DESERIALIZE(Forall);

};


} // end namespace uast
} // end namespace chpl

#endif
