#include "SimplCache.h"
#define CATCH_CONFIG_MAIN
#include "catch.h"

static int TestSimplCache(int i)
{


	return i;
}

TEST_CASE("SimplCache Unit Test", "[SimplCache UT]") {

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


	TICstruct tmem1("test1", "test1"), tmem2("test2", "test2");
	SimplCache<TICstruct> tCache;
	tCache.PutByKey(tmem1);
	tCache.PutByKey(tmem2);

	TICstruct tmem3;
	REQUIRE(tCache.size() == 2);
	REQUIRE(tCache.GetByKey("test1", tmem3) == true);
	REQUIRE(tmem3 == tmem1);
	REQUIRE(tCache.GetByKey("test4", tmem3) == false);


	SimplCache<std::shared_ptr<TICstruct>> stCache;
	std::shared_ptr<TICstruct> sptr(std::make_shared<TICstruct>("stest1", "stest1"));
	std::shared_ptr<TICstruct> sptr1;
	stCache.PutByKey(sptr);

	REQUIRE(stCache.size() == 1);
	REQUIRE(stCache.GetByKey("stest1", sptr1) == true);
	REQUIRE(sptr1 == sptr);
}
