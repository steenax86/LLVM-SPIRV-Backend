//===- SideEffectUtils.cpp - Side Effect Utils ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Transforms/SideEffectUtils.h"
#include "mlir/IR/Operation.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

using namespace mlir;

bool mlir::isMemoryEffectFree(Operation *op) {
  if (auto memInterface = dyn_cast<MemoryEffectOpInterface>(op)) {
    // If the op has side-effects, it cannot be moved.
    if (!memInterface.hasNoEffect())
      return false;
    // If the op does not have recursive side effects, then it can be moved.
    if (!op->hasTrait<OpTrait::HasRecursiveMemoryEffects>())
      return true;
  } else if (!op->hasTrait<OpTrait::HasRecursiveMemoryEffects>()) {
    // Otherwise, if the op does not implement the memory effect interface and
    // it does not have recursive side effects, then it cannot be known that the
    // op is moveable.
    return false;
  }

  // Recurse into the regions and ensure that all nested ops can also be moved.
  for (Region &region : op->getRegions())
    for (Operation &op : region.getOps())
      if (!isMemoryEffectFree(&op))
        return false;
  return true;
}

bool mlir::isSpeculatable(Operation *op) {
  auto conditionallySpeculatable = dyn_cast<ConditionallySpeculatable>(op);
  if (!conditionallySpeculatable)
    return false;

  switch (conditionallySpeculatable.getSpeculatability()) {
  case Speculation::RecursivelySpeculatable:
    for (Region &region : op->getRegions()) {
      for (Operation &op : region.getOps())
        if (!isSpeculatable(&op))
          return false;
    }
    return true;

  case Speculation::Speculatable:
    return true;

  case Speculation::NotSpeculatable:
    return false;
  }
}
