/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/time/timer.h"
#include "saiga/util/random.h"
#include "saiga/vision/BlockRecursiveBATemplates.h"
#include "saiga/vision/Eigen_Compile_Checker.h"
#include "saiga/vision/MatrixScalar.h"
#include "saiga/vision/SparseHelper.h"
#include "saiga/vision/VisionIncludes.h"

#include "Eigen/Sparse"

using namespace Saiga;


void simpleSchurTest()
{
    // Solution of the following block-structured linear system
    //
    // | U   W |   | da |   | ea |
    // | Wt  V | * | db | = | eb |
    //
    // , where
    // U and V are diagonal matrices, and W is sparse.
    // V is assumed to be much larger then U.
    // If U is larger the schur complement should be computed in the other direction.

    // ======================== Parameters ========================

    // size of U
    int n = 6 * 50;
    // size of V
    int m = 3 * 1000;
    // maximum number of non-zero elements per row in W
    int maxElementsPerRow = 300;

    using Vector = Eigen::Matrix<double, -1, 1>;
    using Matrix = Eigen::Matrix<double, -1, -1>;

    // ======================== Initialize ========================

    // Diagonal matrices U and V
    // All elements are positive!
    Eigen::DiagonalMatrix<double, -1> U(n);
    Eigen::DiagonalMatrix<double, -1> V(m);
    for (int i = 0; i < n; ++i) U.diagonal()(i) = Random::sampleDouble(0.1, 10);
    for (int i = 0; i < m; ++i) V.diagonal()(i) = Random::sampleDouble(0.1, 10);



    // Right hand side of the linear system
    Vector ea(n);
    Vector eb(m);
    ea.setRandom();
    eb.setRandom();

    Eigen::SparseMatrix<double, Eigen::RowMajor> W(n, m);
    W.reserve(n * maxElementsPerRow);
    for (int i = 0; i < n; ++i)
    {
        auto v = Random::uniqueIndices(maxElementsPerRow, m);
        for (auto j : v)
        {
            W.insert(i, j) = Random::sampleDouble(-5, 5);
        }
    }

    // ========================================================================================================

    if (n < 10)  // compute dense solution only for small problems
    {
        SAIGA_BLOCK_TIMER();
        // dense solution
        Matrix A(n + m, n + m);
        A.block(0, 0, n, n) = U.toDenseMatrix();
        A.block(n, n, m, m) = V.toDenseMatrix();
        A.block(0, n, n, m) = W.toDense();
        A.block(n, 0, m, n) = W.toDense().transpose();

        Vector e(n + m);
        e.segment(0, n) = ea;
        e.segment(n, m) = eb;

        Vector delta;
        delta = A.ldlt().solve(e);

        cout << "dense " << (A * delta - e).norm() << endl;
    }

    {
        SAIGA_BLOCK_TIMER();
        // sparse solution
        Eigen::SparseMatrix<double> A(n + m, n + m);

        std::vector<Eigen::Triplet<double>> triplets;
        for (int i = 0; i < n; ++i) triplets.emplace_back(i, i, U.diagonal()(i));
        for (int i = 0; i < m; ++i) triplets.emplace_back(i + n, i + n, V.diagonal()(i));

        auto mtri  = to_triplets(W);
        auto mtrit = to_triplets(W.transpose().eval());

        // add offsets
        for (auto& tri : mtri) tri = Eigen::Triplet<double>(tri.row(), tri.col() + n, tri.value());
        for (auto& tri : mtrit) tri = Eigen::Triplet<double>(tri.row() + n, tri.col(), tri.value());

        triplets.insert(triplets.end(), mtri.begin(), mtri.end());
        triplets.insert(triplets.end(), mtrit.begin(), mtrit.end());

        A.setFromTriplets(triplets.begin(), triplets.end());

        Vector e(n + m);
        e.segment(0, n) = ea;
        e.segment(n, m) = eb;

        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
        solver.compute(A);
        Vector delta = solver.solve(e);

        //        cout << "sparse " << delta.transpose() << endl;
        cout << "sparse " << (A * delta - e).norm() << endl;
    }



    {
        SAIGA_BLOCK_TIMER();

        // Schur complement solution

        // Step 1
        // Invert V
        Eigen::DiagonalMatrix<double, -1> Vinv(m);
        for (int i = 0; i < m; ++i) Vinv.diagonal()(i) = 1.0 / V.diagonal()(i);

        // Step 2
        // Compute Y
        Eigen::SparseMatrix<double, Eigen::RowMajor> Y(n, m);
        Y = W * Vinv;


        // Step 3
        // Compute the Schur complement S
        // Not sure how good the sparse matrix mult is of eigen
        // maybe own implementation because the structure is well known before hand
        Eigen::SparseMatrix<double> S(n, n);
        S = -Y * W.transpose();
        //        cout << "S" << endl << S.toDense() << endl;
        S.diagonal() = U.diagonal() + S.diagonal();

        // Step 4
        // Compute the right hand side of the schur system ej
        // S * da = ej
        Vector ej(n);
        ej = ea - Y * eb;

        // Step 5
        // Solve the schur system for da
        Vector deltaA(n);
        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
        solver.compute(S);
        deltaA = solver.solve(ej);

        // Step 6
        // Substitute the solultion deltaA into the original system and
        // bring it to the right hand side
        Vector q = eb - W.transpose() * deltaA;

        // Step 7
        // Solve the remaining partial system with the precomputed inverse of V
        Vector deltaB(m);
        deltaB = Vinv * q;

        cout << "sparse schur " << (U * deltaA + W * deltaB - ea).norm() << " "
             << (W.transpose() * deltaA + V * deltaB - eb).norm() << endl;
    }
    cout << endl;
}



void baBlockSchurTest()
{
    // Solution of the following block-structured linear system
    //
    // | U   W |   | da |   | ea |
    // | Wt  V | * | db | = | eb |
    //
    // , where
    // U and V are block diagonal matrices, and W is sparse.
    // V is assumed to be much larger then U.
    // If U is larger the schur complement should be computed in the other direction.

    // ======================== Parameters ========================



    // size of U
    int n = 2;
    // size of V
    int m = 3;

    // maximum number of non-zero elements per row in W
    int maxElementsPerRow = 3;


    // ======================== Variables ========================

    UType U(n);
    VType V(m);
    WType W(n, m);
    WTType WT(m, n);

    DAType da(n);
    DBType db(m);

    DAType ea(n);
    DBType eb(m);


    // ======================== Initialize U,V,W,ea,eb ========================

    Random::setSeed(239672031257UL);

    // Init U,V with random symmetric square matrices, but add a large value to the diagonal to ensure positive
    // definiteness and low condition number This is similar to the LM update

    double largeValue = 3;
    for (int i = 0; i < n; ++i)
    {
        ADiag a         = largeValue * ADiag::Identity() + ADiag::Random();
        U.diagonal()(i) = a.selfadjointView<Eigen::Upper>();
    }
    for (int i = 0; i < m; ++i)
    {
        BDiag a         = largeValue * BDiag::Identity() + BDiag::Random();
        V.diagonal()(i) = a.selfadjointView<Eigen::Upper>();
    }

#ifdef PRINT_MATRIX
    // debug print U and V
    cout << "U" << endl << blockDiagonalToMatrix(U) << endl;
    cout << "V" << endl << blockDiagonalToMatrix(V) << endl;
#endif


    // Init W with randoms blocks
    W.reserve(n * maxElementsPerRow);
    WT.reserve(n * maxElementsPerRow);

    std::vector<Eigen::Triplet<WElem>> ws1;
    std::vector<Eigen::Triplet<WTElem>> ws2;

    for (int i = 0; i < n; ++i)
    {
        auto v = Random::uniqueIndices(maxElementsPerRow, m);
        for (auto j : v)
        {
            WElem m = WElem::Random();
            ws1.emplace_back(i, j, m);
            ws2.emplace_back(j, i, m.transpose());
        }
    }

    W.setFromTriplets(ws1.begin(), ws1.end());
    WT.setFromTriplets(ws2.begin(), ws2.end());

#ifdef PRINT_MATRIX
    // debug print W
    cout << "W" << endl << blockMatrixToMatrix(W.toDense()) << endl;
    cout << endl;
#endif

    // Init ea and eb random
    for (int i = 0; i < n; ++i) ea(i) = ARes::Random();
    for (int i = 0; i < m; ++i) eb(i) = BRes::Random();


#if 1
    // ======================== Dense Simple Solution (only for checking the correctness) ========================
    {
        SAIGA_BLOCK_TIMER();
        n *= asize;
        m *= bsize;

        // Build the complete system matrix
        Eigen::MatrixXd M(m + n, m + n);
        M.block(0, 0, n, n) = blockDiagonalToMatrix(U);
        M.block(n, n, m, m) = blockDiagonalToMatrix(V);
        M.block(0, n, n, m) = blockMatrixToMatrix(W.toDense());
        M.block(n, 0, m, n) = blockMatrixToMatrix(W.toDense()).transpose();

        // stack right hand side
        Eigen::VectorXd b(m + n);
        b.segment(0, n) = blockVectorToVector(ea);
        b.segment(n, m) = blockVectorToVector(eb);

        // compute solution
        Eigen::VectorXd x = M.ldlt().solve(b);

        double error = (M * x - b).norm();
        cout << x.transpose() << endl;
        cout << "Dense error " << error << endl;

        n /= asize;
        m /= bsize;
    }
#endif


    // tmp variables
    VType Vinv(m);
    WType Y(n, m);
    SType S(n, n);
    DAType ej(n);

    {
        SAIGA_BLOCK_TIMER();
        // Schur complement solution

        // Step 1
        // Invert V
        for (int i = 0; i < m; ++i) Vinv.diagonal()(i) = V.diagonal()(i).get().inverse();
        //        cout << "Vinv" << endl << blockMatrixToMatrix(Vinv.toDenseMatrix()) << endl;
    }

    {
        SAIGA_BLOCK_TIMER();
        // Step 2
        // Compute Y
        Y = multSparseDiag(W, Vinv);
        //        cout << "Yref" << endl
        //             << (blockMatrixToMatrix(W.toDense()) * blockMatrixToMatrix(Vinv.toDenseMatrix())) << endl;
        //        cout << "Y" << endl << blockMatrixToMatrix(Y.toDense()) << endl;
    }
    {
        SAIGA_BLOCK_TIMER();
        // Step 3
        // Compute the Schur complement S
        // Not sure how good the sparse matrix mult is of eigen
        // maybe own implementation because the structure is well known before hand
        S = -(Y * WT).eval();
        //        S = W * WT;
        S.diagonal() = U.diagonal() + S.diagonal();

        //        cout << "Sref" << endl
        //             << (blockMatrixToMatrix(U.toDenseMatrix()) - blockMatrixToMatrix(W.toDense()) *
        //                                                              blockMatrixToMatrix(Vinv.toDenseMatrix()) *
        //                                                              blockMatrixToMatrix(WT.toDense()))
        //                    .eval()
        //             << endl;
        //        cout << "S" << endl << blockMatrixToMatrix(S.toDense()) << endl;
    }
    {
        SAIGA_BLOCK_TIMER();
        // Step 4
        // Compute the right hand side of the schur system ej
        // S * da = ej
        ej = ea + -(Y * eb);  // todo fix -
                              //        cout << "ejref" << endl
        //             << (blockVectorToVector(ea) - blockMatrixToMatrix(Y.toDense()) * blockVectorToVector(eb)) <<
        //             endl;

        //        cout << "ej" << endl << blockVectorToVector(ej) << endl;
    }


    Eigen::SparseMatrix<double> ssparse(n * asize, n * asize);
    {
        SAIGA_BLOCK_TIMER();
        // Step 5
        // Solve the schur system for da

        auto triplets = sparseBlockToTriplets(S);

        ssparse.setFromTriplets(triplets.begin(), triplets.end());
    }
    {
        SAIGA_BLOCK_TIMER();
        //        cout << "ssparse" << endl << ssparse.toDense() << endl;

        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
        //        Eigen::SimplicialLDLT<SType> solver;
        solver.compute(ssparse);
        Eigen::Matrix<double, -1, 1> deltaA = solver.solve(blockVectorToVector(ej));

        //        cout << "deltaA" << endl << deltaA << endl;

        // copy back into da
        for (int i = 0; i < n; ++i)
        {
            da(i) = deltaA.segment(i * asize, asize);
        }
    }
    DBType q;
    {
        SAIGA_BLOCK_TIMER();
        // Step 6
        // Substitute the solultion deltaA into the original system and
        // bring it to the right hand side
        q = eb + -WT * da;
        //        cout << "qref" << endl
        //             << (blockVectorToVector(eb) - blockMatrixToMatrix(WT.toDense()) * blockVectorToVector(da)) <<
        //             endl;

        //        cout << "q" << endl << blockVectorToVector(q) << endl;
    }
    {
        SAIGA_BLOCK_TIMER();
        // Step 7
        // Solve the remaining partial system with the precomputed inverse of V
        db = multDiagVector(Vinv, q);


#if 1
        // compute residual of linear system
        auto xa                           = blockVectorToVector(da);
        auto xb                           = blockVectorToVector(db);
        Eigen::Matrix<double, -1, 1> res1 = blockMatrixToMatrix(U.toDenseMatrix()) * xa +
                                            blockMatrixToMatrix(W.toDense()) * xb - blockVectorToVector(ea);
        Eigen::Matrix<double, -1, 1> res2 = blockMatrixToMatrix(WT.toDense()) * xa +
                                            blockMatrixToMatrix(V.toDenseMatrix()) * xb - blockVectorToVector(eb);
        cout << "Error: " << res1.norm() << " " << res2.norm() << endl;
#endif

        //        cout << "da" << endl << blockVectorToVector(da).transpose() << endl;
        //        cout << "db" << endl << blockVectorToVector(db).transpose() << endl;
    }
}

int main(int argc, char* argv[])
{
    Saiga::EigenHelper::checkEigenCompabitilty<2765>();


    simpleSchurTest();
    baBlockSchurTest();
    return 0;
}
