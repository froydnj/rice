#ifndef Rice__hpp_
#define Rice__hpp_

#include "detail/ruby.hpp"
#include "traits/rice_traits.hpp"
#include "traits/function_traits.hpp"
#include "traits/method_traits.hpp"
#include "traits/attribute_traits.hpp"
#include "Exception_defn.hpp"
#include "detail/Jump_Tag.hpp"
#include "detail/RubyFunction.hpp"
#include "detail/ExceptionHandler.hpp"
#include "detail/Type.hpp"
#include "detail/TypeRegistry.hpp"
#include "detail/InstanceRegistry.hpp"
#include "detail/HandlerRegistry.hpp"
#include "detail/NativeRegistry.hpp"
#include "detail/Registries.hpp"
#include "detail/cpp_protect.hpp"
#include "detail/default_allocation_func.hpp"
#include "detail/Wrapper.hpp"
#include "Return.hpp"
#include "Arg.hpp"
#include "detail/MethodInfo.hpp"
#include "detail/from_ruby.hpp"
#include "detail/to_ruby.hpp"
#include "Exception.ipp"
#include "detail/self.hpp"
#include "Identifier.hpp"
#include "detail/NativeAttribute.hpp"
#include "detail/NativeFunction.hpp"
#include "detail/NativeIterator.hpp"

#include "ruby_mark.hpp"


#include "cpp_api/Object.hpp"
#include "cpp_api/Builtin_Object.hpp"
#include "cpp_api/String.hpp"
#include "cpp_api/Array.hpp"
#include "cpp_api/Hash.hpp"
#include "cpp_api/Symbol.hpp"

#include "Address_Registration_Guard.hpp"
#include "cpp_api/Module.hpp"
#include "global_function.hpp"

#include "cpp_api/Class.hpp"
#include "cpp_api/Struct.hpp"

#include "Director.hpp"
#include "Data_Type.hpp"
#include "Constructor.hpp"
#include "Data_Object.hpp"

// Dependent on Data_Object due to the way method metadata is stored in the Ruby class
#include "detail/default_allocation_func.ipp"

#include "Enum.hpp"

// Dependent on Module, Class, Array and String
#include "forward_declares.ipp"

#endif // Rice__hpp_
