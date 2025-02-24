/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <executorch/kernels/portable/cpu/util/index_util.h>

namespace torch {
namespace executor {

bool check_index_select_args(
    const Tensor& in,
    int64_t dim,
    const Tensor& index,
    Tensor& out) {
  ET_LOG_AND_RETURN_IF_FALSE(tensor_has_dim(in, dim));
  dim = dim < 0 ? dim + nonzero_dim(in) : dim;
  ET_LOG_MSG_AND_RETURN_IF_FALSE(
      nonempty_size(in, dim) > 0,
      "index_select: Indexing axis dim should be positive");

  ET_LOG_AND_RETURN_IF_FALSE(tensors_have_same_dtype(in, out));
  ET_LOG_MSG_AND_RETURN_IF_FALSE(
      index.scalar_type() == ScalarType::Long ||
          index.scalar_type() == ScalarType::Int,
      "Expected index to have type of Long or Int, but found %s",
      toString(index.scalar_type()));

  ET_LOG_AND_RETURN_IF_FALSE(tensor_has_rank_smaller_or_equal_to(index, 1));
  if (index.dim() > 0 && in.dim() == 0) {
    ET_LOG_MSG_AND_RETURN_IF_FALSE(
        index.numel() == 1,
        "index_select: Index to scalar must have exactly 1 value");
  }

  if (index.scalar_type() == ScalarType::Long) {
    const int64_t* const index_ptr = index.const_data_ptr<int64_t>();
    for (size_t i = 0; i < index.numel(); ++i) {
      ET_LOG_MSG_AND_RETURN_IF_FALSE(
          index_ptr[i] >= 0 && index_ptr[i] < nonempty_size(in, dim),
          "index[%zu] = %" PRId64 " is out of range [0, %zd)",
          i,
          index_ptr[i],
          static_cast<size_t>(nonempty_size(in, dim)));
    }
  } else {
    const int32_t* const index_ptr = index.const_data_ptr<int32_t>();
    for (size_t i = 0; i < index.numel(); ++i) {
      ET_LOG_MSG_AND_RETURN_IF_FALSE(
          index_ptr[i] >= 0 && index_ptr[i] < nonempty_size(in, dim),
          "index[%zu] = %" PRId32 " is out of range [0, %zd)",
          i,
          index_ptr[i],
          static_cast<size_t>(nonempty_size(in, dim)));
    }
  }

  return true;
}

void get_index_select_out_target_size(
    const Tensor& in,
    int64_t dim,
    const Tensor& index,
    Tensor::SizesType* out_sizes,
    size_t* out_ndim) {
  *out_ndim = in.dim();
  for (size_t i = 0; i < in.dim(); ++i) {
    if (i == dim) {
      out_sizes[i] = index.numel();
    } else {
      out_sizes[i] = in.size(i);
    }
  }
}

bool check_scatter_add_args(
    const Tensor& self,
    int64_t dim,
    const Tensor& index,
    const Tensor& src,
    Tensor& out) {
  ET_LOG_AND_RETURN_IF_FALSE(tensors_have_same_dtype(self, out));
  ET_LOG_AND_RETURN_IF_FALSE(tensors_have_same_dtype(self, src));
  ET_LOG_MSG_AND_RETURN_IF_FALSE(
      index.scalar_type() == ScalarType::Long,
      "Expected dypte int64 for index");
  ET_LOG_AND_RETURN_IF_FALSE(tensor_has_dim(self, dim));

  if (index.numel() == 0) {
    return true;
  }

  ET_LOG_MSG_AND_RETURN_IF_FALSE(
      nonzero_dim(self) == nonzero_dim(src) &&
          nonzero_dim(self) == nonzero_dim(index),
      "self, index and src should have same number of dimensions.");

  // Normalize dim to non-negative value
  if (dim < 0) {
    dim += nonzero_dim(self);
  }

  for (size_t d = 0; d < nonzero_dim(self); ++d) {
    ET_LOG_MSG_AND_RETURN_IF_FALSE(
        nonempty_size(index, d) <= nonempty_size(src, d),
        "size of dimension %zd of index should be smaller than the size of that dimension of src",
        d);
    if (d != dim) {
      ET_LOG_MSG_AND_RETURN_IF_FALSE(
          nonempty_size(index, d) <= nonempty_size(self, d),
          "size of dimension %zd of index should be smaller than the size of that dimension of self if dimension %zd != dim %zd",
          d,
          d,
          (size_t)dim);
    }
  }
  const long* index_data = index.const_data_ptr<long>();
  for (size_t i = 0; i < index.numel(); ++i) {
    ET_LOG_MSG_AND_RETURN_IF_FALSE(
        index_data[i] >= 0 && index_data[i] < nonempty_size(self, dim),
        "Index is out of bounds for dimension %zd with size %zd",
        (size_t)dim,
        nonempty_size(self, dim));
  }
  return true;
}

} // namespace executor
} // namespace torch
