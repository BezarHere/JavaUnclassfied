#pragma once
#include "main.h"
constexpr int JVMArrayOffset = -1;

inline constexpr bool is_double_sloted(const ConstTag tag)
{
	return tag == ConstTag::Double || tag == ConstTag::Long;
}

inline size_t get_utf8_data_len(StreamReader &reader)
{
	return (size_t)reader.read<WORD>();
}

inline constexpr size_t get_const_data_len(ConstTag tag)
{
	switch (tag)
	{
	case ConstTag::String:
		throw std::exception("string is't valid for this func!");
		break;
	case ConstTag::Int:
	case ConstTag::Float:
		return 4U;
		break;
	case ConstTag::Long:
	case ConstTag::Double:
		return 8U;
		break;
	case ConstTag::Class:
	case ConstTag::StringRef:
		return 2U;
		break;
	case ConstTag::FieldRef:
	case ConstTag::MethodRef:
	case ConstTag::InterfaceMethodRef:
		return 4U;
		break;
	case ConstTag::NameTypeDesc:
		return 4U;
		break;
	case ConstTag::MethodHandle:
		return 3U;
		break;
	case ConstTag::MethodType:
		return 2U;
		break;
	case ConstTag::Dynamic:
	case ConstTag::InvokeDynamic:
		return 4U;
		break;
	case ConstTag::ModuleId:
	case ConstTag::PackageId:
		return 2U;
		break;
	default:

		break;
	}
	return 0U;
}


inline std::string strip_jvm_name(const std::string &jvm_name, size_t step = 0)
{
	if (step > 0)
		return strip_jvm_name(jvm_name.substr(0, jvm_name.find_last_of('/'))) + '/' + jvm_name.substr(jvm_name.find_last_of('/'));
	return jvm_name.substr(jvm_name.find_last_of('/') + 1);
}

inline std::string &get_qualified_name(const std::string &jvm_name, QualNameTable_t &names)
{
	// already cached
	if (names.find(jvm_name) != names.end())
		return names.at(jvm_name);

	const size_t max_strip_step = bite::strcount(jvm_name, '/');
	size_t strip_step{};

	std::string name{ max_strip_step > 0 ? strip_jvm_name(jvm_name, strip_step) : jvm_name };

	// very slopy...
	// FIXME: not rechecking entire table
	if (max_strip_step)
	{
		for (const auto &kv : names)
		{
			if (kv.second == name)
			{
				name = strip_jvm_name(jvm_name, ++strip_step);

				if (strip_step >= max_strip_step)
					break;
			}
		}
	}

	names.insert_or_assign(jvm_name, name);
	return names.at(jvm_name);
}


template <typename _T>
inline _T reverse_endian(const byte_t *const ptr)
{
	_T a;
	bite::M_EndianOrder((char *)&a, (const char *)ptr, sizeof(_T), bite::EndianOrder::Big);
	return a;
}

inline std::string get_const_type_name(const ConstTag &t)
{
	switch (t)
	{
	case ConstTag::String:
		return std::string("UTF-8 string");
	case ConstTag::Int:
		return std::string("Int");
	case ConstTag::Long:
		return std::string("Long");
	case ConstTag::Float:
		return std::string("Float");
	case ConstTag::Double:
		return std::string("Double");
	case ConstTag::Class:
		return std::string("Class referencse");
	case ConstTag::StringRef:
		return std::string("String referencse");
	case ConstTag::MethodType:
		return std::string("Method type");
	case ConstTag::InterfaceMethodRef:
		return std::string("Interface method type");
	case ConstTag::NameTypeDesc:
		return std::string("Name and type descriptor");
	case ConstTag::MethodHandle:
		return std::string("Method handle");
	case ConstTag::MethodRef:
		return std::string("Method reference");
	case ConstTag::FieldRef:
		return std::string("Field reference");
	case ConstTag::InvokeDynamic:
		return std::string("Bootstrap dynamic invoke");
	default:
		break;
	}
	return std::to_string((int)t);
}

template <typename _T>
inline std::string to_string_reversed_buffer(const _T *val)
{
	_T v{};
	for (size_t i{}; i < sizeof(_T); i++)
	{
		((byte_t *)&v)[ i ] = ((byte_t *)val)[ sizeof(_T) - (i + 1) ];
	}
	return std::to_string(v);
}

inline std::string get_const_value_str(const int index, const ClassFile &file, int i = 64, bool beutiful = true)
{
	if (--i <= 0)
		return std::string("OVERFLOW");
	if (index < 0 || index >= file.consts.size())
		return std::string("OutOfBounds index[ " + index) + std::string(" ]");
	const Constant &c{ file.consts[ index ] };
	const byte_t *data = c.data.begin();
	const auto recall = [&](const int new_ind) { return get_const_value_str(new_ind, file, i, beutiful); };
	switch (c.tag)
	{
	case ConstTag::String:
		return std::string((char *)data, c.data.size());
	case ConstTag::Int:
		return to_string_reversed_buffer((int32_t *)data);
	case ConstTag::Long:
		return to_string_reversed_buffer((int64_t *)data);
	case ConstTag::Float:
		return to_string_reversed_buffer((float *)data);
	case ConstTag::Double:
		return to_string_reversed_buffer((double *)data);
	case ConstTag::Class:
	case ConstTag::StringRef:
	case ConstTag::MethodType:
	{
		const const_id desc_index = reverse_endian<const_id>(data) + JVMArrayOffset;
		return recall(desc_index);
	}
	case ConstTag::FieldRef:
	case ConstTag::MethodRef:
	case ConstTag::InterfaceMethodRef:
	{
		const const_id class_index = reverse_endian<const_id>(data) + JVMArrayOffset;
		const const_id nametype_index = reverse_endian<const_id>(data + 2) + JVMArrayOffset;
		return recall(class_index) + '(' + recall(nametype_index) + ')';
	}
	case ConstTag::NameTypeDesc:
		{
			const const_id di = reverse_endian<const_id>(data) + JVMArrayOffset;
			const const_id ci = reverse_endian<const_id>(data + 2) + JVMArrayOffset;
			//return get_const_type_name(pool[ di ].tag) + "(" + std::to_string(di) + ")" + " : " + get_const_type_name(pool[ci].tag) + "(" + std::to_string(ci) + ")";
			return get_qualified_name(recall(di), file._qualified_names_table) + ':' + get_qualified_name(recall(ci), file._qualified_names_table);
		}
	case ConstTag::MethodHandle:
		{
			const uint16_t ref_kind = (uint16_t)(*data);
			const const_id ref_index = reverse_endian<const_id>(data + 1) + JVMArrayOffset;
			return '(' + ref_kind + ")=>" + recall(ref_index);
		}
	case ConstTag::InvokeDynamic:
		{
			const const_id bootstrap_index = reverse_endian<const_id>(data) + JVMArrayOffset;
			const const_id nametype_index = reverse_endian<const_id>(data + 2) + JVMArrayOffset;
			return "BOOTSTRAP[ " + bootstrap_index + (" ]:" + recall(nametype_index));
		}
	default:
		break;
	}
	return std::string("UNKNOWN");
}

inline std::string get_access_flags(AccessFlags flags, AccessFlagsType type)
{
	std::stringstream ss{};

	if (flags & AccessFlags::Native)
		ss << "native" << ' ';

	if (flags & AccessFlags::Final)
		ss << "final" << ' ';

	if (flags & AccessFlags::Synchronized)
	{
		if (type == AccessFlagsType::Method)
			ss << "synchronized" << ' ';
		else
			ss << "super" << ' '; // Super: classes
	}

	if (flags & AccessFlags::VarArgs)
	{
		if (type == AccessFlagsType::Method)
			ss << "varargs" << ' ';
		else
			ss << "transient" << ' '; // Transient: fields
	}

	if (flags & AccessFlags::Static) // global
		ss << "static" << ' ';

	if (flags & AccessFlags::Interface)
		ss << "interface" << ' ';

	if (flags & AccessFlags::Strict) // methods
		ss << "strictfp" << ' ';

	if (flags & AccessFlags::Abstract) // methods
		ss << "abstract" << ' ';

	if (flags & AccessFlags::Synthetic) // methods
		ss << "phantom" << ' ';
	
	if (flags & AccessFlags::Public) // globals: public/private/protected
		ss << "public" << ' ';
	else if (flags & AccessFlags::Private)
		ss << "private" << ' ';
	else if (flags & AccessFlags::Protected)
		ss << "protected" << ' ';

	if (flags & AccessFlags::Bridge)
	{
		if (type == AccessFlagsType::Method)
			ss << "bridge" << ' ';
		else
			ss << "volatile" << ' '; // Volatile: fields
	}

	if (flags & AccessFlags::Annotation) // sublcasses
		ss << "annot" << ' ';

	if (flags & AccessFlags::Enum) // fields
		ss << "enum" << ' ';

	return ss.str();
}
