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

template <typename IndexType>
void sort_agg(std::shared_ptr<const DefaultExecutor> exec, IndexType num,
              IndexType* row_idxs, IndexType* col_idxs)
{
    auto it = thrust::make_zip_iterator(
        thrust::make_tuple(thrust::device_pointer_cast(row_idxs),
                           thrust::device_pointer_cast(col_idxs)));
    thrust::sort(thrust::device, it, it + num);
}

GKO_INSTANTIATE_FOR_EACH_INDEX_TYPE(GKO_DECLARE_AMGX_PGM_SORT_AGG_KERNEL);


template <typename ValueType, typename IndexType>
void sort_row_major(std::shared_ptr<const DefaultExecutor> exec, size_type nnz,
                    IndexType* row_idxs, IndexType* col_idxs, ValueType* vals)
{
    // workaround for CUDA 9.2 Thrust: Their complex<> implementation is broken
    // due to overly generic assignment operator and constructor leading to
    // ambiguities. So we need to use our own fake_complex type
    using device_value_type = device_member_type<ValueType>;
    auto vals_it =
        thrust::device_pointer_cast(reinterpret_cast<device_value_type*>(vals));
    auto it = thrust::make_zip_iterator(
        thrust::make_tuple(thrust::device_pointer_cast(row_idxs),
                           thrust::device_pointer_cast(col_idxs)));
    // Because reduce_by_key is not determinstic, so we do not need
    // stable_sort_by_key
    // TODO: If we have determinstic reduce_by_key, it should be
    // stable_sort_by_key
    thrust::sort_by_key(thrust::device, it, it + nnz, vals_it);
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_AMGX_PGM_SORT_ROW_MAJOR);


template <typename ValueType, typename IndexType>
void compute_coarse_coo(std::shared_ptr<const DefaultExecutor> exec,
                        size_type fine_nnz, const IndexType* row_idxs,
                        const IndexType* col_idxs, const ValueType* vals,
                        matrix::Coo<ValueType, IndexType>* coarse_coo)
{
    // workaround for CUDA 9.2 Thrust: Their complex<> implementation is broken
    // due to overly generic assignment operator and constructor leading to
    // ambiguities. So we need to use our own fake_complex type
    using device_value_type = device_member_type<ValueType>;
    auto vals_it = thrust::device_pointer_cast(
        reinterpret_cast<const device_value_type*>(vals));
    auto key_it = thrust::make_zip_iterator(
        thrust::make_tuple(thrust::device_pointer_cast(row_idxs),
                           thrust::device_pointer_cast(col_idxs)));

    auto coarse_vals_it = thrust::device_pointer_cast(
        reinterpret_cast<device_value_type*>(coarse_coo->get_values()));
    auto coarse_key_it = thrust::make_zip_iterator(thrust::make_tuple(
        thrust::device_pointer_cast(coarse_coo->get_row_idxs()),
        thrust::device_pointer_cast(coarse_coo->get_col_idxs())));

    thrust::reduce_by_key(thrust::device, key_it, key_it + fine_nnz, vals_it,
                          coarse_key_it, coarse_vals_it);
}

GKO_INSTANTIATE_FOR_EACH_VALUE_AND_INDEX_TYPE(
    GKO_DECLARE_AMGX_PGM_COMPUTE_COARSE_COO);
