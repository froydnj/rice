#ifndef Rice__Data_Type__ipp_
#define Rice__Data_Type__ipp_

#include "detail/method_data.hpp"
#include "detail/NativeAttribute.hpp"
#include "detail/default_allocation_func.hpp"
#include "detail/TypeRegistry.hpp"
#include "detail/Wrapper.hpp"
#include "detail/NativeIterator.hpp"
#include "cpp_api/Class.hpp"
#include "cpp_api/String.hpp"

#include <stdexcept>

namespace Rice
{
  template<typename T>
  void ruby_mark_internal(detail::Wrapper* wrapper)
  {
    // Tell the wrapper to mark the objects its keeping alive
    wrapper->ruby_mark();

    // Get the underlying data and call custom mark function (if any)
    T* data = static_cast<T*>(wrapper->get());
    ruby_mark<T>(data);
  }

  template<typename T>
  void ruby_free_internal(detail::Wrapper* wrapper)
  {
    delete wrapper;
  }

  template<typename T>
  size_t ruby_size_internal(const T* data)
  {
    return sizeof(T);
  }

  template<typename T>
  template <typename Base_T>
  inline Data_Type<T> Data_Type<T>::bind(const Module& klass)
  {
    if (is_bound())
    {
      std::string message = "Type " + detail::typeName(typeid(T)) + " is already bound to a different type";
      throw std::runtime_error(message.c_str());
    }

    klass_ = klass;

    rb_data_type_ = new rb_data_type_t();
    rb_data_type_->wrap_struct_name = strdup(Rice::detail::protect(rb_class2name, klass_));
    rb_data_type_->function.dmark = reinterpret_cast<void(*)(void*)>(&Rice::ruby_mark_internal<T>);
    rb_data_type_->function.dfree = reinterpret_cast<void(*)(void*)>(&Rice::ruby_free_internal<T>);
    rb_data_type_->function.dsize = reinterpret_cast<size_t(*)(const void*)>(&Rice::ruby_size_internal<T>);
    rb_data_type_->data = nullptr;
    rb_data_type_->flags = RUBY_TYPED_FREE_IMMEDIATELY;

    if constexpr (!std::is_void_v<Base_T>)
    {
      rb_data_type_->parent = Data_Type<Base_T>::ruby_data_type();
    }

    // Now register with the type registry
    detail::Registries::instance.types.add<T>(klass_, rb_data_type_);

    for (typename Instances::iterator it = unbound_instances().begin(),
      end = unbound_instances().end();
      it != end;
      unbound_instances().erase(it++))
    {
      (*it)->set_value(klass);
    }

    return Data_Type<T>();
  }

  template<typename T>
  inline void Data_Type<T>::unbind()
  {
    detail::Registries::instance.types.remove<T>();

    if (klass_ != Qnil)
    {
      klass_ = Qnil;
    }

    // There could be objects floating around using the existing rb_type so 
    // do not delete it. This is of course a memory leak.
    rb_data_type_ = nullptr;
  }

  template<typename T>
  inline Data_Type<T>::Data_Type() : Class(klass_ == Qnil ? rb_cObject : klass_)
  {
    if (!is_bound())
    {
      unbound_instances().insert(this);
    }
  }

  template<typename T>
  inline Data_Type<T>::Data_Type(Module const& klass) : Class(klass)
  {
    this->bind(klass);
  }

  template<typename T>
  inline Data_Type<T>::~Data_Type()
  {
    unbound_instances().erase(this);
  }

  template<typename T>
  inline rb_data_type_t* Data_Type<T>::ruby_data_type()
  {
    check_is_bound();
    return rb_data_type_;
  }

  template<typename T>
  inline Class Data_Type<T>::klass()
  {
    check_is_bound();
    return klass_;
  }

  template<typename T>
  inline Data_Type<T>& Data_Type<T>::operator=(Module const& klass)
  {
    this->bind(klass);
    return *this;
  }

  template<typename T>
  template<typename Constructor_T, typename...Arg_Ts>
  inline Data_Type<T>& Data_Type<T>::define_constructor(Constructor_T constructor, Arg_Ts const& ...args)
  {
    check_is_bound();

    // Define a Ruby allocator which creates the Ruby object
    detail::protect(rb_define_alloc_func, static_cast<VALUE>(*this), detail::default_allocation_func<T>);

    // Define an initialize function that will create the C++ object
    this->define_method("initialize", &Constructor_T::construct, args...);

    return *this;
  }

  template<typename T>
  template<typename Director_T>
  inline Data_Type<T>& Data_Type<T>::define_director()
  {
    if (!detail::Registries::instance.types.isDefined<Director_T>())
    {
      Data_Type<Director_T>::bind(*this);
    }

    // TODO - hack to fake Ruby into thinking that a Director is
    // the same as the base data type
    Data_Type<Director_T>::rb_data_type_ = Data_Type<T>::rb_data_type_;
    return *this;
  }

  template<typename T>
  inline bool Data_Type<T>::is_bound()
  {
    return klass_ != Qnil;
  }

  template<typename T>
  inline bool Data_Type<T>::is_descendant(VALUE value)
  {
    check_is_bound();
    return detail::protect(rb_obj_is_kind_of, value, klass_) == Qtrue;
  }

  template<typename T>
  inline void Data_Type<T>::check_is_bound()
  {
    if (!is_bound())
    {
      std::string message = "Type " + detail::typeName(typeid(T)) + " is not bound";
      throw std::runtime_error(message.c_str());
    }
  }

  template<typename T, typename Base_T>
  inline Data_Type<T> define_class_under(Object module, char const* name)
  {
    if (detail::Registries::instance.types.isDefined<T>())
    {
      return Data_Type<T>();
    }
    
    Class superKlass;

    if constexpr (std::is_void_v<Base_T>)
    {
      superKlass = rb_cObject;
    }
    else
    {
      superKlass = Data_Type<Base_T>::klass();
    }
    
    Class c = define_class_under(module, name, superKlass);
    c.undef_creation_funcs();
    return Data_Type<T>::template bind<Base_T>(c);
  }

  template<typename T, typename Base_T>
  inline Data_Type<T> define_class(char const* name)
  {
    if (detail::Registries::instance.types.isDefined<T>())
    {
      return Data_Type<T>();
    }

    Class superKlass;
    if constexpr (std::is_void_v<Base_T>)
    {
      superKlass = rb_cObject;
    }
    else
    {
      superKlass = Data_Type<Base_T>::klass();
    }

    Class c = define_class(name, superKlass);
    c.undef_creation_funcs();
    return Data_Type<T>::template bind<Base_T>(c);
  }

  template<typename T>
  template<typename Iterator_Funct_T>
  inline Data_Type<T>& Data_Type<T>::define_iterator(Iterator_Funct_T begin, Iterator_Funct_T end, Identifier name)
  {
    using Iter_T = detail::NativeIterator<T, Iterator_Funct_T>;
    Iter_T* iterator = new Iter_T(name, begin, end);
    detail::MethodData::define_method(Data_Type<T>::klass(), name, (RUBY_METHOD_FUNC)iterator->call, 0, iterator);

    this->klass().include_module(rb_mEnumerable);

    return *this;
  }

  template <typename T>
  template <typename Attr_T>
  inline Data_Type<T>& Data_Type<T>::define_attr(std::string name, Attr_T attr, AttrAccess access)
  {
    auto* native = detail::Make_Native_Attribute(attr, access);
    using Native_T = typename std::remove_pointer_t<decltype(native)>;

    detail::verifyType<typename Native_T::Native_Return_T>();

    if (access == AttrAccess::ReadWrite || access == AttrAccess::Read)
    {
      detail::MethodData::define_method( klass_, Identifier(name).id(),
        RUBY_METHOD_FUNC(&Native_T::get), 0, native);
    }

    if (access == AttrAccess::ReadWrite || access == AttrAccess::Write)
    {
      if (std::is_const_v<std::remove_pointer_t<Attr_T>>)
      {
        throw std::runtime_error(name + " is readonly");
      }

      detail::MethodData::define_method( klass_, Identifier(name + "=").id(),
        RUBY_METHOD_FUNC(&Native_T::set), 1, native);
    }

    return *this;
  }

  template <typename T>
  template <typename Attr_T>
  inline Data_Type<T>& Data_Type<T>::define_singleton_attr(std::string name, Attr_T attr, AttrAccess access)
  {
    auto* native = detail::Make_Native_Attribute(attr, access);
    using Native_T = typename std::remove_pointer_t<decltype(native)>;

    detail::verifyType<typename Native_T::Native_Return_T>();

    if (access == AttrAccess::ReadWrite || access == AttrAccess::Read)
    {
      VALUE singleton = detail::protect(rb_singleton_class, this->value());
      detail::MethodData::define_method(singleton, Identifier(name).id(),
        RUBY_METHOD_FUNC(&Native_T::get), 0, native);
    }

    if (access == AttrAccess::ReadWrite || access == AttrAccess::Write)
    {
      if (std::is_const_v<std::remove_pointer_t<Attr_T>>)
      {
        throw std::runtime_error(name  + " is readonly");
      }

      VALUE singleton = detail::protect(rb_singleton_class, this->value());
      detail::MethodData::define_method(singleton, Identifier(name + "=").id(),
        RUBY_METHOD_FUNC(&Native_T::set), 1, native);
    }

    return *this;
  }

  template <typename T>
  template<bool IsMethod, typename Function_T>
  inline void Data_Type<T>::wrap_native_call(VALUE klass, Identifier name, Function_T&& function, MethodInfo* methodInfo)
  {
    // Make sure the return type and arguments have been previously seen by Rice
    using traits = detail::method_traits<Function_T, IsMethod>;
    detail::verifyType<typename traits::Return_T>();
    detail::verifyTypes<typename traits::Arg_Ts>();

    // Create a NativeFunction instance to wrap this native call and 
    auto* native = new detail::NativeFunction<T, Function_T, IsMethod>(std::forward<Function_T>(function), methodInfo);

    // Now define the method
    using Native_T = typename std::remove_pointer_t<decltype(native)>;
    detail::MethodData::define_method(klass, name.id(), &Native_T::call, -1, native);
  }
}
#endif