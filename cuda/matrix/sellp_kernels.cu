/*******************************<GINKGO LICENSE>******************************
Copyright (c) 2017-2022, the Ginkgo authors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************<GINKGO LICENSE>*******************************/

#include "core/matrix/sellp_kernels.hpp"


#include <ginkgo/core/base/exception_helpers.hpp>
#include <ginkgo/core/base/math.hpp>
#include <ginkgo/core/base/types.hpp>
#include <ginkgo/core/matrix/csr.hpp>
#include <ginkgo/core/matrix/dense.hpp>


#include "core/components/prefix_sum_kernels.hpp"
#include "cuda/base/config.hpp"
#include "cuda/base/cusparse_bindings.hpp"
#include "cuda/base/types.hpp"
#include "cuda/components/reduction.cuh"
#include "cuda/components/thread_ids.cuh"


namespace gko {
namespace kernels {
namespace cuda {
/**
 * @brief The SELL-P matrix format namespace.
 *
 * @ingroup sellp
 */
namespace sellp {


constexpr int default_block_size = 512;


#include "common/cuda_hip/matrix/sellp_kernels.hpp.inc"


template <typename ValueType, typename IndexType>
void spmv(std::shared_ptr<const CudaExecutor> exec,
          const matrix::Sellp<ValueType, IndexType>* a,
          const matrix::Dense<ValueType>* b, matrix::Dense<ValueType>* c)
{
    const auto block_size = default_block_size;
    const dim3 grid(ceildiv(a->get_size()[0], block_size), b->get_size()[1]);

    if (grid.x > 0 && grid.y > 0) {
        spmv_kernel<<<grid, block_size>>>(
            a->get_size()[0], b->get_size()[1], b->get_stride(),
            c->get_stride(), a->get_slice_size(), a->get_const_slice_sets(),
            as_cuda_type(a->get_const_values()), a->get_const_col_idxs(),
            as_cuda_type(b->get_const_values()), as_cuda_type(c->get_values()));
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(GKO_DECLARE_SELLP_SPMV_KERNEL);


template <typename ValueType, typename IndexType>
void advanced_spmv(std::shared_ptr<const CudaExecutor> exec,
                   const matrix::Dense<ValueType>* alpha,
                   const matrix::Sellp<ValueType, IndexType>* a,
                   const matrix::Dense<ValueType>* b,
                   const matrix::Dense<ValueType>* beta,
                   matrix::Dense<ValueType>* c)
{
    const auto block_size = default_block_size;
    const dim3 grid(ceildiv(a->get_size()[0], block_size), b->get_size()[1]);

    if (grid.x > 0 && grid.y > 0) {
        advanced_spmv_kernel<<<grid, block_size>>>(
            a->get_size()[0], b->get_size()[1], b->get_stride(),
            c->get_stride(), a->get_slice_size(), a->get_const_slice_sets(),
            as_cuda_type(alpha->get_const_values()),
            as_cuda_type(a->get_const_values()), a->get_const_col_idxs(),
            as_cuda_type(b->get_const_values()),
            as_cuda_type(beta->get_const_values()),
            as_cuda_type(c->get_values()));
    }
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_SELLP_ADVANCED_SPMV_KERNEL);


}  // namespace sellp
}  // namespace cuda
}  // namespace kernels
}  // namespace gko
