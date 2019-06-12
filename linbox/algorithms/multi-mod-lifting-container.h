/*
 * Copyright(C) LinBox
 *
 * ========LICENCE========
 * This file is part of the library LinBox.
 *
 * LinBox is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

#pragma once

#include <linbox/algorithms/rns.h>
#include <linbox/solutions/methods.h>

namespace LinBox {
    /**
     * The algorithm find out the p-adic writing of A^{-1} * b.
     * So that A^{-1} * b = c0 + c1 * p + c2 * p^2 + ... + c{k-1} * p^{k-1}.
     * The chosen p is multi-modular.
     *
     * It is based on Chen/Storjohann RNS-based p-adic lifting.
     * Based on https://cs.uwaterloo.ca/~astorjoh/p92-chen.pdf
     * A BLAS Based C Library for Exact Linear Algebra on Integer Matrices (ISSAC 2009)
     * But it has been slightly modified in order to use BLAS3 multiplication within the main loop.
     *
     *  RNS Dixon algorithm goes this way:
     *      (i)     Use (p1, ..., pl) primes with an arbitrary l.
     *      (ii)    Algorithm goes:
     *                  for i = 1 .. l:
     *                  |   Bi = A^{-1} mod pi                      < Pre-computing
     *                  [r1|...|rl] = [b|...|b]
     *                  [y1|...|yl] = [0|...|0]
     *                  for j = 1 .. k:
     *                  |   for i = 1 .. l:
     *                  |   |   (Qi, Ri) = such that ri = pi Qi + Ri with |Ri| < pi
     *                  |   |   ci = Bi ri mod pi                   < Matrix-vector in Z/pZ
     *                  |   |   yi = yi + ci * pi^(i-1)             < Done over ZZ
     *                  |   V = [R1|...|Rl] - A [c1|...|cl]         < Matrix-matrix in ZZ
     *                  |   for i = 1 .. l:
     *                  |   |   ri = Qi + (Vi / pi)
     *              @note The computation of V can be done in a RNS system such that each RNS
     * base-prime is bigger than each (p1, ..., pl). This way, [R1|...|Rl] and [c1|...|cl] are
     * zero-cost to get in the RNS system.
     *      (iii)   y = CRT_Reconstruct(y1, ..., yl)
     *      (iv)    x = Rational_Reconstruct(y)
     *
     * One can configure how many primes are used with `Method::DixonRNS.primeBaseLength`.
     * According to the paper, a value of lp = 2 (ln(n) + log2(||A||)) or without the factor 2
     * can be used, but it depends on the problem, really.
     */
    template <class _Field, class _Ring, class _PrimeGenerator>
    class MultiModLiftingContainer final : public LiftingContainer<_Ring> {
        using BaseClass = LiftingContainer<_Ring>;

    public:
        using Ring = _Ring;
        using Field = _Field;
        using PrimeGenerator = _PrimeGenerator;

        using RNSSystem = FFPACK::rns_double;
        using RNSDomain = FFPACK::RNSInteger<FFPACK::rns_double>;
        using RNSElement = typename RNSDomain::Element;
        using RNSElementPtr = typename RNSDomain::Element_ptr;

        using IElement = typename Ring::Element;
        using IMatrix = DenseMatrix<_Ring>;
        using IVector = DenseVector<_Ring>;
        using FElement = typename Field::Element;
        using FMatrix = DenseMatrix<_Field>;
        using FVector = DenseVector<_Field>;

    public:
        // -------------------
        // ----- Main behavior

        // @fixme Split to inline file
        MultiModLiftingContainer(const Ring& ring, PrimeGenerator primeGenerator, const IMatrix& A,
                                 const IVector& b, const Method::DixonRNS& m)
            : _ring(ring)
            , _A(A)
            , _b(b)
            , _n(A.rowdim())
        {
            linbox_check(A.rowdim() == A.coldim());

            A.write(std::cout << "A: ", Tag::FileFormat::Maple) << std::endl;
            std::cout << "b: " << b << std::endl;

            // This will contain the primes or our MultiMod basis
            // @fixme Pass it through Method::DixonRNS (and rename it Method::DixonMultiMod?)
            _primesCount = 2;
            _primes.resize(_primesCount);
            std::cout << "primesCount: " << _primesCount << std::endl;

            // Some preparation work
            Integer infinityNormA;
            InfinityNorm(infinityNormA, A);
            double logInfinityNormA = Givaro::logtwo(infinityNormA);

            {
                // Based on Chen-Storjohann's paper, this is the bit size
                // of the needed RNS basis for the residue computation
                double rnsBasisBitSize = (logInfinityNormA + Givaro::logtwo(_n));
                _rnsPrimesCount = std::ceil(rnsBasisBitSize / primeGenerator.getBits());
                _rnsPrimes.resize(_rnsPrimesCount);
                std::cout << "rnsBasisPrimesCount: " << _rnsPrimesCount << std::endl;

                std::vector<double> primes;
                for (auto j = 0u; j < _primesCount + _rnsPrimesCount; ++j) {
                    auto p = *primeGenerator;
                    ++primeGenerator;

                    // @note std::lower_bound finds the iterator where to put p in the sorted
                    // container. The name of the routine might be strange, but, hey, that's not my
                    // fault.
                    auto lb = std::lower_bound(primes.begin(), primes.end(), p);
                    if (lb != primes.end() && *lb == p) {
                        --j;
                        continue;
                    }

                    // Inserting the primes at the right place to keep the array sorted
                    primes.insert(lb, p);
                }

                // We take the smallest primes for our MultiMod basis
                std::copy(primes.begin(), primes.begin() + _primesCount, _primes.begin());

                // And the others for our RNS basis
                std::copy(primes.begin() + _primesCount, primes.end(), _rnsPrimes.begin());

                // We check that we really need all the primes within the RNS basis,
                // as the first count was just an upper estimation.
                double bitSize = 0.0;
                for (int h = _rnsPrimes.size() - 1; h >= 0; --h) {
                    bitSize += Givaro::logtwo(primes[h]);

                    if (bitSize > rnsBasisBitSize && h > 0) {
                        _rnsPrimes.erase(_rnsPrimes.begin(), _rnsPrimes.begin() + (h - 1));
                        _rnsPrimesCount -= h;
                        std::cout << "RNS basis: Erasing extra " << h << "primes." << std::endl;
                        break;
                    }
                }
            }

            // Generating primes
            // @fixme Cleanup, might not be needed
            {
                IElement iTmp;
                _ring.assign(_primesProduct, _ring.one);
                for (auto& pj : _primes) {
                    _fields.emplace_back(pj);
                    _ring.init(iTmp, pj);
                    _ring.mulin(_primesProduct, iTmp);
                }

                std::cout << "primesProduct: " << _primesProduct << std::endl;
            }

            // Initialize all inverses
            // @note An inverse mod some p within DixonSolver<Dense> was already computed,
            // and pass through to the lifting container. Here, we could use that, but we have
            // to keep control of generated primes, so that the RNS base has bigger primes
            // than the .
            {
                _B.reserve(_primesCount);

                for (const auto& F : _fields) {
                    _B.emplace_back(A, F); // Rebind into the field

                    int nullity = 0;
                    BlasMatrixDomain<Field> bmd(F);
                    bmd.invin(_B.back(), nullity);
                    if (nullity > 0) {
                        // @fixme Should redraw another prime!
                        throw LinBoxError("Wrong prime, sorry.");
                    }
                }
            }

            // Making A into the RNS domain
            {
                _rnsSystem = new RNSSystem(_rnsPrimes);
                _rnsDomain = new RNSDomain(*_rnsSystem);
                _rnsA = FFLAS::fflas_new(*_rnsDomain, _n, _n);
                _rnsc = FFLAS::fflas_new(*_rnsDomain, _n, _primesCount);
                _rnsR = FFLAS::fflas_new(*_rnsDomain, _n, _primesCount);

                // @note So that 2^(16*cmax) is the max element of A.
                double cmax = logInfinityNormA / 16.;
                FFLAS::finit_rns(*_rnsDomain, _n, _n, std::ceil(cmax), A.getPointer(), A.stride(),
                                 _rnsA);
            }

            // Compute the inverses of pj for each RNS prime
            {
                _primesRNSInverses.resize(_primesCount);
                for (auto j = 0u; j < _primesCount; ++j) {
                    auto prime = _primes[j];
                    _primesRNSInverses[j].resize(_rnsPrimesCount);
                    for (auto h = 0u; h < _rnsPrimesCount; ++h) {
                        auto& rnsF = _rnsSystem->_field_rns[h];
                        auto& primeInverse = _primesRNSInverses[j][h];
                        rnsF.inv(primeInverse, prime);
                    }
                }
            }

            // Compute how many iterations are needed
            {
                auto hb = RationalSolveHadamardBound(A, b);
                double log2P = Givaro::logtwo(_primesProduct);
                // _iterationsCount = log2(2 * N * D) / log2(p)
                _log2Bound = hb.solutionLogBound;
                _iterationsCount = std::ceil(_log2Bound / log2P);
                std::cout << "iterationsCount: " << _iterationsCount << std::endl;

                // @fixme Fact is RationalReconstruction which needs numbound and denbound
                // expects them to be in non-log... @fixme Still needed?
                _ring.init(_numbound, Integer(1)
                                          << static_cast<uint64_t>(std::ceil(hb.numLogBound)));
                _ring.init(_denbound, Integer(1)
                                          << static_cast<uint64_t>(std::ceil(hb.denLogBound)));
            }

            //----- Locals setup

            _r.reserve(_primesCount);
            _Q.reserve(_primesCount);
            _R.reserve(_primesCount);
            _Fc.reserve(_primesCount);
            for (auto j = 0u; j < _primesCount; ++j) {
                auto& F = _fields[j];

                _r.emplace_back(_ring, _n);
                _Q.emplace_back(_ring, _n);
                _R.emplace_back(_ring, _n);
                _Fc.emplace_back(F, _n);

                // Initialize all residues to b
                _r.back() = _b; // Copying data
            }
        }

        ~MultiModLiftingContainer()
        {
            FFLAS::fflas_delete(_rnsR); // @fixme Does it knows the size?
            FFLAS::fflas_delete(_rnsc); // @fixme Does it knows the size?
            FFLAS::fflas_delete(_rnsA); // @fixme Does it knows the size?
            delete _rnsDomain;
            delete _rnsSystem;
        }

        // --------------------------
        // ----- LiftingContainer API

        const Ring& ring() const final { return _ring; }

        /// The length of the container.
        size_t length() const final { return _iterationsCount; }

        /// The dimension of the problem/solution.
        size_t size() const final { return _n; }

        /**
         * We are compliant to the interface even though
         * p is multi-modular and thus not a prime per se.
         */
        const IElement& prime() const final { return _primesProduct; }

        // ------------------------------
        // ----- NOT LiftingContainer API
        // ----- but still needed

        const IElement& numbound() const { return _numbound; }

        const IElement& denbound() const { return _denbound; }

        double log2Bound() const { return _log2Bound; }

        uint32_t primesCount() const { return _primesCount; }

        const FElement& prime(uint32_t index) const { return _primes.at(index); }

        // --------------
        // ----- Iterator

        /**
         * Returns false if the next digit cannot be computed (bad modulus).
         * c is a vector of integers but all element are below p = p1 * ... * pl
         */
        bool next(std::vector<IVector>& digits)
        {
            VectorDomain<Ring> IVD(_ring);

            // @fixme Should be done in parallel!
            for (auto j = 0u; j < _primesCount; ++j) {
                auto pj = _primes[j];
                auto& r = _r[j];
                auto& Q = _Q[j];
                auto& R = _R[j];

                // @note There is no VectorDomain::divmod yet.
                // Euclidian division so that rj = pj Qj + Rj
                for (auto i = 0u; i < _n; ++i) {
                    // @fixme @cpernet Is this OK for any Ring or should we be sure we are using
                    // Integers?
                    _ring.quoRem(Q[i], R[i], r[i], pj);
                }

                // Convert R to the field
                // @fixme @cpernet Could this step be ignored?
                // If not, put that in already allocated memory, and not use a temporary here.
                auto& F = _fields[j];
                FVector FR(F, R); // rebind

                auto& B = _B[j];
                auto& Fc = _Fc[j];
                B.apply(Fc, FR);

                // @fixme We might not need to store digits into IVectors, and returning _Fc
                // would do the trick
                digits[j] = IVector(_ring, Fc);

                // Store the very same result in an RNS system,
                // but fact is all the primes of the RNS system are bigger
                // than the modulus used to compute _Fc, we just copy the result for everybody.
                for (auto i = 0u; i < _n; ++i) {
                    setRNSMatrixElementAllResidues(_rnsR, _n, i, j, FR[i]);
                    setRNSMatrixElementAllResidues(_rnsc, _n, i, j, Fc[i]);
                }
            }

            // ----- Compute the next residues!

            // r <= Q + (R - A c) / p

            std::cout << "A" << std::endl;
            for (auto j = 0u; j < _n; ++j) {
                for (auto i = 0u; i < _n; ++i) {
                    logRNSMatrixElement(_rnsA, _n, i, j);
                }
            }

            std::cout << "c" << std::endl;
            for (auto j = 0u; j < _primesCount; ++j) {
                for (auto i = 0u; i < _n; ++i) {
                    logRNSMatrixElement(_rnsc, _n, i, j);
                }
            }

            // By first computing R <= R - A c as a fgemm within the RNS domain.
            FFLAS::fgemm(*_rnsDomain, FFLAS::FflasNoTrans, FFLAS::FflasNoTrans, _n, _primesCount, _n,
                         _rnsDomain->mOne, _rnsA, _n, _rnsc, _n, _rnsDomain->one,
                         _rnsR, _n);

            std::cout << "R = Ac" << std::endl;
            for (auto j = 0u; j < _primesCount; ++j) {
                for (auto i = 0u; i < _n; ++i) {
                    logRNSMatrixElement(_rnsR, _n, i, j);
                }
            }

            // We divide each residues by the according pj, which is done by multiplying.
            // @fixme Could be done in parallel!
            for (auto j = 0u; j < _primesCount; ++j) {
                for (auto i = 0u; i < _n; ++i) {
                    auto& rnsElement = _rnsR[i * _n + j];
                    auto stride = rnsElement._stride;
                    for (auto h = 0u; h < _rnsPrimesCount; ++h) {
                        auto& rnsF = _rnsSystem->_field_rns[h];
                        rnsF.mulin(rnsElement._ptr[h * stride], _primesRNSInverses[j][h]);
                    }
                }
            }

            // @fixme Could be done in parallel!
            for (auto j = 0u; j < _primesCount; ++j) {
                auto& r = _r[j];
                auto& Q = _Q[j];

                // r <- (R - Ac) / p
                // @fixme @cpernet Don't know how to do that with one fconvert_rns!
                for (auto i = 0u; i < _n; ++i) {
                    FFLAS::fconvert_rns(*_rnsDomain, 1, 1, 0, &r[i], 1, _rnsR + (i * _n + j));
                }

                // r <- Q + (R - Ac) / p
                IVD.addin(r, Q);
            }

            ++_position;
            return true;
        }

    private:
        // Helper function, setting all residues of a matrix element to the very same value.
        // This doesn't check the moduli.
        void setRNSMatrixElementAllResidues(RNSElementPtr& A, size_t lda, size_t i, size_t j,
                                            double value)
        {
            auto stride = A[i * lda + j]._stride;
            for (auto h = 0u; h < _rnsPrimesCount; ++h) {
                A[i * lda + j]._ptr[h * stride] = value;
            }
        }

        void logRNSMatrixElement(RNSElementPtr& A, size_t lda, size_t i, size_t j)
        {
            Integer reconstructedInteger;
            FFLAS::fconvert_rns(*_rnsDomain, 1, 1, 0, &reconstructedInteger, 1, A + (i * lda + j));
            std::cout << i << " " << j << " ";
            _rnsDomain->write(std::cout, A[i * lda + j]);
            std::cout << " -> " << reconstructedInteger << std::endl;
        }

    private:
        const Ring& _ring;

        // The problem: A^{-1} * b
        const IMatrix& _A;
        const IVector& _b;

        IElement _numbound;
        IElement _denbound;
        double _log2Bound;

        RNSSystem* _rnsSystem = nullptr;
        RNSDomain* _rnsDomain = nullptr;
        RNSElementPtr _rnsA; // The matrix A, but in the RNS system
        // A matrix of digits c[j], being the current digits mod pj, in the RNS system
        RNSElementPtr _rnsc;
        RNSElementPtr _rnsR;
        size_t _rnsPrimesCount = 0u;
        // Stores the inverse of pj of the i-th RNS prime into _primesRNSInverses[j][i]
        std::vector<std::vector<FElement>> _primesRNSInverses;

        IElement _primesProduct;       // The global modulus for lifting: a multiple of all _primes.
        std::vector<FElement> _primes; // @fixme We might want something else as a type!
        std::vector<double> _rnsPrimes;
        // Length of the ci sequence. So that p^{k-1} > 2ND (Hadamard bound).
        size_t _iterationsCount = 0u;
        size_t _n = 0u;           // Row/column dimension of A.
        size_t _primesCount = 0u; // How many primes. Equal to _primes.size().

        std::vector<FMatrix> _B;    // Inverses of A mod p[i]
        std::vector<Field> _fields; // All fields Modular<p[i]>

        //----- Iteration
        std::vector<IVector> _r; // @todo Could be a matrix? Might not be useful, as it is never
                                 // used directly in computations.
        std::vector<IVector> _Q;
        std::vector<IVector> _R; // @fixme This one should be expressed in a RNS system q, and
                                 // HAS TO BE A MATRIX for gemm.
        std::vector<FVector>
            _Fc; // @note No need to be a matrix, as we will embed it into an RNS system later.
        size_t _position;
    };
}
