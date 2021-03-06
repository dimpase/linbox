/* Copyright (C) LinBox
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

#ifndef __LINBOX_matrix_stream_readers_H
#define __LINBOX_matrix_stream_readers_H

/*! \file matrix-stream-readers.h
 * Here is where all the formats (each of which is a subclass of
 * MatrixStreamReader) are defined, in two places:
 *
 * First, the macro __MATRIX_STREAM_READERDEFS is put in the init() function of
 * MatrixStream and it creates all the format readers.  For each format reader,
 * a line of this macro should read: addReader( new MyReaderType() );
 *
 * Second, so those statements actually compile, the file containing each format
 * reader should be included with a line of the form: \verbatim #include "my-reader.h" \endverbatim
 */


#include "sms.h"
#include "sparse-row.h"
#include "generic-dense.h"
#include "matrix-market.h"
#include "maple.h"

#define __MATRIX_STREAM_READERDEFS                    \
do {                                                  \
	addReader( new SMSReader<Field>()          ); \
	addReader( new SparseRowReader<Field>()    ); \
	addReader( new MatrixMarketReader<Field>() ); \
	addReader( new MapleReader<Field>()        ); \
	addReader( new DenseReader<Field>()        ); \
} while (false)

#endif //__LINBOX_matrix_stream_readers_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
