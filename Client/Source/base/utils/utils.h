#ifndef __DFG_UTILS__
#define __DFG_UTILS__

#include <unicode/umsg.h>




/*! \brief Simple lerp functor.
 *
 *	\author Andrew "RevEn" Karpushin
 */

template< class _Type >
struct SimpleLerpFn
{
	_Type lerp( const _Type& a, const _Type& b, const float& t ) { return static_cast< _Type >( a + ( b - a ) * t ); };
};


//! Hermite spline interpolation.
template< class _Type >
_Type HermiteSpline( const _Type& v0, const _Type& t0, const _Type& v1, const _Type& t1, float t )
{
    float t2 = t * t;
    float t3 = t * t2;

    return ( 2.0f * t3 - 3.0f * t2 + 1 ) * v0 + ( t3 - 2.0f * t2 + t ) * t0 + ( 3.0f * t2 - 2.0f * t3 ) * v1 + ( t3 - t2 ) * t1;
};


//! Generates new UUID.
std::string GenerateUUID( );


//! Helper function that converts wchar_t string to UChar one. Max 2048 chars.
const UChar * WCSToUString( const wchar_t * str );


//! Convert UTF8 string to wchar_t one.
const wchar_t * UTF8ToWCS( const char * str );


//! Function to map char * string to wchar_t * one. Works ONLY for ANSI characters. Max 2048 chars.
const wchar_t * ANSIToWCS( const char * str );


//! Portable swprintf version. Nested calls are not allowed. Max 2048 chars.
const wchar_t * Format( const wchar_t * fmt, ... );


//! Clip text to bounds inserting '...' if text is too long.
const wchar_t * ClipTextToBounds( const wchar_t * text, float width, const gameplay::Font * font, float fontSize );



#endif // __DFG_UTILS__