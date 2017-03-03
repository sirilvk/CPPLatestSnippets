#pragma once
#include <unordered_map>
#include <memory>
#include <is_shared_ptr.h>

template<typename Key, typename Type>
class SimplCache_T
{
private:
	typedef typename std::unordered_map<Key, Type>::iterator CacheIterator;
	std::unordered_map<Key, Type> cache;

#ifdef _WIN32
	template <bool> struct selectFn {};
	typedef selectFn<true> TrueCond;
	typedef selectFn<false> FalseCond;

	void PutByKeyInternal(Type& value, TrueCond)
	{
		cache.insert(std::make_pair(value->GetKey(), value));
	}

	void PutByKeyInternal(Type& value, FalseCond)
	{
		cache.insert(std::make_pair(value.GetKey(), value));
	}
#endif

public:
	SimplCache_T() = default;
	SimplCache_T(const SimplCache_T&) = delete;
	SimplCache_T(SimplCache_T&&) = delete;
	SimplCache_T& operator=(const SimplCache_T&) = delete;

	void ClearAll()
	{
		cache.clear();
	}

	CacheIterator begin()
	{
		return cache.begin();
	}

	CacheIterator end()
	{
		return cache.end();
	}

	bool GetByKey(const Key& key, Type& value)
	{
		auto iter = cache.find(key);
		if (iter == cache.end())
			return false;
		value = iter->second;
		return true;
	}

#ifdef _WIN32

	// Using tag dispatch pattern instead
	//http://stackoverflow.com/questions/6917079/tag-dispatch-versus-static-methods-on-partially-specialised-classes
	void PutByKey(Type& value)
	{
		PutByKeyInternal(value, selectFn<is_shared_ptr<Type>::value>());
	}

#else
	// SFINAE below works perfectly fine for linux but as usual MSVC fails to understand that...
	
	typename std::enable_if<!is_shared_ptr<Type>::value, void>::type PutByKey(Type& value)
	{
		cache.insert(std::make_pair(value.GetKey(), value));
	}

	typename std::enable_if<is_shared_ptr<Type>::value, void>::type PutByKey(Type& value)
	{
		cache.insert(std::make_pair(value->GetKey(), value));
	}
	
#endif

	unsigned int size()
	{
		return cache.size();
	}
};

template <typename T>
using SimplCache = SimplCache_T<std::string, T>;
