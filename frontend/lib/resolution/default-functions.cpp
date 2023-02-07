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

#include "default-functions.h"

#include "chpl/framework/global-strings.h"
#include "chpl/framework/query-impl.h"
#include "chpl/parsing/parsing-queries.h"
#include "chpl/resolution/can-pass.h"
#include "chpl/resolution/resolution-queries.h"
#include "chpl/resolution/scope-queries.h"
#include "chpl/types/all-types.h"
#include "chpl/uast/all-uast.h"

namespace chpl {
namespace resolution {


using namespace uast;
using namespace types;


/**
  Return true if 'name' is the name of a compiler generated method.
*/
static bool isNameOfCompilerGeneratedMethod(UniqueString name) {
  // TODO: Update me over time.
  if (name == USTR("init")       ||
      name == USTR("deinit")     ||
      name == USTR("init=")) {
    return true;
  }

  return false;
}

static bool
areOverloadsPresentInDefiningScope(Context* context, const Type* type,
                                   UniqueString name) {
  const Scope* scopeForReceiverType = nullptr;

  if (auto compType = type->getCompositeType()) {
    scopeForReceiverType = scopeForId(context, compType->id());
  }

  // there is no defining scope
  if (!scopeForReceiverType) return false;

  // do not look outside the defining module
  const LookupConfig config = LOOKUP_DECLS | LOOKUP_PARENTS;

  auto vec = lookupNameInScope(context, scopeForReceiverType,
                               /* receiver scopes */ {},
                               name, config);

  // nothing found
  if (vec.size() == 0) return false;

  auto haveQt = QualifiedType(QualifiedType::VAR, type);

  // loop through IDs and see if any are methods on the same type
  for (auto& ids : vec) {
    for (auto id : ids) {
      auto node = parsing::idToAst(context, id);
      CHPL_ASSERT(node);

      if (auto fn = node->toFunction()) {
        if (!fn->isMethod()) continue;

        auto ufs = UntypedFnSignature::get(context, fn->id());

        // TODO: way to just compute formal type instead of whole TFS?
        auto tfs = typedSignatureInitial(context, ufs);
        auto receiverQualType = tfs->formalType(0);

        // return true if the receiver type matches or
        // if the receiver type is a generic type and we have
        // an instantiation.
        auto result = canPass(context, haveQt, receiverQualType);
        if (result.passes() && !result.converts() && !result.promotes()) {
          return true;
        }
      }
    }
  }

  return false;
}

bool
needCompilerGeneratedMethod(Context* context, const Type* type,
                            UniqueString name, bool parenless) {
  if (isNameOfCompilerGeneratedMethod(name)) {
    if (!areOverloadsPresentInDefiningScope(context, type, name)) {
      return true;
    }
  }

  return false;
}

// generates the untyped function signature and typed function
// signature formal entries for the 'this' method receiver for
// some kind of 'init' or 'init='
static void
generateInitParts(Context* context,
                  const CompositeType* inCompType,
                  const CompositeType*& compType,
                  std::vector<UntypedFnSignature::FormalDetail>& ufsFormals,
                  std::vector<QualifiedType>& formalTypes) {
  // adjust to refer to fully generic signature if needed
  auto genericCompType = inCompType->instantiatedFromCompositeType();
  compType = genericCompType ? genericCompType : inCompType;

  // start by adding a formal for the receiver
  auto ufsReceiver = UntypedFnSignature::FormalDetail(USTR("this"),
                                                      false,
                                                      nullptr);
  ufsFormals.push_back(std::move(ufsReceiver));

  // Determine the receiver type and intent.
  QualifiedType qtReceiver;

  // If the receiver is a record type, just give it the 'ref' intent.
  if (compType->isRecordType()) {
    qtReceiver = QualifiedType(QualifiedType::REF, compType);

  // If the receiver is a basic class C, use 'const in x: borrowed C'.
  } else if (auto basic = compType->toBasicClassType()) {
    const Type* manager = nullptr;
    auto nonNilBorrowed  = ClassTypeDecorator::BORROWED_NONNIL;
    auto decor = ClassTypeDecorator(nonNilBorrowed);
    auto receiverType = ClassType::get(context, basic, manager, decor);
    CHPL_ASSERT(receiverType);
    qtReceiver = QualifiedType(QualifiedType::CONST_IN, receiverType);
  } else {
    CHPL_ASSERT(false && "Not handled yet!");
  }

  formalTypes.push_back(std::move(qtReceiver));
}


static const TypedFnSignature*
generateInitSignature(Context* context, const CompositeType* inCompType) {
  const CompositeType* compType = nullptr;
  std::vector<UntypedFnSignature::FormalDetail> ufsFormals;
  std::vector<QualifiedType> formalTypes;

  generateInitParts(context, inCompType, compType, ufsFormals, formalTypes);

  // consult the fields to build up the remaining untyped formals
  const DefaultsPolicy defaultsPolicy = DefaultsPolicy::IGNORE_DEFAULTS;
  auto& rf = fieldsForTypeDecl(context, compType, defaultsPolicy);

  // TODO: generic types
  if (rf.isGeneric()) {
    CHPL_ASSERT(false && "Not handled yet!");
  }

  // TODO: super fields and invoking super
  if (auto basic = compType->toBasicClassType()) {
    if (auto parent = basic->parentClassType()) {
      if (!parent->isObjectType()) {
        CHPL_ASSERT(false && "Not handled yet!");
      }
    }
  }

  // push all fields -> formals in order
  for (int i = 0; i < rf.numFields(); i++) {
    auto qualType = rf.fieldType(i);
    auto name = rf.fieldName(i);
    bool hasDefault = rf.fieldHasDefaultValue(i);
    const uast::Decl* node = nullptr;

    auto fd = UntypedFnSignature::FormalDetail(name, hasDefault, node);
    ufsFormals.push_back(std::move(fd));

    // for types & param, use the field kind, for values use 'in' intent
    if (qualType.isType() || qualType.isParam()) {
      formalTypes.push_back(qualType);
    } else {
      auto qt = QualifiedType(QualifiedType::IN, qualType.type());
      formalTypes.push_back(std::move(qt));
    }
  }

  // build the untyped signature
  auto ufs = UntypedFnSignature::get(context,
                        /*id*/ compType->id(),
                        /*name*/ USTR("init"),
                        /*isMethod*/ true,
                        /*isTypeConstructor*/ false,
                        /*isCompilerGenerated*/ true,
                        /*idTag*/ parsing::idToTag(context, compType->id()),
                        /*kind*/ uast::Function::Kind::PROC,
                        /*formals*/ std::move(ufsFormals),
                        /*whereClause*/ nullptr);

  // now build the other pieces of the typed signature
  bool needsInstantiation = rf.isGeneric();

  auto ret = TypedFnSignature::get(context,
                                   ufs,
                                   std::move(formalTypes),
                                   TypedFnSignature::WHERE_NONE,
                                   needsInstantiation,
                                   /* instantiatedFrom */ nullptr,
                                   /* parentFn */ nullptr,
                                   /* formalsInstantiated */ Bitmap());

  return ret;
}

static const TypedFnSignature*
generateInitCopySignature(Context* context, const CompositeType* inCompType) {
  const CompositeType* compType = nullptr;
  std::vector<UntypedFnSignature::FormalDetail> ufsFormals;
  std::vector<QualifiedType> formalTypes;

  generateInitParts(context, inCompType, compType, ufsFormals, formalTypes);

  // add a formal for the 'other' argument
  auto name = UniqueString::get(context, "other");
  bool hasDefault = false;
  const uast::Decl* node = nullptr;

  auto fd = UntypedFnSignature::FormalDetail(name, hasDefault, node);
  ufsFormals.push_back(std::move(fd));

  CHPL_ASSERT(formalTypes.size() == 1);
  auto otherType = QualifiedType(QualifiedType::CONST_REF,
                                 formalTypes[0].type());
  formalTypes.push_back(std::move(otherType));

  // build the untyped signature
  auto ufs = UntypedFnSignature::get(context,
                        /*id*/ compType->id(),
                        /*name*/ USTR("init"),
                        /*isMethod*/ true,
                        /*isTypeConstructor*/ false,
                        /*isCompilerGenerated*/ true,
                        /*idTag*/ parsing::idToTag(context, compType->id()),
                        /*kind*/ uast::Function::Kind::PROC,
                        /*formals*/ std::move(ufsFormals),
                        /*whereClause*/ nullptr);

  // now build the other pieces of the typed signature
  auto g = getTypeGenericity(context, formalTypes[1]);
  bool needsInstantiation = (g == Type::GENERIC ||
                             g == Type::GENERIC_WITH_DEFAULTS);

  auto ret = TypedFnSignature::get(context,
                                   ufs,
                                   std::move(formalTypes),
                                   TypedFnSignature::WHERE_NONE,
                                   needsInstantiation,
                                   /* instantiatedFrom */ nullptr,
                                   /* parentFn */ nullptr,
                                   /* formalsInstantiated */ Bitmap());

  return ret;
}

static const TypedFnSignature*
generateDeinitSignature(Context* context, const CompositeType* inCompType) {
  const CompositeType* compType = nullptr;
  std::vector<UntypedFnSignature::FormalDetail> ufsFormals;
  std::vector<QualifiedType> formalTypes;

  generateInitParts(context, inCompType, compType, ufsFormals, formalTypes);

  // build the untyped signature
  auto ufs = UntypedFnSignature::get(context,
                        /*id*/ compType->id(),
                        /*name*/ USTR("init"),
                        /*isMethod*/ true,
                        /*isTypeConstructor*/ false,
                        /*isCompilerGenerated*/ true,
                        /*idTag*/ parsing::idToTag(context, compType->id()),
                        /*kind*/ uast::Function::Kind::PROC,
                        /*formals*/ std::move(ufsFormals),
                        /*whereClause*/ nullptr);

  // now build the other pieces of the typed signature
  auto g = getTypeGenericity(context, formalTypes[0].type());
  bool needsInstantiation = (g == Type::GENERIC ||
                             g == Type::GENERIC_WITH_DEFAULTS);

  auto ret = TypedFnSignature::get(context,
                                   ufs,
                                   std::move(formalTypes),
                                   TypedFnSignature::WHERE_NONE,
                                   needsInstantiation,
                                   /* instantiatedFrom */ nullptr,
                                   /* parentFn */ nullptr,
                                   /* formalsInstantiated */ Bitmap());

  return ret;
}

static const TypedFnSignature* const&
fieldAccessorQuery(Context* context,
                   const types::CompositeType* compType,
                   UniqueString fieldName) {
  QUERY_BEGIN(fieldAccessorQuery, context, compType, fieldName);

  const TypedFnSignature* result = nullptr;
  std::vector<UntypedFnSignature::FormalDetail> ufsFormals;
  std::vector<QualifiedType> formalTypes;

  // start by adding a formal for the receiver
  auto ufsReceiver = UntypedFnSignature::FormalDetail(USTR("this"),
                                                      false,
                                                      nullptr);
  ufsFormals.push_back(std::move(ufsReceiver));

  const Type* thisType = compType;
  if (auto bct = thisType->toBasicClassType()) {
    auto dec = ClassTypeDecorator(ClassTypeDecorator::BORROWED_NONNIL);
    thisType = ClassType::get(context, bct, /*manager*/ nullptr, dec);
  }

  // receiver is 'ref' to allow mutation
  // TODO: indicate that its const-ness should vary with receiver const-ness
  formalTypes.push_back(QualifiedType(QualifiedType::REF, thisType));

  ID fieldId = parsing::fieldIdWithName(context, compType->id(), fieldName);

  // build the untyped signature
  auto ufs = UntypedFnSignature::get(context,
                        /*id*/ fieldId,
                        /*name*/ fieldName,
                        /*isMethod*/ true,
                        /*isTypeConstructor*/ false,
                        /*isCompilerGenerated*/ true,
                        /*idTag*/ parsing::idToTag(context, fieldId),
                        /*kind*/ uast::Function::Kind::PROC,
                        /*formals*/ std::move(ufsFormals),
                        /*whereClause*/ nullptr);

  // now build the other pieces of the typed signature
  result = TypedFnSignature::get(context, ufs, std::move(formalTypes),
                                 TypedFnSignature::WHERE_NONE,
                                 /* needsInstantiation */ false,
                                 /* instantiatedFrom */ nullptr,
                                 /* parentFn */ nullptr,
                                 /* formalsInstantiated */ Bitmap());

  return QUERY_END(result);
}

const TypedFnSignature* fieldAccessor(Context* context,
                                      const types::CompositeType* compType,
                                      UniqueString fieldName) {
  if (compType == nullptr) {
    return nullptr;
  }

  return fieldAccessorQuery(context, compType, fieldName);
}

static const TypedFnSignature* const&
getCompilerGeneratedMethodQuery(Context* context, const Type* type,
                                UniqueString name, bool parenless) {
  QUERY_BEGIN(getCompilerGeneratedMethodQuery, context, type, name, parenless);

  const TypedFnSignature* result = nullptr;

  if (needCompilerGeneratedMethod(context, type, name, parenless)) {
    auto compType = type->getCompositeType();
    CHPL_ASSERT(compType);

    if (name == USTR("init")) {
      result = generateInitSignature(context, compType);
    } else if (name == USTR("init=")) {
      result = generateInitCopySignature(context, compType);
    } else if (name == USTR("deinit")) {
      result = generateDeinitSignature(context, compType);
    } else {
      CHPL_ASSERT(false && "Not implemented yet!");
    }
  }

  return QUERY_END(result);
}

/**
  Given a type and a UniqueString representing the name of a method,
  determine if the type needs a method with such a name to be
  generated for it, and if so, generates and returns a
  TypedFnSignature representing the generated method.

  If no method was generated, returns nullptr.
*/
const TypedFnSignature*
getCompilerGeneratedMethod(Context* context, const Type* type,
                           UniqueString name, bool parenless) {
  return getCompilerGeneratedMethodQuery(context, type, name, parenless);
}


} // end namespace resolution
} // end namespace chpl
