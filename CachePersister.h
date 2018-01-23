#pragma once
#include <unordered_map>
#include <memory>
#include <common.h>
#include <moodycamel/blockingconcurrentqueue.h>
#include <atomic>
#include <type_traits>

template <bool> struct selectFn {};
typedef selectFn<true> TrueCond;
typedef selectFn<false> FalseCond;

template <typename Type>
struct is_shared_ptr : std::false_type {};
template <typename Type>
struct is_shared_ptr<std::shared_ptr<Type>> : std::true_type {};

struct QMsg
{
  std::string _stream;
  std::string _key;
  std::string _value;
  std::shared_ptr<void> _pvalue;
  std::function<void(std::shared_ptr<void>&, std::string&)> _serializer = nullptr;
  bool _shouldPublish;
  bool _shouldPersist;
};

class EXPORTIT CachePersister
{
 private:
#pragma warning(push)
#pragma warning(disable:4251)
  std::string _persistStream;
  static std::atomic_bool _persisterThStarted;
  bool _shouldPublish = false;
  bool _shouldPersist = false;
  static moodycamel::BlockingConcurrentQueue<QMsg> _msgqueue;
  static void persistThFunc(); // will loop over the msgqueue and will publish to redis whenever it gets a message
#pragma warning(pop)

 public:
  CachePersister(const std::string& stream, bool doPersist = true, bool doPublish = false);
  void writeMsg(const QMsg& msg); // will be async
  void loadMsg(std::function<void(const std::string& key, const std::string& val)> loadFunc);
  std::string& getPersistStream() { return _persistStream; }
  bool shouldPublish() { return _shouldPublish; }
  bool shouldPersist() { return _shouldPersist; }

  template <typename Key, typename Type>
    void persistMessage(Key& key, Type& value)
  {
#ifdef _WIN32
    persistMessageImpl(key, value, selectFn<is_shared_ptr<Type>::value>());
#else
    persistMessageImpl(key, value);
#endif
  }

 private:
#ifdef _WIN32
  template<typename Key, typename Type>
    void persistMessageImpl(Key& key, Type& value, FalseCond)
  {
    QMsg q;
    q._stream = getPersistStream();
    q._key = get_value<Key>{}(key);
    Json vj;
    dump(get_ref<Type>{}(value), vj);
    q._value = vj.dump();
    q._pvalue = nullptr;
    q._serializer = nullptr;
    q._shouldPublish = _shouldPublish;
    q._shouldPersist = _shouldPersist;
    writeMsg(q); // this value must be serializable (to json)
  }

  template<typename Key, typename Type>
    void persistMessageImpl(Key& key, Type& value, TrueCond)
  {
    QMsg q;
    q._stream = getPersistStream();
    q._key = get_value<Key>{}(key);
    q._value = "";
    q._pvalue = value;
    q._serializer = [&](std::shared_ptr<void>& obj, std::string& val) {
      auto actualVal = reinterpret_pointer_cast<Type::element_type>(obj);
      Json j;
      dump(get_ref<Type>{}(actualVal), j);
      val = j.dump();
    };
    q._shouldPublish = _shouldPublish;
    q._shouldPersist = _shouldPersist;
    // the persister thread will take care of the serialization
    writeMsg(q); // this value must be serializable (to json)
  }

#else
  // SFINAE at work
  template<typename Key, typename Type>
    typename std::enable_if_t<!is_shared_ptr<Type>::value> persistMessageImpl(Key& key, Type& value)
  {
    QMsg q;
    q._stream = getPersistStream();
    q._key = get_value<Key>{}(key);
    Json vj;
    dump(get_ref<Type>{}(value), vj);
    q._value = vj.dump();
    q._pvalue = nullptr;
    q._serializer = nullptr;
    q._shouldPublish = _shouldPublish;
    q._shouldPersist = _shouldPersist;
    writeMsg(q); // this value must be serializable (to json)
  }

  template<typename Key, typename Type>
    typename std::enable_if_t<is_shared_ptr<Type>::value> persistMessageImpl(Key& key, Type& value)
  {
    QMsg q;
    q._stream = getPersistStream();
    q._key = get_value<Key>{}(key);
    q._value = "";
    q._pvalue = value;
    q._serializer = [&](std::shared_ptr<void>& obj, std::string& val) {
      auto actualVal = reinterpret_pointer_cast<typename Type::element_type>(obj);
      Json j;
      dump(get_ref<Type>{}(actualVal), j);
      val = j.dump();
    };
    q._shouldPublish = _shouldPublish;
    q._shouldPersist = _shouldPersist;
    // the persister thread will take care of the serialization
    writeMsg(q); // this value must be serializable (to json)
  }
#endif
};

