// Copyright 2020 The TensorFlow Runtime Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//===- core_runtime.cc ----------------------------------------------------===//
//
// This file implements MLIR operation functions for the core runtime library.
//
//===----------------------------------------------------------------------===//
#include "tfrt/core_runtime/opdefs/corert_utils.h"

#include "mlir/IR/Builders.h"
#include "tfrt/basic_kernels/opdefs/tfrt_base.h"
#include "tfrt/basic_kernels/opdefs/types.h"
#include "tfrt/core_runtime/opdefs/types.h"

namespace tfrt {
namespace corert {

static Type GetDeviceType(Builder *builder) {
  return builder->getType<DeviceType>();
}

static Type GetChainType(Builder *builder) {
  return builder->getType<ChainType>();
}

static Type GetTensorHandleType(Builder *builder) {
  return builder->getType<TensorHandleType>();
}

ParseResult ParseExecuteOpImpl(OpAsmParser &parser, OperationState &result,
                               int num_chains) {
  auto &builder = parser.getBuilder();
  auto device_type = GetDeviceType(&builder);
  auto chain_type = GetChainType(&builder);
  auto tensorhandle_type = GetTensorHandleType(&builder);

  StringAttr op_name;
  SmallVector<OpAsmParser::OperandType, 4> device_and_in_chains;
  SmallVector<OpAsmParser::OperandType, 4> operands;
  NamedAttrList op_attrs;
  auto loc = parser.getNameLoc();
  if (parser.parseOperandList(device_and_in_chains,
                              /*requiredOperandCount=*/num_chains + 1,
                              OpAsmParser::Delimiter::Paren) ||
      parser.parseAttribute(op_name, "op_name", result.attributes) ||
      parser.parseOperandList(operands, OpAsmParser::Delimiter::Paren) ||
      parser.parseOptionalAttrDict(op_attrs))
    return failure();

  int64_t num_results = 0;
  if (succeeded(parser.parseOptionalColon())) {
    IntegerAttr attr;
    mlir::NamedAttrList attrs;
    if (failed(parser.parseAttribute(attr, "num_results", attrs)))
      return failure();
    num_results = attr.getValue().getSExtValue();
  }

  SmallVector<Type, 4> operand_types;
  operand_types.push_back(device_type);
  operand_types.append(num_chains, chain_type);
  if (parser.resolveOperands(device_and_in_chains, operand_types, loc,
                             result.operands) ||
      parser.resolveOperands(operands, tensorhandle_type, result.operands))
    return failure();

  result.types.append(num_chains, chain_type);
  result.types.append(num_results, tensorhandle_type);

  SmallVector<Attribute, 4> op_attr_array;
  for (const auto &key_value : op_attrs) {
    auto key = builder.getStringAttr(key_value.first.strref());
    auto value = key_value.second;
    op_attr_array.push_back(builder.getArrayAttr({key, value}));
  }

  result.attributes.push_back(
      builder.getNamedAttr("op_attrs", builder.getArrayAttr(op_attr_array)));

  return success();
}

}  // namespace corert
}  // namespace tfrt
