/* -*- mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
/* Copyright (C) 2010 LinBox
 *
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*! @file field/modular-int32_t.h
 * @ingroup field
 * @brief  representation of <code>Z/mZ</code> over \c int32_t .
 */
#ifndef __LINBOX_modular_int32_H
#define __LINBOX_modular_int32_H


#include <math.h>
#include "linbox/linbox-config.h"
#include "linbox/integer.h"
#include "linbox/vector/vector-domain.h"
#include "linbox/field/field-interface.h"
#include "linbox/field/field-traits.h"
#include "linbox/util/debug.h"
#include "linbox/field/field-traits.h"

#ifndef LINBOX_MAX_INT
#define LINBOX_MAX_INT 2147483647
#endif

// This is replaced by FieldTraits< Modular<int32_t> >::maxModulus(integer&)
// #ifndef LINBOX_MAX_MODULUS
// #define LINBOX_MAX_MODULUS 1073741823
// #endif

// Namespace in which all LinBox code resides
namespace LinBox
{

	template< class Element >
	class Modular;
	template< class Element >
	class ModularRandIter;
	template< class Field, class RandIter >
	class NonzeroRandIter;

	template<class Field>
	class DotProductDomain;
	template<class Field>
	class FieldAXPY;
	template<class Field>
	class MVProductDomain;

	template <class Ring>
	struct ClassifyRing;

	template <class Element>
	struct ClassifyRing<Modular<Element> >;

	template <>
	struct ClassifyRing<Modular<int32_t> > {
		typedef RingCategories::ModularTag categoryTag;
	};



	/** \brief Specialization of Modular to int32_t element type with efficient dot product.
	 *
	 * Efficient element operations for dot product, mul, axpy, by using floating point
	 * inverse of modulus (borrowed from NTL) and some use of non-normalized intermediate values.
	 *
	 * For some uses this is the most efficient field for primes in the range from half word
	 * to 2^30.
	 *
	 * Requires: Modulus < 2^30.
	 * Intended use: 2^15 < prime modulus < 2^30.
	 \ingroup field
	 */
	template <>
	class Modular<int32_t> : public FieldInterface {

	protected:

		int32_t modulus;

		double modulusinv;

		int32_t _two64;

	public:

		friend class FieldAXPY<Modular<int32_t> >;
		friend class DotProductDomain<Modular<int32_t> >;
		friend class MVProductDomain<Modular<int32_t> >;

		typedef int32_t Element;
		typedef ModularRandIter<int32_t> RandIter;
		typedef NonzeroRandIter<Modular<int32_t>, ModularRandIter<int32_t> > NonZeroRandIter;

		//default modular field,taking 65521 as default modulus
		Modular () :
			modulus(65521)
		{
			modulusinv=1/(double)65521;

			_two64 = (int32_t) ((uint64_t) (-1) % (uint64_t) 65521);
			_two64 += 1;
			if (_two64 >= 65521) _two64 -= 65521;
		}

		Modular (int32_t value, int32_t exp = 1) :
			modulus(value)
		{
			modulusinv = 1 / ((double) value);
			if(exp != 1) throw PreconditionFailed(__func__,__FILE__,__LINE__,"exponent must be 1");
			if(value<=1) throw PreconditionFailed(__func__,__FILE__,__LINE__,"modulus must be > 1");
			integer max;
			if(value>FieldTraits< Modular<int32_t> >::maxModulus(max)) throw PreconditionFailed(__func__,__FILE__,__LINE__,"modulus is too big");
			_two64 = (int32_t) ((uint64_t) (-1) % (uint64_t) value);
			_two64 += 1;
			if (_two64 >= value) _two64 -= value;
		}

		Modular(const Modular<int32_t>& mf) :
			modulus(mf.modulus),modulusinv(mf.modulusinv),_two64(mf._two64)
		{}

		const Modular &operator=(const Modular<int32_t> &F)
		{
			modulus = F.modulus;
			modulusinv = F.modulusinv;
			_two64 = F._two64;
			return *this;
		}


		inline integer &cardinality (integer &c) const
		{
			return c = modulus;
		}

		inline integer &characteristic (integer &c) const
		{ return c = modulus; }

		inline size_t characteristic () const
		{ return modulus; }


		inline integer &convert (integer &x, const Element &y) const
		{
			return x = y;
		}

		inline int32_t &convert (int32_t &x, const Element &y) const
		{
			return x = y;
		}
		inline double &convert (double &x, const Element &y) const
		{
			return x = (double) y;
		}

		inline float &convert (float &x, const Element &y) const
		{
			return x = (float) y;
		}

		inline std::ostream &write (std::ostream &os) const
		{
			return os << "int32_t mod " << modulus;
		}

		inline std::istream &read (std::istream &is)
		{
			is >> modulus;
			modulusinv = 1 /((double) modulus );
			if(modulus <= 1) throw PreconditionFailed(__func__,__FILE__,__LINE__,"modulus must be > 1");
			integer max;
			if(modulus > FieldTraits< Modular<int32_t> >::maxModulus(max)) throw PreconditionFailed(__func__,__FILE__,__LINE__,"modulus is too big");
			_two64 = (int32_t) ((uint64_t) (-1) % (uint64_t) modulus);
			_two64 += 1;
			if (_two64 >= modulus) _two64 -= modulus;

			return is;
		}

		inline std::ostream &write (std::ostream &os, const Element &x) const
		{
			return os << x;
		}

		inline std::istream &read (std::istream &is, Element &x) const
		{
			integer tmp;
			is >> tmp;
			init(x,tmp);
			return is;
		}

		inline Element &init (Element & x, const double &y) const
		{
			double z = fmod(y, (double)modulus);
			if (z < 0) z += (double)modulus;
			//z += 0.5; // C Pernet Sounds nasty and not necessary
			return x = static_cast<long>(z); //rounds towards 0
		}

		inline Element &init (Element & x, const float &y) const
		{
			return init(x , (double) y);
		}

		template<class Element1>
		inline Element &init (Element & x, const Element1 &y) const
		{
			x = y % modulus;
			if (x < 0) x += modulus;
			return x;
		}

		inline Element &init (Element &x, const integer &y) const
		{
			x = Element (y % modulus);
			if (x < 0) x += modulus;
			return x;
		}

		inline Element& init(Element& x, int y =0) const
		{
			x = y % modulus;
			if ( x < 0 ) x += modulus;
			return x;
		}

		inline Element& init(Element& x, long y) const
		{
			x = y % modulus;
			if ( x < 0 ) x += modulus;
			return x;
		}

		inline Element& assign(Element& x, const Element& y) const
		{
			return x = y;
		}


		inline bool areEqual (const Element &x, const Element &y) const
		{
			return x == y;
		}

		inline  bool isZero (const Element &x) const
		{
			return x == 0;
		}

		inline bool isOne (const Element &x) const
		{
			return x == 1;
		}

		inline Element &add (Element &x, const Element &y, const Element &z) const
		{
			x = y + z;
			if ( x >= modulus ) x -= modulus;
			return x;
		}

		inline Element &sub (Element &x, const Element &y, const Element &z) const
		{
			x = y - z;
			if (x < 0) x += modulus;
			return x;
		}

		inline Element &mul (Element &x, const Element &y, const Element &z) const
		{
			int32_t q;

			q  = (int32_t) ((((double) y)*((double) z)) * modulusinv);  // q could be off by (+/-) 1
			x = (int32_t) (y*z - q*modulus);


			if (x >= modulus)
				x -= modulus;
			else if (x < 0)
				x += modulus;

			return x;
		}

		inline Element &div (Element &x, const Element &y, const Element &z) const
		{
			linbox_check(!isZero(z));
			Element temp;
			inv (temp, z);
			return mul (x, y, temp);
		}

		inline Element &neg (Element &x, const Element &y) const
		{
			if(y == 0) return x=0;
			else return x = modulus-y;
		}

		inline Element &inv (Element &x, const Element &y) const
		{
			linbox_check(!isZero(y));
			int32_t d, t;
			XGCD(d, x, t, y, modulus);
			if (d != 1)
			{
				throw PreconditionFailed(__func__,__FILE__,__LINE__,"InvMod: Input is not invertible ");
			}
			if (x < 0)
				x += modulus;
			return x;

		}

		inline Element &axpy (Element &r,
				      const Element &a,
				      const Element &x,
				      const Element &y) const
		{
			int32_t q;

			q  = (int32_t) (((((double) a) * ((double) x)) + (double)y) * modulusinv);  // q could be off by (+/-) 1
			r = (int32_t) (a * x + y - q*modulus);


			if (r >= modulus)
				r -= modulus;
			else if (r < 0)
				r += modulus;

			return r;

		}

		inline Element &addin (Element &x, const Element &y) const
		{
			x += y;
			if (  x >= modulus ) x -= modulus;
			return x;
		}

		inline Element &subin (Element &x, const Element &y) const
		{
			x -= y;
			if (x < 0) x += modulus;
			return x;
		}

		inline Element &mulin (Element &x, const Element &y) const
		{
			return mul(x,x,y);
		}

		inline Element &divin (Element &x, const Element &y) const
		{
			return div(x,x,y);
		}

		inline Element &negin (Element &x) const
		{
			if (x == 0) return x;
			else return x = modulus - x;
		}

		inline Element &invin (Element &x) const
		{
			linbox_check(!isZero(x));
			return inv (x, x);
		}

		inline Element &axpyin (Element &r, const Element &a, const Element &x) const
		{
			int32_t q;

			q  = (int32_t) (((((double) a) * ((double) x)) + (double) r) * modulusinv);  // q could be off by (+/-) 1
			r = (int32_t) (a * x + r - q*modulus);


			if (r >= modulus)
				r -= modulus;
			else if (r < 0)
				r += modulus;

			return r;
		}

		unsigned long AccBound(const Element&r) const
		{
			Element one, zero ; init(one,1UL) ; init(zero,0UL);
			double max_double = (double) (INT_MAX) - modulus ;
			double p = modulus-1 ;
			if (areEqual(zero,r))
				return (unsigned long) max_double/p ;
			else if (areEqual(one,r))
			{
				if (modulus>= getMaxModulus())
					return 0 ;
				else
					return (unsigned long) max_double/(modulus*modulus) ;
			} else
				throw LinboxError("Bad input, expecting 0 or 1");
			return 0;
		}


		static inline int32_t getMaxModulus()
		{ return 1073741824; } // 2^30

	private:

		static void XGCD(int32_t& d, int32_t& s, int32_t& t, int32_t a, int32_t b)
		{
			int32_t  u, v, u0, v0, u1, v1, u2, v2, q, r;

			int32_t aneg = 0, bneg = 0;

			if (a < 0)
			{
				if (a < -LINBOX_MAX_INT) throw PreconditionFailed(__func__,__FILE__,__LINE__,"XGCD: integer overflow");
				a = -a;
				aneg = 1;
			}

			if (b < 0)
			{
				if (b < -LINBOX_MAX_INT) throw PreconditionFailed(__func__,__FILE__,__LINE__,"XGCD: integer overflow");
				b = -b;
				bneg = 1;
			}

			u1 = 1; v1 = 0;
			u2 = 0; v2 = 1;
			u = a; v = b;

			while (v != 0)
			{
				q = u / v;
				r = u % v;
				u = v;
				v = r;
				u0 = u2;
				v0 = v2;
				u2 =  u1 - q*u2;
				v2 = v1- q*v2;
				u1 = u0;
				v1 = v0;
			}

			if (aneg)
				u1 = -u1;

			if (bneg)
				v1 = -v1;

			d = u;
			s = u1;
			t = v1;
		}

	};

	template <>
	class FieldAXPY<Modular<int32_t> > {
	public:

		typedef int32_t Element;
		typedef Modular<int32_t> Field;

		FieldAXPY (const Field &F) :
			_F (F),_y(0)
		{}


		FieldAXPY (const FieldAXPY &faxpy) :
			_F (faxpy._F), _y (0)
		{}

		FieldAXPY<Modular<int32_t> > &operator = (const FieldAXPY &faxpy)
		{
			_F = faxpy._F;
			_y = faxpy._y;
			return *this;
		}

		inline uint64_t& mulacc (const Element &a, const Element &x)
		{
			uint64_t t = (uint64_t) a * (uint64_t) x;
			_y += t;
			if (_y < t)
				return _y += _F._two64;
			else
				return _y;
		}

		inline uint64_t& accumulate (const Element &t)
		{
			_y += t;
			if (_y < (uint64_t)t)
				return _y += _F._two64;
			else
				return _y;
		}

		inline Element& get (Element &y)
		{
			y =_y % (uint64_t) _F.modulus;
			return y;
		}

		inline FieldAXPY &assign (const Element y)
		{
			_y = y;
			return *this;
		}

		inline void reset()
		{
			_y = 0;
		}

	protected:
		Field _F;
		uint64_t _y;
	};


	template <>
	class DotProductDomain<Modular<int32_t> > : private virtual VectorDomainBase<Modular<int32_t> > {

	public:
		typedef int32_t Element;
		DotProductDomain (const Modular<int32_t> &F) :
			VectorDomainBase<Modular<int32_t> > (F)
		{}


	protected:
		template <class Vector1, class Vector2>
		inline Element &dotSpecializedDD (Element &res, const Vector1 &v1, const Vector2 &v2) const
		{

			typename Vector1::const_iterator i;
			typename Vector2::const_iterator j;

			uint64_t y = 0;
			uint64_t t;

			for (i = v1.begin (), j = v2.begin (); i < v1.end (); ++i, ++j)
			{
				t = ( (uint64_t) *i ) * ( (uint64_t) *j );
				y += t;

				if (y < t)
					y += _F._two64;
			}

			y %= (uint64_t) _F.modulus;
			return res = y;

		}

		template <class Vector1, class Vector2>
		inline Element &dotSpecializedDSP (Element &res, const Vector1 &v1, const Vector2 &v2) const
		{
			typename Vector1::first_type::const_iterator i_idx;
			typename Vector1::second_type::const_iterator i_elt;

			uint64_t y = 0;
			uint64_t t;

			for (i_idx = v1.first.begin (), i_elt = v1.second.begin (); i_idx != v1.first.end (); ++i_idx, ++i_elt)
			{
				t = ( (uint64_t) *i_elt ) * ( (uint64_t) v2[*i_idx] );
				y += t;

				if (y < t)
					y += _F._two64;
			}


			y %= (uint64_t) _F.modulus;

			return res = y;
		}
	};

	// Specialization of MVProductDomain for int32_t modular field

	template <>
	class MVProductDomain<Modular<int32_t> > {
	public:

		typedef int32_t Element;

	protected:
		template <class Vector1, class Matrix, class Vector2>
		inline Vector1 &mulColDense
		(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v) const
		{
			return mulColDenseSpecialized
			(VD, w, A, v, typename VectorTraits<typename Matrix::Column>::VectorCategory ());
		}

	private:
		template <class Vector1, class Matrix, class Vector2>
		Vector1 &mulColDenseSpecialized
		(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
		 VectorCategories::DenseVectorTag) const;
		template <class Vector1, class Matrix, class Vector2>
		Vector1 &mulColDenseSpecialized
		(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
		 VectorCategories::SparseSequenceVectorTag) const;
		template <class Vector1, class Matrix, class Vector2>
		Vector1 &mulColDenseSpecialized
		(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
		 VectorCategories::SparseAssociativeVectorTag) const;
		template <class Vector1, class Matrix, class Vector2>
		Vector1 &mulColDenseSpecialized
		(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
		 VectorCategories::SparseParallelVectorTag) const;

		mutable std::vector<uint64_t> _tmp;
	};

	template <class Vector1, class Matrix, class Vector2>
	Vector1 &MVProductDomain<Modular<int32_t> >::mulColDenseSpecialized
	(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
	 VectorCategories::DenseVectorTag) const
	{

		linbox_check (A.coldim () == v.size ());
		linbox_check (A.rowdim () == w.size ());

		typename Matrix::ConstColIterator i = A.colBegin ();
		typename Vector2::const_iterator j;
		typename Matrix::Column::const_iterator k;
		std::vector<uint64_t>::iterator l;

		uint64_t t;

		if (_tmp.size () < w.size ())
			_tmp.resize (w.size ());

		std::fill (_tmp.begin (), _tmp.begin () + w.size (), 0);

		for (j = v.begin (); j != v.end (); ++j, ++i)
		{
			for (k = i->begin (), l = _tmp.begin (); k != i->end (); ++k, ++l)
			{
				t = ((uint64_t) *k) * ((uint64_t) *j);

				*l += t;

				if (*l < t)
					*l += VD.field ()._two64;
			}
		}

		typename Vector1::iterator w_j;

		for (w_j = w.begin (), l = _tmp.begin (); w_j != w.end (); ++w_j, ++l)
			*w_j = *l % VD.field ().modulus;

		return w;
	}

	template <class Vector1, class Matrix, class Vector2>
	Vector1 &MVProductDomain<Modular<int32_t> >::mulColDenseSpecialized
	(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
	 VectorCategories::SparseSequenceVectorTag) const
	{
		linbox_check (A.coldim () == v.size ());
		linbox_check (A.rowdim () == w.size ());

		typename Matrix::ConstColIterator i = A.colBegin ();
		typename Vector2::const_iterator j;
		typename Matrix::Column::const_iterator k;
		std::vector<uint64_t>::iterator l;

		uint64_t t;

		if (_tmp.size () < w.size ())
			_tmp.resize (w.size ());

		std::fill (_tmp.begin (), _tmp.begin () + w.size (), 0);

		for (j = v.begin (); j != v.end (); ++j, ++i)
		{
			for (k = i->begin (), l = _tmp.begin (); k != i->end (); ++k, ++l)
			{
				t = ((uint64_t) k->second) * ((uint64_t) *j);

				_tmp[k->first] += t;

				if (_tmp[k->first] < t)
					_tmp[k->first] += VD.field ()._two64;
			}
		}

		typename Vector1::iterator w_j;

		for (w_j = w.begin (), l = _tmp.begin (); w_j != w.end (); ++w_j, ++l)
			*w_j = *l % VD.field ().modulus;

		return w;
	}

	template <class Vector1, class Matrix, class Vector2>
	Vector1 &MVProductDomain<Modular<int32_t> >::mulColDenseSpecialized
	(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
	 VectorCategories::SparseAssociativeVectorTag) const
	{

		linbox_check (A.coldim () == v.size ());
		linbox_check (A.rowdim () == w.size ());

		typename Matrix::ConstColIterator i = A.colBegin ();
		typename Vector2::const_iterator j;
		typename Matrix::Column::const_iterator k;
		std::vector<uint64_t>::iterator l;

		uint64_t t;

		if (_tmp.size () < w.size ())
			_tmp.resize (w.size ());

		std::fill (_tmp.begin (), _tmp.begin () + w.size (), 0);

		for (j = v.begin (); j != v.end (); ++j, ++i)
		{
			for (k = i->begin (), l = _tmp.begin (); k != i->end (); ++k, ++l)
			{
				t = ((uint64_t) k->second) * ((uint64_t) *j);

				_tmp[k->first] += t;

				if (_tmp[k->first] < t)
					_tmp[k->first] += VD.field ()._two64;
			}
		}

		typename Vector1::iterator w_j;

		for (w_j = w.begin (), l = _tmp.begin (); w_j != w.end (); ++w_j, ++l)
			*w_j = *l % VD.field ().modulus;

		return w;
	}

	template <class Vector1, class Matrix, class Vector2>
	Vector1 &MVProductDomain<Modular<int32_t> >::mulColDenseSpecialized
	(const VectorDomain<Modular<int32_t> > &VD, Vector1 &w, const Matrix &A, const Vector2 &v,
	 VectorCategories::SparseParallelVectorTag) const
	{

		linbox_check (A.coldim () == v.size ());
		linbox_check (A.rowdim () == w.size ());

		typename Matrix::ConstColIterator i = A.colBegin ();
		typename Vector2::const_iterator j;
		typename Matrix::Column::first_type::const_iterator k_idx;
		typename Matrix::Column::second_type::const_iterator k_elt;
		std::vector<uint64_t>::iterator l;

		uint64_t t;

		if (_tmp.size () < w.size ())
			_tmp.resize (w.size ());

		std::fill (_tmp.begin (), _tmp.begin () + w.size (), 0);

		for (j = v.begin (); j != v.end (); ++j, ++i)
		{
			for (k_idx = i->first.begin (), k_elt = i->second.begin (), l = _tmp.begin ();
			     k_idx != i->first.end ();
			     ++k_idx, ++k_elt, ++l)
			{
				t = ((uint64_t) *k_elt) * ((uint64_t) *j);

				_tmp[*k_idx] += t;

				if (_tmp[*k_idx] < t)
					_tmp[*k_idx] += VD.field ()._two64;
			}
		}

		typename Vector1::iterator w_j;

		for (w_j = w.begin (), l = _tmp.begin (); w_j != w.end (); ++w_j, ++l)
			*w_j = *l % VD.field ().modulus;

		return w;
	}


}

#include "linbox/randiter/modular.h"

#endif //__LINBOX_modular_int32_H
