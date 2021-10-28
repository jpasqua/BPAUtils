/*
 * BPAFixedSizeBuffer
 *    A simple fixed size array that can be manipulated with push/pop and shift/unshift.
 *    It keeps a count of the current number of elements and the _capacity. Storage may be
 *    allocated on the heap, or provided by the client.
 *
 */

#ifndef BPAFixedSizeBuffer_h
#define BPAFixedSizeBuffer_h

#include <stdint.h>
#include <stddef.h>

/**
 * For the push() and unshift() functions, you may choose whether to pass-by-value, pass-by-reference,
 * or have the template choose at compile time based on the element type. This is the default and in this case
 * we pass-by-value for fundamental types and pass-by-reference otherwise. To always use pass-by-value, which
 * is what the original library did, #define ARG_TYPE_VAL *before* including BPAFixedSizeBuffer.h. To always
 * use pass-by-reference #define ARG_TYPE_REF. Don't define either to get the default behavior.
 */
#include <type_traits>
template<typename T>
#if defined(ARG_TYPE_VAL)
	using choose_arg_type = T;
#elif defined(ARG_TYPE_REF)
	using choose_arg_type = const T &;
#else
	using choose_arg_type = typename std::conditional<std::is_fundamental<T>::value, T,  const T&>::type;
#endif


template<typename T>
class BPAFixedSizeBuffer {
public:

  BPAFixedSizeBuffer() = default;	// Can't use until init() is called!

  BPAFixedSizeBuffer(size_t maxSize) { init(maxSize); }

  BPAFixedSizeBuffer(T* space, size_t maxSize, bool initialized = false) {
  	init(space, maxSize);
  	if (initialized) {
  		// The client has provided initialized storage. The buffer should act as
  		// if it received empty storage and push was called maxSize times.
  		count = maxSize;
  		tail = buffer + _capacity - 1;
  	}
  }

  ~BPAFixedSizeBuffer() {
    if (_manageStorage) delete[] buffer;
  }

  void init(size_t maxSize) {
    init(new T[maxSize], maxSize, true);
  }

  void init(T* space, size_t maxSize) {
    init(space, maxSize, false);
  }
  

	/**
	 * Disables copy constructor
	 */
	BPAFixedSizeBuffer(const BPAFixedSizeBuffer&) = delete;
	BPAFixedSizeBuffer(BPAFixedSizeBuffer&&) = delete;

	/**
	 * Disables assignment operator
	 */
	BPAFixedSizeBuffer& operator=(const BPAFixedSizeBuffer&) = delete;
	BPAFixedSizeBuffer& operator=(BPAFixedSizeBuffer&&) = delete;

	/**
	 * Adds an element to the beginning of buffer if there is space
	 */
	bool unshift(choose_arg_type<T> value){
		if (count == _capacity) return false;
    if (head == buffer) { head = buffer + _capacity; }
    *--head = value;
    if (isEmpty()) { tail = head; }
    count++;
    return true;
  }

	/**
	 * Adds an element to the end of buffer if there is space
	 */
	bool push(choose_arg_type<T> value) {
		if (count == _capacity) return false;

    if (++tail == buffer + _capacity) { tail = buffer; }
    *tail = value;

    if (isEmpty()) { head = tail; }
    count++;
    return true;
  }

	/**
	 * Removes an element from the beginning of the buffer.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	T shift() {
    if (count == 0) return *head;
    T result = *head++;
    if (head >= buffer + _capacity) { head = buffer; }
    count--;
    return result;
  }

	/**
	 * Removes an element from the end of the buffer.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	T pop() {
    if (count == 0) return *tail;
    T result = *tail--;
    if (tail < buffer) { tail = buffer + _capacity - 1; }
    count--;
    return result;
  }

	/**
	 * Returns the element at the beginning of the buffer.
	 */
	T inline first() const { return *head; }

	/**
	 * Returns the element at the end of the buffer.
	 */
	T inline last() const { return *tail; }

	/**
	 * Array-like access to buffer.
	 * Calling this operation using and index value greater than `size - 1` returns the tail element.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	T operator [] (size_t index) const {
    if (index >= count) return *tail;
    return *(buffer + ((head - buffer + index) % _capacity));
  }
   

	/**
	 * Similar to operator [], but returns a const ref to the stored element rather than a copy.
	 * Similar to std::vector::at, but always returns a const reference. 
	 * Calling this operation using and index value greater than `size - 1` returns the tail element.
	 * *WARNING* Calling this operation on an empty buffer has an unpredictable behaviour.
	 */
	const T& peekAt(size_t index) const {
    if (index >= count) return *tail;
    return (buffer[(head - buffer + index) % _capacity]);
  }

  /**
   * Returns the maximum cpacity of the buffer.
   */
  size_t inline capacity() const { return _capacity; }

  /**
   * Returns how many elements are actually stored in the buffer.
   */
  size_t inline size() const { return count; }

	/**
	 * Returns how many elements can be safely pushed into the buffer.
	 */
	size_t inline available() const { return _capacity - count; }

	/**
	 * Returns `true` if no elements can be removed from the buffer.
	 */
	bool inline isEmpty() const { return count == 0; }

	/**
	 * Returns `true` if no elements can be added to the buffer without overwriting existing elements.
	 */
	bool inline isFull() const { return count == _capacity; }

	/**
	 * Resets the buffer to a clean status, making all buffer positions available.
	 */
	void inline clear() {
    head = tail = buffer;
    count = 0;
  }


private:

  void init(T* space, size_t maxSize, bool manage) {
    buffer = space;
    _capacity = maxSize;
    _manageStorage = manage;
    head = buffer;
    tail = buffer;
    count = 0;
  }

	T* buffer = nullptr;
  size_t _capacity = 0;
  bool _manageStorage = false;

	T *head = nullptr;
	T *tail = nullptr;
#ifndef CIRCULAR_BUFFER_INT_SAFE
	size_t count = 0;
#else
	volatile size_t count = 0;
#endif
};

#endif	// BPAFixedSizeBuffer_h

/*
 Copyright (c) 2017 Roberto Lo Giacco.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as 
 published by the Free Software Foundation, either version 3 of the 
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
