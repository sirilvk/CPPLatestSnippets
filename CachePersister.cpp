#include <CachePersister.h>

std::atomic_bool CachePersister::_persisterThStarted = ATOMIC_VAR_INIT(false);;
moodycamel::BlockingConcurrentQueue<QMsg> CachePersister::_msgqueue;

void CachePersister::persistThFunc()
{
    QMsg val;
    ThreadInit persistThInit("Cache Persister Thread");
    // use cpp_redis to get inst
    while (!util::getShouldExit())
    {
	_msgqueue.wait_dequeue(val);
	if (val._pvalue != nullptr)
	{
	    // do the serialization here
	    std::string value;
	    val._serializer(val._pvalue, value);
	    if (val._shouldPersist)
		inst.set(val._stream, val._key, value);
	    if (val._shouldPublish)
		inst.publish(val._stream, value);
	}
	else
	{
	    if (val._shouldPersist)
		inst.set(val._stream, val._key, val._value);
	    if (val._shouldPublish)
		inst.publish(val._stream, val._value);
	}
    }
}

CachePersister::CachePersister(const std::string& stream, bool doPersist/* = true*/, bool doPublish/* = false*/) :_persistStream(stream), _shouldPersist(doPersist), _shouldPublish(doPublish)
{
    bool expected = false;
    if (_persisterThStarted.compare_exchange_strong(expected, true))
    {
	_persisterThStarted = true;
	// start the persistance thread.. This thread will asynchronously persist the Cache data onto the Redis Cache
	std::thread persistTh(&CachePersister::persistThFunc);
	persistTh.detach();
    }
}

void CachePersister::writeMsg(const QMsg& msg)
{
    _msgqueue.enqueue(msg);
}

void CachePersister::loadMsg(std::function<void(const std::string& key, const std::string& val)> loadFunc)
{
    // load all the values persisted into redis
    //loadAll(_persistStream, loadFunc);
}
