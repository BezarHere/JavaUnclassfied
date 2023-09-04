#pragma once
#include "pch.h"


using bite::span;
using bite::StreamReader;
using bite::byte_t;
using bite::BufferSmartPtr_t;
using std::string;
typedef std::unordered_map<std::string, std::string> QualNameTable_t;

typedef uint16_t const_id;
typedef uint16_t code_ptr;

constexpr DWORD MAGIC = 0xCAFEBABE;

enum class Error
{
	Ok,
	InvalidReader,
	NoMagicNumber,
	NoSpaceForConstants,
	NoSpaceForInterfaces,
	NoSpaceForFields,
	NoSpaceForMethods,
	NoSpaceForAttributes,
};

enum class AccessFlagsType : byte_t
{
	Field,
	Method,
	Class
};

enum AccessFlags : uint16_t
{
	Public = 0x0001,
	Private = 0x0002,
	Protected = 0x0004,
	Static = 0x0008,
	Final = 0x0010,
	Synchronized = 0x0020,
	Super = 0x0020,
	Bridge = 0x0040,
	Volatile = 0x0040,
	VarArgs = 0x0080,
	Transient = 0x0080,
	Native = 0x0100,
	Interface = 0x0200,
	Abstract = 0x0400,
	Strict = 0x0800,
	Synthetic = 0x1000,
	Annotation = 0x2000,
	Enum = 0x4000,
};

enum class ConstTag : byte_t
{
	Invalid = 0,
	String, // utf-8

	Int = 3,
	Float,
	Long,
	Double,
	Class,
	StringRef,
	FieldRef,
	MethodRef,
	InterfaceMethodRef,
	NameTypeDesc,


	MethodHandle = 15,
	MethodType,
	Dynamic,
	InvokeDynamic,
	ModuleId,
	PackageId,
};

enum class TypeJ : char
{
	None = 0,
	Byte = 'B',
	Char = 'C',
	Double = 'D',
	Float = 'F',
	Int = 'I',
	Long = 'J', // J Not L
	Class = 'L', // L
	Short = 'S',
	Bool = 'Z', // Z
	Array = '['
};

struct TypeDescriptor
{
	TypeJ type;
	std::string class_name;
	std::unique_ptr<TypeDescriptor> next;
};


struct Exception
{
	code_ptr start, end;
	code_ptr handler;
	const_id catch_type;
};

struct Constant
{
	ConstTag tag{};
	span<byte_t> data{};
};

struct Attribute
{
	const_id name_id;
	span<byte_t> info;
};

struct JElementInfo
{
	AccessFlags access_flags;
	const_id name_index;
	const_id descriptor_index;
	span<Attribute> attributes;
};

typedef JElementInfo FieldInfo;
typedef JElementInfo MethodInfo;

struct ClassFile
{
	uint16_t minor_version{};
	uint16_t major_version{};
	span<Constant> consts;
	AccessFlags access_flags{};
	const_id class_id{};
	const_id superclass_id{};
	span<const_id> interfaces;
	span<FieldInfo> fields;
	span<MethodInfo> methods;
	span<Attribute> attributes;
	
	mutable QualNameTable_t _qualified_names_table{ 1024 }; // used to check for name collisions
};

