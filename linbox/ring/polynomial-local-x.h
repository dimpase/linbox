/* Copyright (C) 2017 LinBox
 *
 * Written by Gavin Harrison, August 2017
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

#ifndef __LINBOX_polynomial_local_x_H
#define __LINBOX_polynomial_local_x_H

#include "linbox/linbox-config.h"
#include "linbox/util/debug.h"

#include <stdlib.h>
#include <algorithm>

// Namespace in which all LinBox code resides
namespace LinBox
{
	template<class Field>
	class PolynomialLocalX {
	public:
		typedef typename Field::Element Polynomial;
		typedef typename Field::CoeffField CoeffField;
		typedef typename Field::Coeff Coeff;
		
		// f(x) * x^e ; x does not divide f(x)
		typedef struct {
			size_t e; 
			Polynomial f;
		} Element;
		
		// typedef UnparametricRandIter<Element> RandIter;
		
		// typedef NTL_zz_p CoeffField;
		// typedef NTL::zz_p Coeff;

		const Field _F;
		const Element zero, one, mOne, X;
		size_t exp;

		PolynomialLocalX(const Field &F, size_t exponent) : _F(F),
			zero({0, _F.zero}), one({0, _F.one}), mOne({0, _F.mOne}),
			X({1, _F.one}), exp(exponent)
			{}
			
		PolynomialLocalX(const PolynomialLocalX &P) : _F(P._F), zero(P.zero),
			one(P.one), mOne(P.mOne), X(P.X), exp(P.exp) {}
			
		PolynomialLocalX(const PolynomialLocalX &P, size_t exponent) : _F(P.F),
			zero(P.zero), one(P.one), mOne(P.mOne), X(P.X), exp(exponent) {}
			
		void setExponent(size_t exponent) {
			exp = exponent;
		}
		
		size_t getExponent() const {
			return exp;
		}
			
		const CoeffField &getCoeffField() const {
			return _F.getCoeffField();
		}
		
		size_t firstNonZeroCoeff(Element &a) const {
			return a.e;
		}
		
		Element &normalize(Element &a, const Polynomial &b) const {
			if (_F.isZero(b)) {
				return a = zero;
			}
			
			size_t i = 0;
			for (; i <= _F.deg(b); i++) {
				Coeff coeff;
				if (!getCoeffField().isZero(_F.getCoeff(coeff, b, i))) {
					break;
				}
			}
			
			Polynomial tmp, x, xe;
			std::vector<integer> v;
			_F.init(x, v = {0, 1});
			_F.pow(xe, x, i);
			_F.div(tmp, b, xe);
			
			return a = {
				i,
				tmp
			};
		}
			
		Element &normalize(Element &a, const Element &b) const {
			Element tmp;
			normalize(tmp, b.f);
			return a = {b.e + tmp.e, tmp.f};
		}
		
		Polynomial &denormalize(Polynomial &a, const Element &b) const {
			Polynomial tmp, x, xe;
			std::vector<integer> v;
			_F.init(x, v = {0, 1});
			_F.pow(xe, x, b.e);
			_F.mul(a, b.f, xe);
			
			return a;
		}
		
		Element &assign(Element &a, const Element &b) const {
			Polynomial tmp;
			_F.assign(tmp, b.f);
			return a = {b.e, tmp};
		}
		
		Element &init(Element &p) const {
			return assign(p, zero);
		}
		
		Element &init(Element &p, const std::vector<integer> &v) const {
			Polynomial tmp;
			_F.init(tmp, v);
			return normalize(p, tmp);
		}
		
		Element &init(Element &a, const Polynomial &b) const {
			return normalize(a, b);
		}
		
		integer &convert(integer &a, const Element &b) const {
			Polynomial tmp;
			return _F.convert(a, denormalize(tmp, b));
		}

		std::ostream& write(std::ostream& os) const {
			os << "Polynomial local ring at X^" << exp << " using ";
			_F.write(os);
			return os;
		}
		
		std::ostream& write(std::ostream& os, const Element &p) const {
			Polynomial tmp;
			_F.write(os, denormalize(tmp, p));
			return os;
		}
		
		std::ostream& writeNormalized(std::ostream& os, const Element &p) const {
			if (p.e == 0) {
				_F.write(os, p.f);
				return os;
			}
			
			os << "x";
			if (p.e > 1) {
				os << "^" << p.e;
			}
			
			if (_F.isOne(p.f)) {
				return os;
			}
			
			_F.write(os << "*(", p.f) << ")";
			return os;
		}
		
		bool isZero(const Element &a) const {
			return _F.isZero(a.f);
		}
		
		bool isOne(const Element &a) const {
			return a.e == 0 && _F.isOne(a.f);
		}
		
		bool isMOne(const Element &a) const {
			return a.e == 0 && _F.isMOne(a.f);
		}
		
		bool areEqual(const Element &a, const Element &b) const {
			return a.e == b.e && _F.areEqual(a.f, b.f);
		}
		
		bool isUnit(const Element &a) const {
			return a.e == 0 && !isZero(a);
		}
		
		size_t deg(const Element &p) const {
			return _F.deg(p.f) + p.e;
		}
		
		Coeff &getCoeff(Coeff &c, const Element &a, size_t e) const {
			if (e < a.e) {
				return getCoeffField().assign(c, getCoeffField().zero);
			}
			
			return _F.getCoeff(c, a.f, a.e - e);
		}
		
		Coeff &leadCoeff(Coeff &c, const Element &a) const {
			return _F.leadCoeff(c, a.f);
		}
		
		Element &monic(Element &m, const Element &a) const {
			Polynomial mf;
			_F.monic(mf, a.f);
			
			return m = {
				a.e,
				mf
			};
		}
		
		/// returns true if b divides a evenly
		bool isDivisor(const Element &a, const Element &b) const {
			return b.e <= a.e && !isZero(b);
		}
		
		// b = a * x^e
		Element &leftShift(Element &b, const Element &a, size_t e) const {
			if (a.e + e > exp) {
				return b = zero;
			}
			
			return b = {a.e + e, a.f};
		}
		
		// b = a / x^e
		Element &rightShift(Element &b, const Element &a, size_t e) const {
			return b = {a.e - e, a.f};
		}
		
		Element &rightShiftIn(Element &b, size_t e) const {
			b.e -= e;
			return b;
		}
		
		Element &div(Element &q, const Element &a, const Element &b) const {
			if (isZero(a)) {
				return q = zero;
			}
			
			if (_F.isOne(b.f)) {
				return q = {a.e - b.e, a.f};
			}
			
			Polynomial tmp, x, xe, xer;
			std::vector<integer> v = {0, 1};
			_F.init(x, v);
			_F.pow(xe, x, exp - a.e + b.e);
			_F.pow(xer, x, exp - a.e + b.e);
			
			_F.invMod(tmp, b.f, xe);
			_F.mulin(tmp, a.f);
			_F.modin(tmp, xer);
			
			return q = {a.e - b.e, tmp};
		}
		
		Element &divin(Element &a, const Element &b) const {
			Element tmp;
			assign(tmp, a);
			return div(a, tmp, b);
		}
		
		Element &mul(Element &c, const Element &a, const Element &b) const {
			if (isZero(a) || isZero(b)) {
				return c = zero;
			}
			
			if (_F.isOne(a.f)) {
				return c = {a.e + b.e, b.f};
			}
			if (_F.isOne(b.f)) {
				return c = {a.e + b.e, a.f};
			}
			
			Polynomial tmp, x, xe;
			std::vector<integer> v = {0, 1};
			_F.init(x, v);
			_F.pow(xe, x, exp - a.e - b.e);
			_F.mul(tmp, a.f, b.f);
			_F.modin(tmp, xe);
			
			return c = {a.e + b.e, tmp};
		}
		
		Element &mulin(Element &a, const Element &b) const {
			Element tmp;
			assign(tmp, a);
			return mul(a, b, tmp);
		}
		
		Element &pow(Element &r, const Element &a, size_t e) {
			if (a.e * e >= exp) {
				return r = zero;
			}
			
			Polynomial tmp;
			_F.pow(tmp, a.f, e);
			return r = {
				a.e * e,
				tmp
			};
		}
		
		Element &add(Element &c, const Element &a, const Element &b) const {
			if (a.e > b.e) {
				return add(c, b, a);
			}
			
			if (a.e == b.e) {
				Polynomial tmp;
				_F.add(tmp, a.f, b.f);
				return normalize(c, {a.e, tmp});
			}
			
			Polynomial tmp, x, xe;
			std::vector<integer> v = {0, 1};
			_F.init(x, v);
			_F.pow(xe, x, b.e - a.e);
			_F.mul(tmp, xe, b.f);
			_F.addin(tmp, a.f);
			return normalize(c, {a.e, tmp});
		}
		
		Element &addin(Element &a, const Element &b) const {
			Element tmp;
			assign(tmp, a);
			return add(a, b, tmp);
		}
		
		Element &neg(Element &a, const Element &b) const {
			Polynomial tmp;
			_F.neg(tmp, b.f);
			return a = {b.e, tmp};
		}
		
		Element &negin(Element &a) const {
			Element tmp;
			assign(tmp, a);
			return neg(a, tmp);
		}
		
		// r = a*x + y
		Element &axpy(Element &r, const Element &a, const Element &x, const Element &y) const {
			mul(r, a, x);			
			return addin(r, y);
		}
		
		// r += a * x
		Element &axpyin(Element &r, const Element &a, const Element &x) const {
			Element tmp;
			assign(tmp, r);
			
			return axpy(r, a, x, tmp);
		}
		
		Element &inv(Element &r, const Element &a) {
			assert(isUnit(a));
			
			Polynomial x, xe;
			std::vector<integer> v = {0, 1};
			_F.init(x, v);
			_F.pow(xe, x, exp);
			
			Polynomial tmp;
			_F.invMod(tmp, a.f, xe);
			
			return normalize(r, tmp);
		}
	}; // end of class PolynomialLocalX
	
	template<>
	class PolynomialLocalX<NTL_zz_pX> {
	public:
		typedef NTL_zz_pX Field;
		
		typedef typename Field::Element Polynomial;
		typedef typename Field::CoeffField CoeffField;
		typedef typename Field::Coeff Coeff;
		
		// f(x) * x^e ; x does not divide f(x)
		typedef struct {
			size_t e; 
			Polynomial f;
		} Element;
		
		// typedef UnparametricRandIter<Element> RandIter;

		const Field _F;
		const Element zero, one, mOne, X;
		size_t exp;

		PolynomialLocalX(const Field &F, size_t exponent) : _F(F),
			zero({0, _F.zero}), one({0, _F.one}), mOne({0, _F.mOne}),
			X({1, _F.one}), exp(exponent)
			{}
			
		PolynomialLocalX(const PolynomialLocalX &P) : _F(P._F), zero(P.zero),
			one(P.one), mOne(P.mOne), X(P.X), exp(P.exp) {}
			
		PolynomialLocalX(const PolynomialLocalX &P, size_t exponent) : _F(P._F),
			zero(P.zero), one(P.one), mOne(P.mOne), X(P.X), exp(exponent) {}
			
		void setExponent(size_t exponent) {
			exp = exponent;
		}
		
		size_t getExponent() const {
			return exp;
		}
			
		const CoeffField &getCoeffField() const {
			return _F.getCoeffField();
		}
		
		size_t firstNonZeroCoeff(Element &a) const {
			return a.e;
		}
		
		Element &normalize(Element &a, const Polynomial &b) const {
			if (_F.isZero(b)) {
				return a = zero;
			}
			
			size_t i = 0;
			for (; i <= _F.deg(b) && NTL::coeff(b, i) == 0; i++);
			
			Polynomial tmp;
			tmp = b >> i; // tmp = b / x^i
			
			return a = {i, tmp};
		}
			
		Element &normalize(Element &a, const Element &b) const {
			Element tmp;
			normalize(tmp, b.f);
			return a = {b.e + tmp.e, tmp.f};
		}
		
		Polynomial &denormalize(Polynomial &a, const Element &b) const {			
			a = b.f << b.e; // a = b.f * x^(b.e)
			return a;
		}
		
		Element &assign(Element &a, const Element &b) const {
			Polynomial tmp;
			_F.assign(tmp, b.f);
			return a = {b.e, tmp};
		}
		
		Element &init(Element &p) const {
			return assign(p, zero);
		}
		
		Element &init(Element &p, const std::vector<integer> &v) const {
			Polynomial tmp;
			_F.init(tmp, v);
			return normalize(p, tmp);
		}
		
		Element &init(Element &a, const Polynomial &b) const {
			return normalize(a, b);
		}
		
		integer &convert(integer &a, const Element &b) const {
			Polynomial tmp;
			return _F.convert(a, denormalize(tmp, b));
		}

		std::ostream& write(std::ostream& os) const {
			os << "Polynomial local ring at X^" << exp << " using ";
			_F.write(os);
			return os;
		}
		
		std::ostream& write(std::ostream& os, const Element &p) const {
			Polynomial tmp;
			_F.write(os, denormalize(tmp, p));
			return os;
		}
		
		std::ostream& writeNormalized(std::ostream& os, const Element &p) const {
			if (p.e == 0) {
				_F.write(os, p.f);
				return os;
			}
			
			os << "x";
			if (p.e > 1) {
				os << "^" << p.e;
			}
			
			if (_F.isOne(p.f)) {
				return os;
			}
			
			_F.write(os << "*(", p.f) << ")";
			return os;
		}
		
		bool isZero(const Element &a) const {
			return _F.isZero(a.f);
		}
		
		bool isOne(const Element &a) const {
			return a.e == 0 && _F.isOne(a.f);
		}
		
		bool isMOne(const Element &a) const {
			return a.e == 0 && _F.isMOne(a.f);
		}
		
		bool areEqual(const Element &a, const Element &b) const {
			return a.e == b.e && _F.areEqual(a.f, b.f);
		}
		
		bool isUnit(const Element &a) const {
			return a.e == 0 && !isZero(a);
		}
		
		size_t deg(const Element &p) const {
			return _F.deg(p.f) + p.e;
		}
		
		Coeff &getCoeff(Coeff &c, const Element &a, size_t e) const {
			if (e < a.e) {
				return getCoeffField().assign(c, getCoeffField().zero);
			}
			
			return _F.getCoeff(c, a.f, a.e - e);
		}
		
		Coeff &leadCoeff(Coeff &c, const Element &a) const {
			return _F.leadCoeff(c, a.f);
		}
		
		Element &monic(Element &m, const Element &a) const {
			Polynomial mf;
			_F.monic(mf, a.f);
			
			return m = {a.e, mf};
		}
		
		/// returns true if b divides a evenly
		bool isDivisor(const Element &a, const Element &b) const {
			return b.e <= a.e && !isZero(b);
		}
		
		// b = a * x^e
		Element &leftShift(Element &b, const Element &a, size_t e) const {
			if (a.e + e > exp) {
				return b = zero;
			}
			
			return b = {a.e + e, NTL::trunc(a.f, exp - a.e - e)};
		}
		
		Element &leftShiftIn(Element &b, size_t e) const {
			b.e += e;
			_F.assign(b.f, NTL::trunc(b.f, exp - b.e));
			return b;
		}
		
		// b = a / x^e
		Element &rightShift(Element &b, const Element &a, size_t e) const {
			return b = {a.e - e, a.f};
		}
		
		Element &rightShiftIn(Element &b, size_t e) const {
			b.e -= e;
			return b;
		}
		
		Element &div(Element &q, const Element &a, const Element &b) const {
			if (isZero(a)) {
				return q = zero;
			}
			
			if (_F.isOne(b.f)) {
				return q = {a.e - b.e, a.f};
			}
						
			Polynomial tmp, x, xe, xer;
			std::vector<integer> v = {0, 1};
			_F.init(x, v);
			_F.pow(xe, x, exp - a.e + b.e);
			_F.pow(xer, x, exp - a.e + b.e);
			
			_F.invMod(tmp, b.f, xe);
			_F.mulin(tmp, a.f);
			_F.modin(tmp, xer);
			
			return q = {a.e - b.e, tmp};
		}
		
		Element &divin(Element &a, const Element &b) const {
			Element tmp;
			assign(tmp, a);
			return div(a, tmp, b);
		}
		
		Element &mul(Element &c, const Element &a, const Element &b) const {
			if (isZero(a) || isZero(b)) {
				return c = zero;
			}
			
			Polynomial tmp;
			NTL::MulTrunc(tmp, a.f, b.f, exp - a.e - b.e);
			return c = {a.e + b.e, tmp};
		}
		
		Element &mulin(Element &a, const Element &b) const {
			Element tmp;
			assign(tmp, a);
			return mul(a, b, tmp);
		}
		
		Element &pow(Element &r, const Element &a, size_t e) const {
			if (a.e * e >= exp) {
				return r = zero;
			}
			
			Polynomial tmp;
			_F.pow(tmp, a.f, e);
			return r = {a.e * e, tmp};
		}
		
		Element &add(Element &c, const Element &a, const Element &b) const {
			if (a.e > b.e) {
				return add(c, b, a);
			}
			
			if (a.e == b.e) {
				Polynomial tmp;
				_F.add(tmp, a.f, b.f);
				return normalize(c, {a.e, tmp});
			}
			
			Polynomial tmp;
			tmp = b.f << (b.e - a.e);
			_F.addin(tmp, a.f);
			return normalize(c, {a.e, tmp});
		}
		
		Element &addin(Element &a, const Element &b) const {
			Element tmp;
			assign(tmp, a);
			return add(a, b, tmp);
		}
		
		Element &neg(Element &a, const Element &b) const {
			Polynomial tmp;
			_F.neg(tmp, b.f);
			return a = {b.e, tmp};
		}
		
		Element &negin(Element &a) const {
			Element tmp;
			assign(tmp, a);
			return neg(a, tmp);
		}
		
		// r = a*x + y
		Element &axpy(Element &r, const Element &a, const Element &x, const Element &y) const {
			mul(r, a, x);			
			return addin(r, y);
		}
		
		// r += a * x
		Element &axpyin(Element &r, const Element &a, const Element &x) const {
			Element tmp;
			assign(tmp, r);
			
			return axpy(r, a, x, tmp);
		}
		
		Element &inv(Element &r, const Element &a) const {			
			Polynomial tmp;
			NTL::InvTrunc(tmp, a.f, exp);
			return normalize(r, tmp);
		}
	}; // end of class PolynomialLocalX<NTL_zz_px>
	
	template<class PolynomialRing>
	class Hom<PolynomialRing, PolynomialLocalX<PolynomialRing>> {
	public:
		typedef PolynomialRing Source;
		typedef PolynomialLocalX<PolynomialRing> Target;
		typedef typename Source::Element SrcElt;
		typedef typename Target::Element Elt;
		
	public:
		Hom(const Source& S, const Target& T) : _source(S), _target(T) {}

		Elt& image(Elt& t, const SrcElt& s) {
			return _target.init(t, s);
		}
		
		SrcElt& preimage(SrcElt& s, const Elt& t) {
			return _target.convert(s, t);
		}
		
		const Source& source() { return _source;}
		const Target& target() { return _target;}
		
	private:
		const Source& _source;
		const Target& _target;
	}; // end Hom<PolynomialRing, PolynomialLocalX>

} // end of namespace LinBox

#endif // __LINBOX_polynomial_local_x_H


// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,:0,t0,+0,=s
// Local Variables:
// mode: C++
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 8
// End:
