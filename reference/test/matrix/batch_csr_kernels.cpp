// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#include <ginkgo/core/matrix/batch_csr.hpp>


#include <complex>
#include <memory>
#include <random>


#include <gtest/gtest.h>


#include <ginkgo/core/base/batch_multi_vector.hpp>
#include <ginkgo/core/base/exception.hpp>
#include <ginkgo/core/base/executor.hpp>
#include <ginkgo/core/base/math.hpp>
#include <ginkgo/core/matrix/csr.hpp>
#include <ginkgo/core/matrix/dense.hpp>


#include "core/matrix/batch_csr_kernels.hpp"
#include "core/test/utils.hpp"


template <typename T>
class Csr : public ::testing::Test {
protected:
    using value_type = T;
    using size_type = gko::size_type;
    using BMtx = gko::batch::matrix::Csr<value_type>;
    using BMVec = gko::batch::MultiVector<value_type>;
    using CsrMtx = gko::matrix::Csr<value_type>;
    using DenseMtx = gko::matrix::Dense<value_type>;
    Csr()
        : exec(gko::ReferenceExecutor::create()),
          mtx_0(gko::batch::initialize<BMtx>(
              {{{1.0, -1.0, 0.0}, {-2.0, 2.0, 3.0}},
               {{1.0, -2.0, 0.0}, {1.0, -2.5, 4.0}}},
              exec, 5)),
          mtx_00(gko::initialize<CsrMtx>(
              {I<T>({1.0, -1.0, 0.0}), I<T>({-2.0, 2.0, 3.0})}, exec)),
          mtx_01(gko::initialize<CsrMtx>(
              {I<T>({1.0, -2.0, 0.0}), I<T>({1.0, -2.5, 4.0})}, exec)),
          b_0(gko::batch::initialize<BMVec>(
              {{I<T>({1.0, 0.0, 1.0}), I<T>({2.0, 0.0, 1.0}),
                I<T>({1.0, 0.0, 2.0})},
               {I<T>({-1.0, 1.0, 1.0}), I<T>({1.0, -1.0, 1.0}),
                I<T>({1.0, 0.0, 2.0})}},
              exec)),
          b_00(gko::initialize<DenseMtx>(
              {I<T>({1.0, 0.0, 1.0}), I<T>({2.0, 0.0, 1.0}),
               I<T>({1.0, 0.0, 2.0})},
              exec)),
          b_01(gko::initialize<DenseMtx>(
              {I<T>({-1.0, 1.0, 1.0}), I<T>({1.0, -1.0, 1.0}),
               I<T>({1.0, 0.0, 2.0})},
              exec)),
          x_0(gko::batch::initialize<BMVec>(
              {{I<T>({2.0, 0.0, 1.0}), I<T>({2.0, 0.0, 2.0})},
               {I<T>({-2.0, 1.0, 1.0}), I<T>({1.0, -1.0, -1.0})}},
              exec)),
          x_00(gko::initialize<DenseMtx>(
              {I<T>({2.0, 0.0, 1.0}), I<T>({2.0, 0.0, 2.0})}, exec)),
          x_01(gko::initialize<DenseMtx>(
              {I<T>({-2.0, 1.0, 1.0}), I<T>({1.0, -1.0, -1.0})}, exec))
    {}

    std::shared_ptr<const gko::ReferenceExecutor> exec;
    std::shared_ptr<BMtx> mtx_0;
    std::unique_ptr<CsrMtx> mtx_00;
    std::unique_ptr<CsrMtx> mtx_01;
    std::unique_ptr<BMVec> b_0;
    std::unique_ptr<DenseMtx> b_00;
    std::unique_ptr<DenseMtx> b_01;
    std::unique_ptr<BMVec> x_0;
    std::unique_ptr<DenseMtx> x_00;
    std::unique_ptr<DenseMtx> x_01;

    std::ranlux48 rand_engine;
};

TYPED_TEST_SUITE(Csr, gko::test::ValueTypes, TypenameNameGenerator);


TYPED_TEST(Csr, AppliesToBatchMultiVector)
{
    using T = typename TestFixture::value_type;

    this->mtx_0->apply(this->b_0.get(), this->x_0.get());

    this->mtx_00->apply(this->b_00.get(), this->x_00.get());
    this->mtx_01->apply(this->b_01.get(), this->x_01.get());
    auto res = gko::batch::unbatch<gko::batch::MultiVector<T>>(this->x_0.get());
    GKO_ASSERT_MTX_NEAR(res[0].get(), this->x_00.get(), r<T>::value);
    GKO_ASSERT_MTX_NEAR(res[1].get(), this->x_01.get(), r<T>::value);
}


TYPED_TEST(Csr, ConstAppliesToBatchMultiVector)
{
    using T = typename TestFixture::value_type;
    using BMtx = typename TestFixture::BMtx;

    static_cast<const BMtx*>(this->mtx_0.get())->apply(this->b_0, this->x_0);

    this->mtx_00->apply(this->b_00.get(), this->x_00.get());
    this->mtx_01->apply(this->b_01.get(), this->x_01.get());
    auto res = gko::batch::unbatch<gko::batch::MultiVector<T>>(this->x_0.get());
    GKO_ASSERT_MTX_NEAR(res[0].get(), this->x_00.get(), r<T>::value);
    GKO_ASSERT_MTX_NEAR(res[1].get(), this->x_01.get(), r<T>::value);
}


TYPED_TEST(Csr, AppliesLinearCombinationToBatchMultiVector)
{
    using BMVec = typename TestFixture::BMVec;
    using DenseMtx = typename TestFixture::DenseMtx;
    using T = typename TestFixture::value_type;
    auto alpha = gko::batch::initialize<BMVec>({{1.5}, {-1.0}}, this->exec);
    auto beta = gko::batch::initialize<BMVec>({{2.5}, {-4.0}}, this->exec);
    auto alpha0 = gko::initialize<DenseMtx>({1.5}, this->exec);
    auto alpha1 = gko::initialize<DenseMtx>({-1.0}, this->exec);
    auto beta0 = gko::initialize<DenseMtx>({2.5}, this->exec);
    auto beta1 = gko::initialize<DenseMtx>({-4.0}, this->exec);

    this->mtx_0->apply(alpha.get(), this->b_0.get(), beta.get(),
                       this->x_0.get());

    this->mtx_00->apply(alpha0.get(), this->b_00.get(), beta0.get(),
                        this->x_00.get());
    this->mtx_01->apply(alpha1.get(), this->b_01.get(), beta1.get(),
                        this->x_01.get());
    auto res = gko::batch::unbatch<gko::batch::MultiVector<T>>(this->x_0.get());
    GKO_ASSERT_MTX_NEAR(res[0].get(), this->x_00.get(), r<T>::value);
    GKO_ASSERT_MTX_NEAR(res[1].get(), this->x_01.get(), r<T>::value);
}


TYPED_TEST(Csr, ConstAppliesLinearCombinationToBatchMultiVector)
{
    using BMtx = typename TestFixture::BMtx;
    using BMVec = typename TestFixture::BMVec;
    using DenseMtx = typename TestFixture::DenseMtx;
    using T = typename TestFixture::value_type;
    auto alpha = gko::batch::initialize<BMVec>({{1.5}, {-1.0}}, this->exec);
    auto beta = gko::batch::initialize<BMVec>({{2.5}, {-4.0}}, this->exec);
    auto alpha0 = gko::initialize<DenseMtx>({1.5}, this->exec);
    auto alpha1 = gko::initialize<DenseMtx>({-1.0}, this->exec);
    auto beta0 = gko::initialize<DenseMtx>({2.5}, this->exec);
    auto beta1 = gko::initialize<DenseMtx>({-4.0}, this->exec);

    static_cast<const BMtx*>(this->mtx_0.get())
        ->apply(alpha.get(), this->b_0.get(), beta.get(), this->x_0.get());

    this->mtx_00->apply(alpha0.get(), this->b_00.get(), beta0.get(),
                        this->x_00.get());
    this->mtx_01->apply(alpha1.get(), this->b_01.get(), beta1.get(),
                        this->x_01.get());
    auto res = gko::batch::unbatch<gko::batch::MultiVector<T>>(this->x_0.get());
    GKO_ASSERT_MTX_NEAR(res[0].get(), this->x_00.get(), r<T>::value);
    GKO_ASSERT_MTX_NEAR(res[1].get(), this->x_01.get(), r<T>::value);
}


TYPED_TEST(Csr, CanTwoSidedScale)
{
    using value_type = typename TestFixture::value_type;
    using index_type = gko::int32;
    using BMtx = typename TestFixture::BMtx;
    auto col_scale = gko::array<value_type>(this->exec, 3 * 2);
    auto row_scale = gko::array<value_type>(this->exec, 2 * 2);
    col_scale.fill(2);
    row_scale.fill(3);

    gko::batch::matrix::two_sided_scale<value_type, index_type>(
        col_scale, row_scale, this->mtx_0.get());

    auto scaled_mtx_0 =
        gko::batch::initialize<BMtx>({{{6.0, -6.0, 0.0}, {-12.0, 12.0, 18.0}},
                                      {{6.0, -12.0, 0.0}, {6.0, -15.0, 24.0}}},
                                     this->exec, 5);
    GKO_ASSERT_BATCH_MTX_NEAR(this->mtx_0.get(), scaled_mtx_0.get(), 0.);
}


TYPED_TEST(Csr, CanTwoSidedScaleWithDifferentAlpha)
{
    using value_type = typename TestFixture::value_type;
    using index_type = gko::int32;
    using BMtx = typename TestFixture::BMtx;
    auto col_scale = gko::array<value_type>(this->exec, {1, 2, 1, 2, 2, 3});
    auto row_scale = gko::array<value_type>(this->exec, {2, 4, 3, 1});

    gko::batch::matrix::two_sided_scale<value_type, index_type>(
        col_scale, row_scale, this->mtx_0.get());

    auto scaled_mtx_0 =
        gko::batch::initialize<BMtx>({{{2.0, -4.0, 0.0}, {-8.0, 16.0, 12.0}},
                                      {{6.0, -12.0, 0.0}, {2.0, -5.0, 12.0}}},
                                     this->exec, 5);
    GKO_ASSERT_BATCH_MTX_NEAR(this->mtx_0.get(), scaled_mtx_0.get(), 0.);
}


TYPED_TEST(Csr, ApplyFailsOnWrongNumberOfResultCols)
{
    using BMVec = typename TestFixture::BMVec;
    auto res = BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{2}});

    ASSERT_THROW(this->mtx_0->apply(this->b_0.get(), res.get()),
                 gko::DimensionMismatch);
}


TYPED_TEST(Csr, ApplyFailsOnWrongNumberOfResultRows)
{
    using BMVec = typename TestFixture::BMVec;
    auto res = BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{3}});

    ASSERT_THROW(this->mtx_0->apply(this->b_0.get(), res.get()),
                 gko::DimensionMismatch);
}


TYPED_TEST(Csr, ApplyFailsOnWrongInnerDimension)
{
    using BMVec = typename TestFixture::BMVec;
    auto res =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{2, 3}});

    ASSERT_THROW(this->mtx_0->apply(res.get(), this->x_0.get()),
                 gko::DimensionMismatch);
}


TYPED_TEST(Csr, AdvancedApplyFailsOnWrongInnerDimension)
{
    using BMVec = typename TestFixture::BMVec;
    auto res =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{2, 3}});
    auto alpha =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{1, 1}});
    auto beta =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{1, 1}});

    ASSERT_THROW(
        this->mtx_0->apply(alpha.get(), res.get(), beta.get(), this->x_0.get()),
        gko::DimensionMismatch);
}


TYPED_TEST(Csr, AdvancedApplyFailsOnWrongAlphaDimension)
{
    using BMVec = typename TestFixture::BMVec;
    auto res =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{3, 3}});
    auto alpha =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{2, 1}});
    auto beta =
        BMVec::create(this->exec, gko::batch_dim<2>{2, gko::dim<2>{1, 1}});

    ASSERT_THROW(
        this->mtx_0->apply(alpha.get(), res.get(), beta.get(), this->x_0.get()),
        gko::DimensionMismatch);
}
