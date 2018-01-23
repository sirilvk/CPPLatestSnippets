#include <SCache.h>
#include <catch.h>

struct TICstruct
{
    std::string key;
    std::string value;
    TICstruct() = default;
    TICstruct(const std::string& _key, const std::string& _value) :key(_key), value(_value)
	{}

    std::string& GetKey() { return value; }
    bool operator==(const TICstruct& ts) { return (this->key == ts.key && this->value == ts.value); }
};

struct TICstruct2
{
    std::string key;
    int val;
    std::string val2;
    std::string value;
    TICstruct2() = default;
    TICstruct2(const std::string& _key, int _val, const std::string& _val2, const std::string& _value) :key(_key), val(_val), val2(_val2), value(_value)
	{}

    std::string& GetKey() { return value; }
    bool operator==(const TICstruct& ts) { return (this->key == ts.key && this->value == ts.value); }
};

void dump(const TICstruct& obj, Json& val)
{
    val["key"] = obj.key;
    val["value"] = obj.value;
}

void load(TICstruct& obj, const std::string& val)
{
    Json j;
    j = Json::parse(val.c_str());
    obj.key = util::getOpt<std::string>(j, "key", "");
    obj.value = util::getOpt<std::string>(j, "value", "");
}

void dump(const TICstruct2& obj, Json& val)
{
    val["key"] = obj.key;
    val["val"] = obj.val;
    val["val2"] = obj.val2;
    val["value"] = obj.value;
}

void load(TICstruct2& obj, const std::string& val)
{
    Json j;
    j = Json::parse(val.c_str());
    obj.key = util::getOpt<std::string>(j, "key", "");
    obj.val = util::getOpt<int>(j, "key", 0);
    obj.val2 = util::getOpt<std::string>(j, "key", "");
    obj.value = util::getOpt<std::string>(j, "value", "");
}

TEST_CASE("SCache Unit Test", "[SCache UT]") {
    TICstruct tmem1("test1", "test1"), tmem2("test2", "test2");
    SCache<std::string, TICstruct> tCache;
    tCache.setPersistence("TestingSCache");
    tCache.PutByKey(tmem1.key, tmem1);
    tCache.PutByKey(tmem2.key, tmem2);

    TICstruct tmem3;
    REQUIRE(tCache.size() == 2);
    REQUIRE(tCache.GetByKey("test1", tmem3) == true);
    REQUIRE(tmem3 == tmem1);
    REQUIRE(tCache.GetByKey("test4", tmem3) == false);


    SCache<std::string, std::shared_ptr<TICstruct>> stCache;
    stCache.setPersistence("TestingSCache");
    std::shared_ptr<TICstruct> sptr(std::make_shared<TICstruct>("stest1", "stest1"));
    std::shared_ptr<TICstruct> sptr1;
    std::shared_ptr<TICstruct2> sptr3;
    stCache.PutByKey(sptr->key, sptr);

    // testing deffer serialization
    std::shared_ptr<TICstruct2> sptr2(std::make_shared<TICstruct2>("stest2", 20, "stest2", "stest2"));
    SCache<std::string, std::shared_ptr<TICstruct2>> stCache2;
    stCache2.setPersistence("TestingSCache2");
    stCache2.PutByKey(sptr2->key, sptr2);

    REQUIRE(stCache.size() == 1);
    REQUIRE(stCache.GetByKey("stest1", sptr1) == true);
    REQUIRE(sptr1 == sptr);
}
