/*
 *  SVector.h
 *  
 *  An simple, non-exception safe implementation of vector
 */
 
#ifndef _SVECTOR_H_
#define _SVECTOR_H_

#include <stdint.h>
#include "memory.h" 


//T must be default and copy constructable; does not have to be assignable
template<class T>
class SVector
{
	public:
		explicit SVector(uint32_t newCapacity = 0)
		:	fCapacity(0), fSize(0), fData(NULL)
		{
			reserve(newCapacity);
		}

		SVector(const SVector &rhs)
		:	fCapacity(0), fSize(0), fData(NULL)
		{
			reserve(rhs.size());
			fSize = rhs.size();
			for(uint32_t i = 0; i < rhs.size(); ++i)
				NEW(fData + i) T(rhs[i]);
		}

		~SVector()
		{
			clear();
			operator delete[](fData);
		}

		SVector &operator=(const SVector &rhs)
		{
			clear();
			reserve(rhs.size());
			fSize = rhs.size();
			for(uint32_t i = 0; i < rhs.size(); ++i)
				NEW (fData + i) T(rhs[i]);
			return *this;
		}

		T &operator[](uint32_t i) const 			{ return fData[i]; }
		T &front() const 							{ return fData[0]; }
		T &back() const 							{ return fData[fSize - 1]; }
		T *begin() const 							{ return fData; }
		T *end() const 							{ return fData + fSize; }
		
		//Returns searchEnd if target is not found; uses == for equality comparison
		uint32_t find(uint32_t searchStart, uint32_t searchEnd, const T &target)			{ return find<EqualOp>(searchStart, searchEnd, target); }
		
		//Allows you to specify an equality comparison functor
		template<class Eq>
		uint32_t find(uint32_t searchStart, uint32_t searchEnd, const T &target, Eq eq = Eq())
		{
            uint32_t i = searchStart;
            for(; i < searchEnd; ++i)
                if (eq(target, fData[i]))
                    break;
            return i;
		}

		//returns size() if the element is not found
		uint32_t find(const T &target)					{ return find<EqualOp>(target); }

		template<class Eq>
		uint32_t find(const T &target, Eq eq = Eq())      { return find(0, size(), target, eq); }

		//Doubles the capacity as needed
		void push_back(const T &newItem)
		{
			reserve(fSize + 1);
			NEW (fData + fSize) T(newItem);
			fSize++;
		}

		void pop_back() 								{ fData[--fSize].~T(); }
		
		void swap(SVector &rhs)
		{
			uint32_t tmpCapacity = fCapacity;
			uint32_t tmpSize = fSize;
			T *tmpData = fData;
			fCapacity = rhs.fCapacity;
			fSize = rhs.fSize;
			fData = rhs.fData;
			rhs.fCapacity = tmpCapacity;
			rhs.fSize = tmpSize;
			rhs.fData = tmpData;
		}
		
		void insert(uint32_t position, const T &newItem)					{ insert(position, 1, newItem); }

		void insert(uint32_t position, uint32_t count, const T &newItem)
		{
			reserve(fSize + count);
			for(uint32_t i = fSize; i > position; --i)
			{
				NEW (fData + i - 1 + count) T(fData[i - 1]);
				fData[i - 1].~T();
			}
			for(uint32_t i = position; i < position + count; ++i)
				NEW (fData + i) T(newItem);
			fSize += count;
		}

		//can accept count of 0 - which results in a NOP
		void erase(uint32_t position, uint32_t count = 1)
		{
			if(count == 0)
				return;
			for(uint32_t i = position; i < position + count; ++i)
				fData[i].~T();
			for(uint32_t i = position + count; i < fSize; ++i)
			{
				NEW (fData + i - count) T(fData[i]);
				fData[i].~T();
			}
			fSize -= count;
		}
		
		//Removes 1 element by swapping it with the last item.
		void swap_erase(uint32_t position)
		{
			fData[position].~T();
			if (position < --fSize)
			{
				NEW (fData + position) T(fData[fSize]);
				fData[fSize].~T();
			}
		}

		bool empty() const 							{ return fSize == 0; }
		uint32_t capacity() const 						{ return fCapacity; }
		uint32_t size() const 							{ return fSize; }
		
		void clear() 									{ resize(0); }

		//unlike clear(), this will free the memories
		void wipe()
		{
			this->~SVector();
			fCapacity = fSize = 0;
			fData = NULL;
		}

		//Doubles the capacity on a reallocation to preserve linear time semantics
		void reserve(uint32_t newCapacity)
		{
			if (newCapacity > fCapacity)
			{
				uint32_t targetCapacity = fCapacity == 0 ? 4 : fCapacity;
				while(targetCapacity < newCapacity)
					targetCapacity *= 2;
				reserveImpl(targetCapacity);
			}
		}

		void resize(uint32_t newSize, const T &newItem = T())
		{
			if (newSize > fSize)
			{
				reserve(newSize);
				for(uint32_t i = fSize; i < newSize; ++i)
					NEW (fData + i) T(newItem);
			}
			else if (newSize < fSize)
			{
				for(uint32_t i = newSize; i < fSize; ++i)
					fData[i].~T();
			}
			fSize = newSize;
		}

	private:
		void reserveImpl(uint32_t newCapacity)
		{
			T *newData = static_cast<T *>(operator new[](sizeof(T) * newCapacity));
			fCapacity = newCapacity;
			for(uint32_t i = 0; i < fSize; ++i)
				NEW (newData + i) T(fData[i]);
			operator delete[](fData);
			fData = newData;
		}

		uint32_t fCapacity;
		uint32_t fSize;
		T *fData;	
		
		struct EqualOp
		{
			bool operator()(const T &left, const T &right)	{ return left == right; }
		};
};

#endif //_SVECTOR_H_

