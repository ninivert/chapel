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

/* Support for move-initializing and deinitializing values.

  The :mod:`MemMove` module provides functions which enable users to
  move-initialize and deinitialize values.

  The functionality provided by this module can be used to implement
  collections in a manner similar to those implemented by the Chapel standard
  modules (such as :mod:`List` or :mod:`Map`).

  .. note::

    Throughout documentation, the term `variable` also includes non-const
    formals, array elements, record or class fields, and tuple components.


*/
module MemMove {

  // Mark as "unsafe" to silence lifetime errors.
  pragma "unsafe"
  private inline proc _move(ref dst, const ref src) {
    __primitive("=", dst, src);
  }

  /*
    Check to see if a given type needs to be deinitialized.

    :arg t: A type to check for deinitialization
    :type t: `type`

    :return: ``true`` if ``t`` needs to be deinitialized
    :rtype: param bool
  */
  proc needsDeinit(type t) param {
    return __primitive("needs auto destroy", t);
  }

  /*
    Explicitly deinitialize a variable. The variable referred to by ``arg``
    should be considered uninitialized after a call to this function.

    .. warning::

      At present the compiler does not account for deinitialization performed
      upon a call to :proc:`explicitDeinit()`. It should only be called
      when deinitialization would not occur otherwise.

    :arg: A variable to deinitialize
  */
  proc explicitDeinit(ref arg: ?t) {
    if needsDeinit(t) then
      chpl__autoDestroy(arg);
  }

  /*
    Move-initialize ``lhs`` with the value in ``rhs``. The contents of ``lhs``
    are not deinitialized before the move, and ``rhs`` is not deinitialized
    after the move.

    .. warning::

      If ``lhs`` references an already initialized variable, it will be
      overwritten by the contents of ``rhs`` without being deinitialized
      first. Call :proc:`explicitDeinit()` to deinitialize ``lhs`` if
      necessary.

    :arg lhs: A variable to move-initialize, whose type matches ``rhs``

    :arg rhs: A value to move-initialize from
  */
  pragma "last resort"
  deprecated "The formals 'lhs' and 'rhs' are deprecated, please use 'dst' and 'src' instead"
  proc moveInitialize(ref lhs,
                      pragma "no auto destroy"
                      pragma "error on copy" in rhs) {
    if __primitive("static typeof", lhs) != __primitive("static typeof", rhs) {
      compilerError("type mismatch move-initializing an expression of type '"+lhs.type:string+"' from one of type '"+rhs.type:string+"'");
    } else if __primitive("static typeof", lhs) != nothing {
      _move(lhs, rhs);
    }
  }

  /*
    Move-initialize ``dst`` with the value in ``src``. The contents of ``dst``
    are not deinitialized before the move, and ``src`` is not deinitialized
    after the move.

    .. warning::

      If ``dst`` references an already initialized variable, it will be
      overwritten by the contents of ``src`` without being deinitialized
      first. Call :proc:`explicitDeinit()` to deinitialize ``dst`` if
      necessary.

    .. warning::

      The static types of ``dst`` and ``src`` must match, or else a
      compile-time error will be issued.

    .. note::

      If the compiler inserts a copy for the argument to ``src``, then a
      compile-time error will be issued. The most likely cause is that the
      argument is used elsewhere following the call to ``moveInitialize``.
      The ``moveFrom`` function can be used with the ``src`` argument to avoid
      the copy when certain of the variable's usage:

      moveInitialize(myDst, moveFrom(mySrc));

    :arg dst: A variable to move-initialize, whose type matches ``src``

    :arg src: A value to move-initialize from
  */
  proc moveInitialize(ref dst,
                      pragma "no auto destroy"
                      pragma "error on copy" in src) {
    if __primitive("static typeof", dst) != __primitive("static typeof", src) {
      compilerError("type mismatch move-initializing an expression of type '"+dst.type:string+"' from one of type '"+src.type:string+"'");
    } else if __primitive("static typeof", dst) != nothing {
      _move(dst, src);
    }
  }

  /*
    Move the contents of the variable or constant referred to by ``arg`` into
    a new value.

    .. warning::

      The variable or constant referred to by ``arg`` should be considered
      uninitialized after a call to this function.

    :arg arg: A variable or constant to move

    :return: The contents of ``arg`` moved into a new value
  */
  deprecated "'moveToValue' is deprecated; please use 'moveFrom' instead"
  proc moveToValue(const ref arg: ?t) {
    if t == nothing {
      return none;
    } else {
      pragma "no init"
      pragma "no copy"
      pragma "no auto destroy"
      var result: t;
      _move(result, arg);
      return result;
    }
  }

  /*
    Move the contents of the variable or constant referred to by ``src`` into
    a new returned value.

    .. warning::

      The variable or constant referred to by ``src`` should be considered
      uninitialized after a call to this function.

    :arg src: A variable or constant to move

    :return: The contents of ``src`` moved into a new value
  */
  proc moveFrom(const ref src: ?t) {
    // Note: using 't' as an explicit return type can result in incorrect
    // behavior for arrays
    if t == nothing {
      return none;
    } else {
      pragma "no init"
      pragma "no copy"
      pragma "no auto destroy"
      var result: t;
      _move(result, src);
      return result;
    }
  }

  /*
    Swap the contents of the variables referred to by ``lhs`` and ``rhs``.
    This function does not call the ``<=>`` operator. Unlike the ``<=>``
    operator, :proc:`moveSwap()` does not perform assignment or
    initialization.

    :arg lhs: A variable to swap

    :arg rhs: A variable to swap
  */
  pragma "last resort"
  deprecated "the formals 'lhs' and 'rhs' are deprecated, please use 'x' and 'y' instead"
  proc moveSwap(ref lhs: ?t, ref rhs: t) {
    moveSwap(x=lhs, y=rhs);
  }

  /*
    Swap the contents of the variables referred to by ``x`` and ``y``.
    This function does not call the ``<=>`` operator. Unlike the ``<=>``
    operator, :proc:`moveSwap()` does not perform assignment or
    initialization.

    :arg x: A variable to swap

    :arg y: A variable to swap
  */
  proc moveSwap(ref x: ?t, ref y: t) {
    if t != nothing {
      pragma "no init"
      pragma "no copy"
      pragma "no auto destroy"
      var temp: t;
      _move(temp, x);
      _move(x, y);
      _move(y, temp);
    }
  }

  private inline proc _haltBadIndex(a, idx, indexName: string) {
    import HaltWrappers.boundsCheckHalt;

    if !a.domain.contains(idx) then
      boundsCheckHalt('Cannot move-initialize array because its domain ' +
                      'does not contain: ' + indexName);
  }

  // Call to 'indexOrder' assumes 'idx' is a valid index.
  private inline proc _haltBadElementRange(a, idx, numElements: int) {
    import HaltWrappers.boundsCheckHalt;

    if numElements > a.size then
      boundsCheckHalt('Cannot move-initialize array because number of ' +
                      'elements to copy exceeds array size');

    if numElements <= 0 then
      boundsCheckHalt('Cannot move-initialize array because number of ' +
                      'elements to copy is <= 0');

    const order = a.domain.indexOrder(idx);
    const hi = order + numElements;

    if hi > a.size then
      boundsCheckHalt('Cannot move-initialize array because one or ' +
                      'more indices fall outside its domain');
  }

  private inline proc _haltRangeOverlap(dstIndex, srcIndex, numElements) {
    import HaltWrappers.boundsCheckHalt;

    const dstRange = dstIndex..#numElements;
    const srcRange = srcIndex..#numElements;

    if dstRange[srcRange].size != 0 then
      boundsCheckHalt('Cannot move-initialize array because source and ' +
                      'destination ranges intersect');
  }

  // TODO: Relax this restriction in the future.
  private inline proc _errorNot1DRectangularArray(a) {
    if !a.isDefaultRectangular() || a.rank > 1 then
      compilerError('Can only move-initialize one-dimensional ',
                    'rectangular arrays', 2);
  }

  /*
    Move-initialize a group of array elements from a group of elements in the
    same array. This function is equivalent to a sequence of individual calls
    to :proc:`moveInitialize()`.

    .. warning::

      This function will halt if the value of ``numElements`` is negative,
      or if any of the elements in ``dstStartIndex..#numElements`` or
      ``srcStartIndex..#numElements`` are out of bounds.

      This function will halt if the ranges ``dstStartIndex..#numElements``
      and ``srcStartIndex..#numElements`` intersect.

      No checks will occur when the `--fast` or `--no-checks` flags are used.

    :arg a: The array with source and destination elements

    :arg dstStartIndex: Destination index of elements to move-initialize
    :type dstStartIndex: `a.idxType`

    :arg srcStartIndex: Source index of elements to move-initialize from
    :type srcStartIndex: `a.idxType`

    :arg numElements: The number of elements to move-initialize
    :type numElements: int
  */
  proc moveInitializeArrayElements(ref a: [?d], dstStartIndex: a.idxType,
                                   srcStartIndex: a.idxType,
                                   numElements: int) {
    _errorNot1DRectangularArray(a);

    if boundsChecking {
      _haltBadElementRange(a, dstStartIndex, numElements);
      _haltBadIndex(a, dstStartIndex, 'dstStartIndex');
      _haltBadIndex(a, srcStartIndex, 'srcStartIndex');
      _haltRangeOverlap(dstStartIndex, srcStartIndex, numElements);
    }

    if dstStartIndex == srcStartIndex then
      return;

    const d = a.domain;

    const dstLo = d.indexOrder(dstStartIndex);
    const srcLo = d.indexOrder(srcStartIndex);

    forall i in 0..<numElements {
      const dstIdx = d.orderToIndex(dstLo + i);
      const srcIdx = d.orderToIndex(srcLo + i);
      ref dst = a[dstIdx];
      ref src = a[srcIdx];
      _move(dst, src);
    }
  }

  /*
    Move-initialize a group of array elements from a group of elements in
    another array. This function is equivalent to a sequence of individual
    calls to :proc:`moveInitialize()`.

    .. warning::

      This function will halt if the value of ``numElements`` is negative,
      or if any of the elements in ``dstStartIndex..#numElements`` or
      ``srcStartIndex..#numElements`` are out of bounds.

      This function will halt if ``dstA`` and ``srcA`` are the same array
      and the ranges ``dstStartIndex..#numElements`` and
      ``srcStartIndex..#numElements`` intersect.

      No checks will occur when the `--fast` or `--no-checks` flags are used.

    :arg dstA: The array with destination elements

    :arg dstStartIndex: Destination index of elements to move-initialize
    :type dstStartIndex: `dstA.idxType`

    :arg srcA: The array with source elements
    :type srcA: An array with the same element type as `dstA.eltType`

    :arg srcStartIndex: Source index of elements to move-initialize from
    :type srcStartIndex: `srcA.idxType`

    :arg numElements: The number of elements to move-initialize
    :type numElements: int
  */
  proc moveInitializeArrayElements(ref dstA: [] ?t,
                                   dstStartIndex: dstA.idxType,
                                   srcA: [] t,
                                   srcStartIndex: srcA.idxType,
                                   numElements: int) {
    _errorNot1DRectangularArray(dstA);
    _errorNot1DRectangularArray(srcA);

    if boundsChecking {
      _haltBadElementRange(dstA, dstStartIndex, numElements);
      _haltBadElementRange(srcA, srcStartIndex, numElements);
      _haltBadIndex(dstA, dstStartIndex, 'dstStartIndex');
      _haltBadIndex(srcA, srcStartIndex, 'srcStartIndex');
    }

    const isSameArray = dstA._instance == srcA._instance;

    if boundsChecking && isSameArray {
      _haltRangeOverlap(dstStartIndex, srcStartIndex, numElements);
    }

    if isSameArray && dstStartIndex == srcStartIndex then
      return;

    const dstD = dstA.domain;
    const srcD = srcA.domain;

    var dstLo = dstD.indexOrder(dstStartIndex);
    var srcLo = dstD.indexOrder(srcStartIndex);

    // TODO: Optimize communication for this loop?
    forall i in 0..<numElements {
      const dstIdx = dstD.orderToIndex(dstLo + i);
      const srcIdx = srcD.orderToIndex(srcLo + i);
      ref dst = dstA[dstIdx];
      ref src = srcA[srcIdx];
      _move(dst, src);
    }
  }

// MemMove;
}

