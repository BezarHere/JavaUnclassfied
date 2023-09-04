#include "pch.h"
#include "main.h"
#include "util.h"


inline TypeDescriptor read_type_desc(StreamReader &reader)
{
	TypeDescriptor type{ reader.read<TypeJ>() };
	switch (type.type)
	{
	case TypeJ::Class:
		{
			intptr_t end{};

			while (reader && ++end && reader.read<char>() != ';');

			if (end < 2)
			{
				type.type = TypeJ::None;
				break;
			}

			reader.move(-end);
			end--;
			type.class_name = std::string((char *)reader.read(end).release(), (size_t)(end));
			reader.move(1);
		}
		break;
	case TypeJ::Array:
		{
		type.next.reset( new TypeDescriptor(read_type_desc(reader)) );
		}
		break;
	default:
		break;
	}

	return type;
}

inline Attribute read_attr(StreamReader &reader)
{
	const_id name_id = reader.read<const_id>();
	uint32_t len = reader.read<uint32_t>();
	return Attribute{ name_id, span<byte_t>(reader.read(len).get(), len) };
}

inline span<Attribute> read_attrs_span(uint16_t count, StreamReader &reader)
{
	span<Attribute> sp{ count };
	for (int i{}; i < count; i++)
		sp[ i ] = read_attr(reader);
	return sp;
}

inline Constant read_const(StreamReader &reader)
{
	const ConstTag tag = reader.read<ConstTag>();
	const size_t len = tag == ConstTag::String ? get_utf8_data_len(reader) : get_const_data_len(tag);
	const bool doubled_slot = is_double_sloted(tag);
	span<byte_t> buf{ reader.read(len).release(), len };


	return Constant{ tag, std::move(buf) };
}

inline FieldInfo read_field(StreamReader &reader)
{
	return FieldInfo{ reader.read<AccessFlags>(), reader.read<const_id>(), reader.read<const_id>(), read_attrs_span(reader.read<uint16_t>(), reader) };
}

inline MethodInfo read_method(StreamReader &reader)
{
	return MethodInfo{ reader.read<AccessFlags>(), reader.read<const_id>(), reader.read<const_id>(), read_attrs_span(reader.read<uint16_t>(), reader) };
}

inline Error load_file(StreamReader &reader, ClassFile &f)
{
	if (!reader)
		return Error::InvalidReader;

	if (reader.read<DWORD>() != MAGIC)
		return Error::NoMagicNumber;
		
	f.minor_version = reader.read<uint16_t>();
	f.major_version = reader.read<uint16_t>();
	const uint16_t cpool_len = reader.read<uint16_t>() + 1;
	Constant *consts = new Constant[ cpool_len ];

	for (size_t i{}; i < cpool_len; i++)
	{
		consts[ i ] = read_const(reader);
		if (is_double_sloted(consts[ i ].tag))
		{
			consts[ ++i ].tag = ConstTag::Invalid;
		}
	}

	f.consts = span<Constant>(consts, cpool_len);


	f.access_flags = reader.read<AccessFlags>();
	f.class_id = reader.read<const_id>();
	f.superclass_id = reader.read<const_id>();
	const uint16_t interfaces_count = reader.read<uint16_t>();
	f.interfaces = span<const_id>((const_id *)reader.read(interfaces_count).get(), interfaces_count);
	
	const uint16_t fields_size = reader.read<uint16_t>();
	f.fields = span<FieldInfo>(fields_size);

	for (int i{}; i < f.fields.size(); i++)
		f.fields[ i ] = read_field(reader);
	
	const uint16_t methods_size = reader.read<uint16_t>();
	f.methods = span<MethodInfo>(methods_size);

	for (int i{}; i < f.methods.size(); i++)
		f.methods[ i ] = read_method(reader);

	return Error::Ok;
}

inline Error load_file(string path, ClassFile &f)
{
	StreamReader reader{ path, bite::EndianOrder::Big };
	return load_file(reader, f);
}

inline BufferSmartPtr_t class_decompiled(const ClassFile &file)
{
	return BufferSmartPtr_t();
}

template <typename _T>
inline void class_repr(const ClassFile &file, _T &out)
{

	out << "Version: " << file.major_version << '.' << file.minor_version << '\n';
	out << "Consts: " << file.consts.size() << '\n';
	
	for (int i{}; i < file.consts.size(); i++)
	{
		out << " [ " << i << " ] " << get_const_type_name(file.consts[ i ].tag) << ": " << get_const_value_str(i, file) << '\n';
	}

	out << "Interfaces: " << file.interfaces.size() << '\n';


	for (const_id i : file.interfaces)
	{
		out << "Interface{ " << i << " }: " << get_const_value_str(i, file) << '\n';
	}

	out << "Fields " << file.fields.size() << ':' << '\n';

	for (const FieldInfo &i : file.fields)
	{
		for (const Attribute &j : i.attributes)
		{
			out << "\t@" << j.name_id << get_qualified_name(get_const_value_str(j.name_id, file), file._qualified_names_table) << '\n';
		}

		out
			<< '\t'
			<< get_access_flags(i.access_flags, AccessFlagsType::Field)
			<< get_qualified_name(get_const_value_str(i.descriptor_index, file), file._qualified_names_table) << ' '
			<< get_qualified_name(get_const_value_str(i.name_index, file), file._qualified_names_table)
			<< ";\n\n";
		
	}

	out << "Methods " << file.methods.size() << ':' << '\n';

	for (const MethodInfo &i : file.methods)
	{
		for (const Attribute &j : i.attributes)
		{
			out << "\t@" << get_qualified_name(get_const_value_str(j.name_id, file), file._qualified_names_table) << '\n';
		}

		out
			<< get_access_flags(i.access_flags, AccessFlagsType::Method)
			<< get_qualified_name(get_const_value_str(i.descriptor_index, file), file._qualified_names_table) << ' '
			<< get_qualified_name(get_const_value_str(i.name_index, file), file._qualified_names_table)
			<< "();\n\n";
	}

	//ss.str(out);
}

int main()
{
	const string path = "F:\\Assets\\visual studio\\JavaUnclassfied\\JavaUnclassfied\\ald.class";
	ClassFile f{};
	std::cout << (int)load_file(path, f) << '\n';
	std::ofstream fs{ "F:\\Assets\\visual studio\\JavaUnclassfied\\JavaUnclassfied\\test.txt" };
	class_repr(f, fs);
	class_repr(f, std::cout);
}

