/* Copyright (C) 2010 LinBox
 * Written by <Jean-Guillaume.Dumas@imag.fr>
 *
 *
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

#ifndef __LINBOX_rational_early_multip_cra_H
#define __LINBOX_rational_early_multip_cra_H

#include "givaro/zring.h"
#include "linbox/algorithms/rational-cra-builder-early-single.h"
#include "linbox/algorithms/rational-cra-builder-full-multip.h"

namespace LinBox
{

	template<class Domain_Type>
	struct RationalCRABuilderEarlyMultip : public RationalCRABuilderEarlySingle<Domain_Type>, public RationalCRABuilderFullMultip<Domain_Type> {
		typedef Domain_Type			Domain;
		typedef typename Domain_Type::Element 	DomainElement;
		typedef RationalCRABuilderEarlyMultip<Domain>	Self_t;
	protected:
		// Random coefficients for a linear combination
		// of the elements to be reconstructed
		std::vector< size_t >      	randv;

		void initialize (const Integer& D, const Integer& e) {return;}; // DON'T TOUCH
		void progress (const Integer & D, const Integer & e) {return;};
		 void initialize (const Domain& D, const DomainElement& e){return;};
		  Integer& result(Integer& d){return d;};
		         void progress (const Domain& D, const DomainElement& e){return;};






	public:


		RationalCRABuilderEarlyMultip(const size_t EARLY=LINBOX_DEFAULT_EARLY_TERMINATION_THRESHOLD) :
			RationalCRABuilderEarlySingle<Domain>(EARLY), RationalCRABuilderFullMultip<Domain>()
		{ }

		//!init
		template<template<class, class> class Vect, template <class> class Alloc>
		void initialize (const Domain& D, const Vect <DomainElement, Alloc<DomainElement> >& e)
		{
			// Random coefficients for a linear combination
			// of the elements to be reconstructed
			srand48(BaseTimer::seed());
			randv. resize ( e.size() );
			for ( std::vector<size_t>::iterator int_p = randv. begin();
			      int_p != randv. end(); ++ int_p)
				*int_p = ((size_t)lrand48()) % 20000;

			DomainElement z;
			// Could be much faster
			// - do not compute twice the product of moduli
			// - reconstruct one element of e until Early Termination,
			//   then only, try a random linear combination.
			RationalCRABuilderEarlySingle<Domain>::initialize(D,dot(z, D, e, randv) );
			RationalCRABuilderFullMultip<Domain>::initialize(D, e);
		}

		void initialize (const Domain& D, const BlasVector<Domain>& e)
		{
			// Random coefficients for a linear combination
			// of the elements to be reconstructed
			srand48(BaseTimer::seed());
			randv. resize ( e.size() );
			for ( std::vector<size_t>::iterator int_p = randv. begin();
			      int_p != randv. end(); ++ int_p)
				*int_p = ((size_t)lrand48()) % 20000;

			DomainElement z;
			// Could be much faster
			// - do not compute twice the product of moduli
			// - reconstruct one element of e until Early Termination,
			//   then only, try a random linear combination.
			RationalCRABuilderEarlySingle<Domain>::initialize(D,dot(z, D, e, randv) );
			RationalCRABuilderFullMultip<Domain>::initialize(D, e);
		}

		//!progress
		template<template<class,class> class Vect, template <class> class Alloc>
		void progress (const Domain& D, const Vect<DomainElement, Alloc<DomainElement> >& e)
		{
			DomainElement z;
			// Could be much faster
			// - do not compute twice the product of moduli
			// - reconstruct one element of e until Early Termination,
			//   then only, try a random linear combination.
			RationalCRABuilderEarlySingle<Domain>::progress(D, dot(z, D, e, randv));
			RationalCRABuilderFullMultip<Domain>::progress(D, e);
		}

		void progress (const Domain& D, const BlasVector<Domain>& e)
		{
			DomainElement z;
			// Could be much faster
			// - do not compute twice the product of moduli
			// - reconstruct one element of e until Early Termination,
			//   then only, try a random linear combination.
			RationalCRABuilderEarlySingle<Domain>::progress(D, dot(z, D, e, randv));
			RationalCRABuilderFullMultip<Domain>::progress(D, e);
		}

		//!result
		template<template<class, class> class Vect, template <class> class Alloc>
		Vect<Integer, Alloc<Integer> >& result(Vect<Integer, Alloc<Integer> >& num, Integer& den)
		{
			return RationalCRABuilderFullMultip<Domain>::result(num, den);
		}

		BlasVector<Givaro::ZRing<Integer> >& result(BlasVector<Givaro::ZRing<Integer>>& num, Givaro::ZRing<Integer>::Element& den)
		{
			return RationalCRABuilderFullMultip<Domain>::result(num, den);
		}

		//!tools
		bool terminated()
		{
			return RationalCRABuilderEarlySingle<Domain>::terminated();
		}

		bool noncoprime(const Integer& i) const
		{
			return RationalCRABuilderEarlySingle<Domain>::noncoprime(i);
		}

	protected:

		template <template<class, class> class Vect1, template <class> class Alloc, class Vect2>
		DomainElement& dot (DomainElement& z, const Domain& D,
				    const Vect1<DomainElement, Alloc<DomainElement> >& v1, const Vect2& v2)
		{

			D.assign(z,D.zero); DomainElement tmp;
			typename Vect1<DomainElement, Alloc<DomainElement> >::const_iterator v1_p;
			typename Vect2::const_iterator v2_p;
			for (v1_p  = v1. begin(), v2_p = v2. begin();
			     v1_p != v1. end();
			     ++ v1_p, ++ v2_p)
				D.axpyin(z, (*v1_p), D.init(tmp, (*v2_p)));
#if 0
			commentator().report(Commentator::LEVEL_ALWAYS, INTERNAL_DESCRIPTION) << "v: " << v2 << std::endl;
			commentator().report(Commentator::LEVEL_ALWAYS, INTERNAL_DESCRIPTION) << "z: " << z << std::endl;
#endif
			return z;
		}

		template <class Vect2>
		DomainElement& dot (DomainElement& z, const Domain& D,
				    const BlasVector<Domain >& v1, const Vect2& v2)
		{

			D.assign(z,D.zero); DomainElement tmp;
			typename BlasVector<Domain >::const_iterator v1_p;
			typename Vect2::const_iterator v2_p;
			for (v1_p  = v1. begin(), v2_p = v2. begin();
			     v1_p != v1. end();
			     ++ v1_p, ++ v2_p)
				D.axpyin(z, (*v1_p), D.init(tmp, (*v2_p)));
#if 0
			commentator().report(Commentator::LEVEL_ALWAYS, INTERNAL_DESCRIPTION) << "v: " << v2 << std::endl;
			commentator().report(Commentator::LEVEL_ALWAYS, INTERNAL_DESCRIPTION) << "z: " << z << std::endl;
#endif
			return z;
		}

	};
}

#endif //__LINBOX_rational_early_multip_cra_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
