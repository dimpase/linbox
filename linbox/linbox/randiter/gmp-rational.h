/* -*- mode: c; style: linux -*- */

/* linbox/randiter/gmp-rational.h
 * Copyright (C) 2001-2002 Bradford Hovinen
 *
 * Written by William J Turner <wjturner@math.ncsu.edu>,
 *            Bradford Hovinen <hovinen@cis.udel.edu>
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

#ifndef __RANDITER_GMP_RATIONAL_H
#define __RANDITER_GMP_RATIONAL_H

#include "linbox/field/gmp-rational.h"
#include "linbox/element/gmp-rational.h"
#include "linbox/element/abstract.h"
#include "linbox/element/envelope.h"

#include <sys/time.h>
#include <stdlib.h>

namespace LinBox
{
	class GMPRationalRandIter
	{
	    public:
    
		typedef GMPRationalElement element;
    
		GMPRationalRandIter (const GMPRationalField &F,
				     const integer &size = 0,
				     const integer &seed = 0)
			: _F (F), _size (size), _seed (seed)
		{
			if (seed == 0)
				_seed = time (NULL);
		}

		GMPRationalRandIter (const GMPRationalRandIter& R)
			: _F (R._F), _size (R._size), _seed (R._seed) {}

		~GMPRationalRandIter() 
			{}
    
		GMPRationalRandIter& operator=(const GMPRationalRandIter& R)
		{
			if (this != &R) { // guard against self-assignment
				_F = R._F;
				_seed = R._seed;
				_size = R._size;
			}
			return *this;
		}
 
		element &random (element &a) 
		{
			unsigned int s;
			int value;

			if (_size == 0) {
				s = _seed;

				mpz_set_si (mpq_numref (a.rep), value);

				do {
					value = rand_r (&s);
				} while (value == 0);

				_seed = s;
				mpz_set_si (mpq_denref (a.rep), value);

				return a;
			}
			else {
				unsigned int s;
				int num, den;

				s = _seed;
				num = rand_r (&s);

				if (_size > 0) {
					num %= _size;
					den = 1L;
				} else {
					den = rand_r (&s);
				}

				_seed = s;

				mpz_set_si (mpq_numref (a.ref), num);
				mpz_set_si (mpq_denref (a.ref), den);

				return a;
			}
		}
 
		/** Random field element creator.
		 * This returns a random field element from the information supplied
		 * at the creation of the generator.
		 * Required by abstract base class.
		 * @return reference to random field element
		 */
		Element_abstract &random (Element_abstract &a) 
		{
			integer tmp;

			random (tmp);
			return (a = Element_envelope <GMPRationalField> (tmp));
		}

	    private:

		GMPRationalField _F;

		integer _size;
		integer _seed;
     
	}; // class GMPRationalRandIter
 
} // namespace LinBox

#endif // __RANDITER_GMP_RANDOM_H
