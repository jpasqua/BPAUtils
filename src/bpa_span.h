/*
 * bpa_span
 *     A wrapper around Tristan Brindle's span.hpp which implements
 *     C++20's std::span.
 *
 * NOTES:
 * o The point of the wrapper is to set the namespace to bpa (default is tcb).
 * o When C++20 is available, users of the code will replace bpa::span with std::span
 * o See span.hpp for details
 *
 */

#ifndef BPA_SPAN_h
#define BPA_SPAN_h

#define TCB_SPAN_NAMESPACE_NAME bpa
#include "span.hpp"

#endif	// BPA_SPAN_h