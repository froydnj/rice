#include "unittest.hpp"
#include "embed_ruby.hpp"
#include <rice/rice.hpp>

#include <memory>

using namespace Rice;

TESTSUITE(Ownership);

namespace
{
  class MyClass
  {
  public:
    static inline uint32_t constructorCalls = 0;
    static inline uint32_t copyConstructorCalls = 0;
    static inline uint32_t moveConstructorCalls = 0;
    static inline uint32_t destructorCalls = 0;
    static inline uint32_t methodCalls = 0;

    static void reset()
    {
      constructorCalls = 0;
      copyConstructorCalls = 0;
      moveConstructorCalls = 0;
      destructorCalls = 0;
      methodCalls = 0;
    }

  public:
    int flag = 0;

  public:
    MyClass()
    {
      constructorCalls++;
    }

    ~MyClass()
    {
      destructorCalls++;
    }

    MyClass(const MyClass& other): flag(other.flag)
    {
      copyConstructorCalls++;
    }

    MyClass(MyClass&& other) : flag(other.flag)
    {
      moveConstructorCalls++;
    }

    int32_t process()
    {
      methodCalls++;
      return methodCalls;
    }

    void setFlag(int value)
    {
      this->flag = value;
    }
  };

  class Factory
  {
  public:
    static void reset()
    {
      delete Factory::instance_;
      Factory::instance_ = nullptr;
    }

  public:
    MyClass* transferPointer()
    {
      return new MyClass();
    }

    MyClass* keepPointer()
    {
      return this->instance();
    }

    MyClass& keepReference()
    {
      return *this->instance();
    }

    MyClass value()
    {
      return MyClass();
    }

    MyClass moveValue()
    {
      return std::move(MyClass());
    }

    MyClass* instance()
    {
      if (!instance_)
      {
        instance_ = new MyClass();
      }
      return instance_;
    }

  public:
    static inline MyClass* instance_ = nullptr;
  };
}

SETUP(Ownership)
{
  embed_ruby();

  define_class<MyClass>("MyClass").
    define_method("process", &MyClass::process).
    define_method("set_flag", &MyClass::setFlag);

  define_class<Factory>("Factory").
    define_constructor(Constructor<Factory>()).
    define_method("value", &Factory::value).
    define_method("move_value", &Factory::moveValue).
    define_method("transfer_pointer", &Factory::transferPointer).
    define_method("keep_pointer", &Factory::keepPointer, Return(false)).
    define_method("copy_reference", &Factory::keepReference).
    define_method("keep_reference", &Factory::keepReference, Return(false));
}

TESTCASE(TransferPointer)
{
  Factory::reset();
  MyClass::reset();

  Module m = define_module("TestingModule");

  std::string code = R"(factory = Factory.new
                        10.times do |i|
                          my_class = factory.transfer_pointer
                          my_class.set_flag(i)
                          my_class = nil
                        end
                        GC.start)";

  m.instance_eval(code);

  ASSERT_EQUAL(10, MyClass::constructorCalls);
  ASSERT_EQUAL(0, MyClass::copyConstructorCalls);
  ASSERT_EQUAL(0, MyClass::moveConstructorCalls);
  ASSERT_EQUAL(10, MyClass::destructorCalls);
  ASSERT(!Factory::instance_);
}

TESTCASE(KeepPointer)
{
  Factory::reset();
  MyClass::reset();

  Module m = define_module("TestingModule");

  // Create ruby objects that point to the same instance of MyClass
  std::string code = R"(factory = Factory.new
                        10.times do |i|
                          my_class = factory.keep_pointer
                          my_class.set_flag(i)
                        end
                        GC.start)";

  m.instance_eval(code);

  ASSERT_EQUAL(1, MyClass::constructorCalls);
  ASSERT_EQUAL(0, MyClass::copyConstructorCalls);
  ASSERT_EQUAL(0, MyClass::moveConstructorCalls);
  ASSERT_EQUAL(0, MyClass::destructorCalls);
  ASSERT_EQUAL(9, Factory::instance_->flag);
}

TESTCASE(KeepReference)
{
  Factory::reset();
  MyClass::reset();

  Module m = define_module("TestingModule");

  // Create ruby objects that point to the same instance of MyClass
  std::string code = R"(factory = Factory.new
                        10.times do |i|
                          my_class = factory.keep_reference
                          my_class.set_flag(i)
                        end
                        GC.start)";

  m.instance_eval(code);

  ASSERT_EQUAL(1, MyClass::constructorCalls);
  ASSERT_EQUAL(0, MyClass::copyConstructorCalls);
  ASSERT_EQUAL(0, MyClass::moveConstructorCalls);
  ASSERT_EQUAL(0, MyClass::destructorCalls);
  ASSERT_EQUAL(9, Factory::instance_->flag);
}

TESTCASE(CopyReference)
{
  Factory::reset();
  MyClass::reset();

  Module m = define_module("TestingModule");

  // Create ruby objects that point to the same instance of MyClass
  std::string code = R"(factory = Factory.new
                        10.times do |i|
                          my_class = factory.copy_reference
                          my_class.set_flag(i)
                        end
                        GC.start)";

  m.instance_eval(code);

  ASSERT_EQUAL(1, MyClass::constructorCalls);
  ASSERT_EQUAL(10, MyClass::copyConstructorCalls);
  ASSERT_EQUAL(0, MyClass::moveConstructorCalls);
  ASSERT_EQUAL(10, MyClass::destructorCalls);
  ASSERT_EQUAL(0, Factory::instance_->flag);
}

TESTCASE(TransferValue)
{
  Factory::reset();
  MyClass::reset();

  Module m = define_module("TestingModule");

  // Create ruby objects that point to the same instance of MyClass
  std::string code = R"(factory = Factory.new
                        10.times do |i|
                          my_class = factory.value
                          my_class.set_flag(i)
                        end
                        GC.start)";

  m.instance_eval(code);

  ASSERT_EQUAL(10, MyClass::constructorCalls);
  ASSERT_EQUAL(10, MyClass::copyConstructorCalls);
  ASSERT_EQUAL(0, MyClass::moveConstructorCalls);
  ASSERT_EQUAL(20, MyClass::destructorCalls);
  ASSERT(!Factory::instance_);
}

TESTCASE(MoveValue)
{
  Factory::reset();
  MyClass::reset();

  Module m = define_module("TestingModule");

  // Create ruby objects that point to the same instance of MyClass
  std::string code = R"(factory = Factory.new
                        10.times do |i|
                          my_class = factory.move_value
                          my_class.set_flag(i)
                        end
                        GC.start)";

  m.instance_eval(code);

  ASSERT_EQUAL(10, MyClass::constructorCalls);
  ASSERT_EQUAL(10, MyClass::copyConstructorCalls);
  ASSERT_EQUAL(10, MyClass::moveConstructorCalls);
  ASSERT_EQUAL(30, MyClass::destructorCalls);
  ASSERT(!Factory::instance_);
}
