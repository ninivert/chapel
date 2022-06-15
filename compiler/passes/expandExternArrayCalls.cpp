/*
 * Copyright 2020-2022 Hewlett Packard Enterprise Development LP
 * Copyright 2004-2019 Cray Inc.
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

#include "passes.h"

#include "astutil.h"
#include "baseAST.h"
#include "build.h"
#include "stmt.h"
#include "stringutil.h"
#include "symbol.h"

#include "global-ast-vecs.h"

#include <iostream>
#include <set>

// The goal of this pass is to convert any extern proc declarations that
// contain an array argument to take a c_ptr instead and provide a wrapping
// function that can handle being passed a Chapel array.
//
// So we take a declaration like:
//
//    extern proc print_array(x: [] int, n: int);
//
// And create:
//
//    // This will have the name print_array in the generated C code
//    extern proc chpl__extern_array_print_array(x: c_ptr(int), n: int);
//
//    // This will have the name chpl__extern_array_wrapper_print_array in the
//    // generated C code
//    inline proc print_array(x: [] int, n: int) {
//      chpl__extern_array_print_array(c_ptrTo(x), n);
//    }
//
// If no element type is specified for the array a c_void_ptr will be used. If
// we leave out the element type in the example above the result would be:
//
//    extern proc chpl__extern_array_print_array(x: c_void_ptr, n: int);
//
//    inline proc print_array(x: [], n: int) {
//      chpl__extern_array_print_array((c_ptrTo(x)):c_void_ptr, n);
//    }

bool ExpandExternArrayCalls::shouldProcess(FnSymbol* fn) {
  if (!fn->hasFlag(FLAG_EXTERN)) return false;

  for_formals(formal, fn) {
    if (isFormalArray(formal)) return true;
  }

  return false;
}

void ExpandExternArrayCalls::process(FnSymbol* fn) {
  std::set<Expr*> cptrScopes;
  std::set<int> replaced_args;
  int current_formal = -1;

  SET_LINENO(fn);
  FnSymbol* fcopy = fn->copy();

  for_formals(formal, fn) {
    UnresolvedSymExpr* eltType = nullptr;
    current_formal++;

    if (!isFormalArray(formal, &eltType)) continue;

    replaced_args.insert(current_formal);

    if (eltType) {
      // replace '[] foo' with 'c_ptr(foo)'
      SET_LINENO(formal);
      formal->typeExpr->replace(
          new BlockStmt(
            new CallExpr("c_ptr", eltType->remove())));
    } else {
      // generic arrays are replaced with 'c_void_ptr'
      SET_LINENO(formal);
      formal->typeExpr->replace(
          new BlockStmt(
            new UnresolvedSymExpr("chpl__c_void_ptr")));
    }
  }

  // There should always be something to do.
  INT_ASSERT(replaced_args.size());

  Expr* parentScope = fn->defPoint->parentExpr;
  if (cptrScopes.count(parentScope) == 0) {
    auto use = new UseStmt(new UnresolvedSymExpr("CTypes"), "", true);
    auto useBlock = buildChapelStmt(use);
    fn->defPoint->insertAfter(useBlock);
    cptrScopes.insert(parentScope);
  }

  fn->defPoint->insertAfter(new DefExpr(fcopy));
  fn->addFlag(FLAG_EXTERN_FN_WITH_ARRAY_ARG);
  fn->addFlag(FLAG_VOID_NO_RETURN_VALUE);
  fcopy->removeFlag(FLAG_EXTERN);
  fcopy->removeFlag(FLAG_NO_FN_BODY);
  fcopy->addFlag(FLAG_INLINE);

  fcopy->cname = astr("chpl__extern_array_wrapper_", fcopy->cname);
  fn->name = astr("chpl__extern_array_", fn->name);

  // Create the function call and add the arguments one at time, replacing
  // arrays when necessary
  CallExpr* externCall = new CallExpr(fn);
  current_formal = 0;
  for_formals(formal, fcopy) {
    if(replaced_args.count(current_formal)) {
      UnresolvedSymExpr* eltType = NULL;
      std::ignore = isFormalArray(formal, &eltType);
      externCall->argList.insertAtTail(new CallExpr("chpl_arrayToPtr",
                                                    new SymExpr(formal),
                                                    new SymExpr((eltType ?
                                                                 gFalse :
                                                                 gTrue))));
    } else {
      externCall->argList.insertAtTail(new SymExpr(formal));
    }
    current_formal++;
  }

  if (fn->retType == dtVoid || isRetExprVoid(fn->retExprType)) {
    fcopy->body->replace(new BlockStmt(externCall));
  } else {
    auto body = new BlockStmt(new CallExpr(PRIM_RETURN, externCall));
    fcopy->body->replace(body);
  }
}

// Returns true if formal is an array
// After the function, eltType will be set to the element type of the array if
// it is found, NULL otherwise
bool ExpandExternArrayCalls::isFormalArray(ArgSymbol* formal,
                                           UnresolvedSymExpr** outType) {
  if (BlockStmt* bs = formal->typeExpr) {
    INT_ASSERT(bs->length() == 1);
    Expr* firstExpr = bs->body.first();
    INT_ASSERT(firstExpr);
    if (CallExpr* typeAsCall = toCallExpr(firstExpr)) {
      UnresolvedSymExpr* urSym = toUnresolvedSymExpr(typeAsCall->baseExpr);
      auto arrayRtTypeFn = "chpl__buildArrayRuntimeType";
      if (urSym && !strcmp(urSym->unresolved, arrayRtTypeFn)) {
        if (typeAsCall->numActuals() > 1) {
          if (outType) {
            *outType= toUnresolvedSymExpr(typeAsCall->get(2));
          }
        }
        return true;
      }
    }
  }
  return false;
}

bool ExpandExternArrayCalls::isRetExprVoid(BlockStmt* retExprType) {
  if (retExprType != NULL && retExprType->body.length == 1) {
    if (SymExpr* se = toSymExpr(retExprType->body.only())) {
      if (se->symbol()->type == dtVoid) {
        return true;
      }
    }
  }
  return false;
}

void expandExternArrayCalls() {
  PassManager pm;
  pm.runPass<FnSymbol*>(ExpandExternArrayCalls(), gFnSymbols);
}
