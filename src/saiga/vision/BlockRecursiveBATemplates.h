/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/vision/MatrixScalar.h"
#include "saiga/vision/VisionIncludes.h"
#include "saiga/vision/recursiveMatrices/Expand.h"
#include "saiga/vision/recursiveMatrices/Transpose.h"

#include "Eigen/Sparse"

namespace Saiga
{
// ======================== Types ========================

// Block size
const int asize = 6;
const int bsize = 3;

using T = double;

// block types
using ADiag  = Eigen::Matrix<T, asize, asize>;
using BDiag  = Eigen::Matrix<T, bsize, bsize>;
using WElem  = Eigen::Matrix<T, asize, bsize>;
using WTElem = Eigen::Matrix<T, bsize, asize>;
using ARes   = Eigen::Matrix<T, asize, 1>;
using BRes   = Eigen::Matrix<T, bsize, 1>;

// Block structured diagonal matrices
using UType = Eigen::DiagonalMatrix<MatrixScalar<ADiag>, -1>;
using VType = Eigen::DiagonalMatrix<MatrixScalar<BDiag>, -1>;

// Block structured vectors
using DAType = Eigen::Matrix<MatrixScalar<ARes>, -1, 1>;
using DBType = Eigen::Matrix<MatrixScalar<BRes>, -1, 1>;

// Block structured sparse matrix
using WType  = Eigen::SparseMatrix<MatrixScalar<WElem>, Eigen::ColMajor>;
using WTType = Eigen::SparseMatrix<MatrixScalar<WTElem>, Eigen::ColMajor>;
using SType  = Eigen::SparseMatrix<MatrixScalar<ADiag>, Eigen::RowMajor>;



}  // namespace Saiga

namespace Eigen
{
// ======================== Eigen Magic ========================

template <typename BinaryOp>
struct ScalarBinaryOpTraits<Saiga::MatrixScalar<Saiga::WElem>, Saiga::MatrixScalar<Saiga::WTElem>, BinaryOp>
{
    typedef Saiga::MatrixScalar<Saiga::ADiag> ReturnType;
};

template <typename BinaryOp>
struct ScalarBinaryOpTraits<Saiga::MatrixScalar<Saiga::WElem>, Saiga::MatrixScalar<Saiga::BDiag>, BinaryOp>
{
    typedef Saiga::MatrixScalar<Saiga::WElem> ReturnType;
};



template <typename BinaryOp>
struct ScalarBinaryOpTraits<Saiga::MatrixScalar<Saiga::WElem>, Saiga::MatrixScalar<Saiga::BRes>, BinaryOp>
{
    typedef Saiga::MatrixScalar<Saiga::ARes> ReturnType;
};


template <typename SparseLhsType, typename DenseRhsType, typename DenseResType>
struct internal::sparse_time_dense_product_impl<SparseLhsType, DenseRhsType, DenseResType,
                                                Saiga::MatrixScalar<Saiga::ARes>, Eigen::ColMajor, true>
{
    typedef typename internal::remove_all<SparseLhsType>::type Lhs;
    typedef typename internal::remove_all<DenseRhsType>::type Rhs;
    typedef typename internal::remove_all<DenseResType>::type Res;
    typedef typename evaluator<Lhs>::InnerIterator LhsInnerIterator;
    using AlphaType = Saiga::MatrixScalar<Saiga::ARes>;
    static void run(const SparseLhsType& lhs, const DenseRhsType& rhs, DenseResType& res, const AlphaType& alpha)
    {
        evaluator<Lhs> lhsEval(lhs);
        for (Index c = 0; c < rhs.cols(); ++c)
        {
            for (Index j = 0; j < lhs.outerSize(); ++j)
            {
                for (LhsInnerIterator it(lhsEval, j); it; ++it)
                {
                    res.coeffRef(it.index(), c) += (it.value() * rhs.coeff(j, c));
                }
            }
        }
    }
};

template <typename BinaryOp>
struct ScalarBinaryOpTraits<Saiga::MatrixScalar<Saiga::WTElem>, Saiga::MatrixScalar<Saiga::ARes>, BinaryOp>
{
    typedef Saiga::MatrixScalar<Saiga::BRes> ReturnType;
};

template <typename SparseLhsType, typename DenseRhsType, typename DenseResType>
struct internal::sparse_time_dense_product_impl<SparseLhsType, DenseRhsType, DenseResType,
                                                Saiga::MatrixScalar<Saiga::BRes>, Eigen::ColMajor, true>
{
    typedef typename internal::remove_all<SparseLhsType>::type Lhs;
    typedef typename internal::remove_all<DenseRhsType>::type Rhs;
    typedef typename internal::remove_all<DenseResType>::type Res;
    typedef typename evaluator<Lhs>::InnerIterator LhsInnerIterator;
    using AlphaType = Saiga::MatrixScalar<Saiga::BRes>;
    static void run(const SparseLhsType& lhs, const DenseRhsType& rhs, DenseResType& res, const AlphaType& alpha)
    {
        evaluator<Lhs> lhsEval(lhs);
        for (Index c = 0; c < rhs.cols(); ++c)
        {
            for (Index j = 0; j < lhs.outerSize(); ++j)
            {
                for (LhsInnerIterator it(lhsEval, j); it; ++it)
                {
                    res.coeffRef(it.index(), c) += (it.value() * rhs.coeff(j, c));
                }
            }
        }
    }
};

}  // namespace Eigen


// Computes R = M * D  with
// M : Sparse Matrix in either row or column major format
// D : Diagonal (dense) matrix
// R : Result same format and sparsity pattern as M
template <typename S, typename DiagType>
S multSparseDiag(const S& M, const DiagType& D)
{
    SAIGA_ASSERT(M.cols() == D.rows());

    S result(M.rows(), M.cols());
    result.reserve(M.nonZeros());
    result.markAsRValue();

    // Copy the structure
    for (int k = 0; k < M.outerSize() + 1; ++k)
    {
        result.outerIndexPtr()[k] = M.outerIndexPtr()[k];
    }
    for (int k = 0; k < M.nonZeros(); ++k)
    {
        result.innerIndexPtr()[k] = M.innerIndexPtr()[k];
    }

    // Copmpute result
    for (int k = 0; k < M.outerSize(); ++k)
    {
        typename S::InnerIterator itM(M, k);
        typename S::InnerIterator itRes(result, k);

        for (; itM; ++itM, ++itRes)
        {
            itRes.valueRef() = itM.value() * D.diagonal()(itM.col());
        }
    }

    return result;
}

template <typename Diag, typename Vec>
Vec multDiagVector(const Diag& D, const Vec& v)
{
    SAIGA_ASSERT(D.cols() == v.rows());

    Vec result;
    result.resize(v.rows(), v.cols());
    //    Vec result = v;

    for (int k = 0; k < D.rows(); ++k)
    {
        result(k) = D.diagonal()(k) * v(k);
    }

    return result;
}


template <typename Diag, typename Vec>
Vec multDiagVectorMulti(const Diag& D, const Vec& v)
{
    SAIGA_ASSERT(D.cols() == v.rows());

    Vec result;
    result.resize(v.rows(), v.cols());
    //    Vec result = v;

    for (int k = 0; k < D.rows(); ++k)
    {
        result.row(k) = D.diagonal()(k) * v.row(k);
    }

    return result;
}


// ==========================================================================================


namespace Eigen
{
namespace internal
{
template <typename Lhs, typename Rhs, typename ResultType>
static void aconservative_sparse_sparse_product_impl(const Lhs& lhs, const Rhs& rhs, ResultType& res,
                                                     bool sortedInsertion = false)
{
    typedef typename remove_all<Lhs>::type::Scalar LhsScalar;
    typedef typename remove_all<Rhs>::type::Scalar RhsScalar;
    typedef typename remove_all<ResultType>::type::Scalar ResScalar;

    // make sure to call innerSize/outerSize since we fake the storage order.
    Index rows = lhs.innerSize();
    Index cols = rhs.outerSize();
    eigen_assert(lhs.outerSize() == rhs.innerSize());

    ei_declare_aligned_stack_constructed_variable(bool, mask, rows, 0);
    ei_declare_aligned_stack_constructed_variable(ResScalar, values, rows, 0);
    ei_declare_aligned_stack_constructed_variable(Index, indices, rows, 0);

    std::memset(mask, 0, sizeof(bool) * rows);

    evaluator<Lhs> lhsEval(lhs);
    evaluator<Rhs> rhsEval(rhs);

    // estimate the number of non zero entries
    // given a rhs column containing Y non zeros, we assume that the respective Y columns
    // of the lhs differs in average of one non zeros, thus the number of non zeros for
    // the product of a rhs column with the lhs is X+Y where X is the average number of non zero
    // per column of the lhs.
    // Therefore, we have nnz(lhs*rhs) = nnz(lhs) + nnz(rhs)
    Index estimated_nnz_prod = lhsEval.nonZerosEstimate() + rhsEval.nonZerosEstimate();

    res.setZero();
    res.reserve(Index(estimated_nnz_prod));
    // we compute each column of the result, one after the other
    for (Index j = 0; j < cols; ++j)
    {
        res.startVec(j);
        Index nnz = 0;
        for (typename evaluator<Rhs>::InnerIterator rhsIt(rhsEval, j); rhsIt; ++rhsIt)
        {
            RhsScalar y = rhsIt.value();
            Index k     = rhsIt.index();
            for (typename evaluator<Lhs>::InnerIterator lhsIt(lhsEval, k); lhsIt; ++lhsIt)
            {
                Index i     = lhsIt.index();
                LhsScalar x = lhsIt.value();
                if (!mask[i])
                {
                    mask[i]      = true;
                    values[i]    = y * x;
                    indices[nnz] = i;
                    ++nnz;
                }
                else
                    values[i] += y * x;
            }
        }
        if (!sortedInsertion)
        {
            // unordered insertion
            for (Index k = 0; k < nnz; ++k)
            {
                Index i                                   = indices[k];
                res.insertBackByOuterInnerUnordered(j, i) = values[i];
                mask[i]                                   = false;
            }
        }
        else
        {
            // alternative ordered insertion code:
            const Index t200 = rows / 11;  // 11 == (log2(200)*1.39)
            const Index t    = (rows * 100) / 139;

            // FIXME reserve nnz non zeros
            // FIXME implement faster sorting algorithms for very small nnz
            // if the result is sparse enough => use a quick sort
            // otherwise => loop through the entire vector
            // In order to avoid to perform an expensive log2 when the
            // result is clearly very sparse we use a linear bound up to 200.
            if ((nnz < 200 && nnz < t200) || nnz * numext::log2(int(nnz)) < t)
            {
                if (nnz > 1) std::sort(indices, indices + nnz);
                for (Index k = 0; k < nnz; ++k)
                {
                    Index i                          = indices[k];
                    res.insertBackByOuterInner(j, i) = values[i];
                    mask[i]                          = false;
                }
            }
            else
            {
                // dense path
                for (Index i = 0; i < rows; ++i)
                {
                    if (mask[i])
                    {
                        mask[i]                          = false;
                        res.insertBackByOuterInner(j, i) = values[i];
                    }
                }
            }
        }
    }
    res.finalize();
}

template <typename Rhs, typename ResultType>
struct conservative_sparse_sparse_product_selector<Saiga::WType, Rhs, ResultType, RowMajor, RowMajor, ColMajor>
{
    using Lhs = Saiga::WType;
    static void run(const Lhs& lhs, const Rhs& rhs, ResultType& res)
    {
        typedef SparseMatrix<typename ResultType::Scalar, RowMajor, typename ResultType::StorageIndex> RowMajorMatrix;
        RowMajorMatrix resRow(lhs.rows(), rhs.cols());
        internal::aconservative_sparse_sparse_product_impl<Rhs, Lhs, RowMajorMatrix>(rhs, lhs, resRow);
        res = resRow;
    }
};



template <typename SparseLhsType, typename DenseRhsType>
struct sparse_time_dense_product_impl<SparseLhsType, DenseRhsType, Saiga::DBType, typename Saiga::DBType::Scalar,
                                      RowMajor, true>
{
    using DenseResType = Saiga::DBType;
    typedef typename internal::remove_all<SparseLhsType>::type Lhs;
    typedef typename internal::remove_all<DenseRhsType>::type Rhs;
    typedef typename internal::remove_all<DenseResType>::type Res;
    typedef typename evaluator<Lhs>::InnerIterator LhsInnerIterator;
    typedef evaluator<Lhs> LhsEval;
    static void run(const SparseLhsType& lhs, const DenseRhsType& rhs, DenseResType& res,
                    const typename Res::Scalar& alpha)
    {
        LhsEval lhsEval(lhs);

        Index n = lhs.outerSize();
#ifdef EIGEN_HAS_OPENMP
        Eigen::initParallel();
        Index threads = Eigen::nbThreads();
#endif

        for (Index c = 0; c < rhs.cols(); ++c)
        {
#ifdef EIGEN_HAS_OPENMP
            // This 20000 threshold has been found experimentally on 2D and 3D Poisson problems.
            // It basically represents the minimal amount of work to be done to be worth it.
            if (threads > 1 && lhsEval.nonZerosEstimate() > 20000)
            {
#    pragma omp parallel for schedule(dynamic, (n + threads * 4 - 1) / (threads * 4)) num_threads(threads)
                for (Index i = 0; i < n; ++i) processRow(lhsEval, rhs, res, alpha, i, c);
            }
            else
#endif
            {
                for (Index i = 0; i < n; ++i) processRow(lhsEval, rhs, res, alpha, i, c);
            }
        }
    }

    static void processRow(const LhsEval& lhsEval, const DenseRhsType& rhs, DenseResType& res,
                           const typename Res::Scalar& alpha, Index i, Index col)
    {
        typename Res::Scalar tmp(0);
        for (LhsInnerIterator it(lhsEval, i); it; ++it) tmp += it.value() * rhs.coeff(it.index(), col);
        res.coeffRef(i, col) += tmp;
    }
};


template <typename SparseLhsType, typename DenseRhsType>
struct sparse_time_dense_product_impl<SparseLhsType, DenseRhsType, Saiga::DAType, typename Saiga::DAType::Scalar,
                                      RowMajor, true>
{
    using DenseResType = Saiga::DAType;
    typedef typename internal::remove_all<SparseLhsType>::type Lhs;
    typedef typename internal::remove_all<DenseRhsType>::type Rhs;
    typedef typename internal::remove_all<DenseResType>::type Res;
    typedef typename evaluator<Lhs>::InnerIterator LhsInnerIterator;
    typedef evaluator<Lhs> LhsEval;
    static void run(const SparseLhsType& lhs, const DenseRhsType& rhs, DenseResType& res,
                    const typename Res::Scalar& alpha)
    {
        LhsEval lhsEval(lhs);

        Index n = lhs.outerSize();
#ifdef EIGEN_HAS_OPENMP
        Eigen::initParallel();
        Index threads = Eigen::nbThreads();
#endif

        for (Index c = 0; c < rhs.cols(); ++c)
        {
#ifdef EIGEN_HAS_OPENMP
            // This 20000 threshold has been found experimentally on 2D and 3D Poisson problems.
            // It basically represents the minimal amount of work to be done to be worth it.
            if (threads > 1 && lhsEval.nonZerosEstimate() > 20000)
            {
#    pragma omp parallel for schedule(dynamic, (n + threads * 4 - 1) / (threads * 4)) num_threads(threads)
                for (Index i = 0; i < n; ++i) processRow(lhsEval, rhs, res, alpha, i, c);
            }
            else
#endif
            {
                for (Index i = 0; i < n; ++i) processRow(lhsEval, rhs, res, alpha, i, c);
            }
        }
    }

    static void processRow(const LhsEval& lhsEval, const DenseRhsType& rhs, DenseResType& res,
                           const typename Res::Scalar& alpha, Index i, Index col)
    {
        typename Res::Scalar tmp(0);
        for (LhsInnerIterator it(lhsEval, i); it; ++it) tmp += it.value() * rhs.coeff(it.index(), col);
        res.coeffRef(i, col) += tmp;
    }
};

}  // namespace internal

}  // namespace Eigen
