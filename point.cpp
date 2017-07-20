#include <iostream>
#include <type_traits>

template<typename T>
struct Point
{
  template<typename U = T>
  void MyFunction(typename std::enable_if<std::is_same<U, int>::value, U >::type* = 0)
  {
    std::cout << "T is int." << std::endl;
  }

  template<typename U = T>
  void MyFunction(typename std::enable_if<!std::is_same<U, int>::value, float >::type* = 0)
  {
    std::cout << "T is not int." << std::endl;
  }
};

int main()
{
    Point<int> intPoint;
    intPoint.MyFunction();

    Point<float> floatPoint;
    floatPoint.MyFunction();
}
