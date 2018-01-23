#pragma once
#include <unordered_map>
#include <memory>
#include <common.h>
#include <moodycamel/blockingconcurrentqueue.h>
#include <atomic>
#include <type_traits>
#include <GUtil.h>
#include <CachePersister.h>

enum CacheOptions
  {
    DOPERSIST = 1,
    DOPUBLISH = 2,
    LOADNOW = 4,
    DOASYNC = 8
  };

template<typename Key, typename Type>
  class SCache
{
 private:
  std::unordered_map<Key, Type> _map;
  std::unique_ptr<CachePersister> _persister;
  int _cacheOptions = 0;
  std::shared_mutex _sharedMutex;
  std::mutex _persistMutex;

 public:
  SCache() = default;
  SCache(const SCache&) = default;
  SCache(SCache&&) = delete;
  SCache& operator=(const SCache&) = default;
  EventHandler<Type> ItemChanged;

  typedef typename std::unordered_map<Key, Type>::iterator iterator;
  typedef typename std::unordered_map<Key, Type>::const_iterator const_iterator;

  iterator begin() { return _map.begin(); }
  const_iterator begin() const { return _map.begin(); }
  iterator end() { return _map.end(); }
  const_iterator end() const { return _map.end(); }

  bool empty() const { return _map.empty(); }
  size_t size() const { return _map.size(); }

  // non thread safe entries.. use carefully
  template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
      return _map.emplace(std::forward<Args>(args)...);
    }

  // thread safe entries
  bool GetByKey(const Key& key, Type& value)
  {
    std::shared_lock<std::shared_mutex> lock(_sharedMutex);
    auto iter = _map.find(key);
    if (iter == _map.end())
      return false;
    value = iter->second;
    return true;
  }

  Type& GetByKey(const Key& key)
    {
      std::unique_lock<std::shared_mutex> lock(_sharedMutex);
      return _map[key];
    }

  void PutByKey(Key& key, Type& value, bool raiseEvent=true,bool persistNow=true)
  {
    {
      std::unique_lock<std::shared_mutex> lock(_sharedMutex);
      _map[key] = value;
    }
    if (raiseEvent)
      ItemChanged.raise(value);

    if (!_persister)
      return;

    if (persistNow)
      if (_cacheOptions & CacheOptions::DOASYNC )
	{
	  _persister->persistMessage(key, value);
	}
      else
	{
	  // persist and publish in the same thread .. (slow)
	  Json vj;
	  std::string vs;
	  {
	    dump(get_ref<Type>{}(value), vj);
	    vs = vj.dump();
	  }
	  if (_persister->shouldPersist())
	    GUtil::instance().set(_persister->getPersistStream(), get_value<Key>{}(key), vs);
	  if (_persister->shouldPublish())
	    GUtil::instance().publish(_persister->getPersistStream(), vs);
	}
  }

  void ClearAll() { _map.clear(); }
  void setPersistence(const std::string& stream, int cacheOptions = (CacheOptions::LOADNOW | CacheOptions::DOASYNC | CacheOptions::DOPERSIST))
  {
    _persister.reset(new CachePersister(stream, ((cacheOptions & CacheOptions::DOPERSIST) != 0), ((cacheOptions & CacheOptions::DOPUBLISH) != 0)));
    if (cacheOptions & CacheOptions::LOADNOW)
      {
	loadFromCache();
      }
    _cacheOptions = cacheOptions;
  }

  void copyFrom(const SCache<Key, Type>& cache)
  {
    for (auto& elem : cache)
      {
	_map[elem.first] = elem.second;
      }
  }

 private:
  void loadFromCache()
  {
    int count = 0;
    _persister->loadMsg([&](const std::string& key, const std::string& val) {
	Type obj;
	load(get_Obj<Type>{}(obj), val);
	_map[get_actual<Key>{}(key)] = obj;
	++count;
      });
    info("Loaded {0} records from cache {1}", count, _persister->getPersistStream());
  }
};

template <typename Type>
using SSCache = SCache<std::string, Type>;

