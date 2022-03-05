Style Guide:
	Naming Convention:
		Functions and variables:
		   Functions and Variables are named in uppercase CamelCase 
		Macros:
			Macros are named in full uppercase SNAKE_CASE
		Types (structs, union, typedefs):
			Types are named in lowercase snake_case
	Shared Naming Convention:
		Functions and variables shared between layers:
			Functions and variables shared between layers should be prefixed 
			with their layer prefix while mantaining uppercase CamelCase. 
			Ex: Prefix + FuncName -> PrefixFuncName 
		Macros:
			Macros shared between layers should be prefixed with their layer 
			prefix while mantaining full uppercase SNAKE_CASE.
			Ex: PREFIX + TYPE_NAME -> PREFIX_TYPE_NAME 
		Types shared between layers:
			Types shared between layers should be prefixed with their layer 
			prefix while mantaining lowercase snake_case.
			Ex: prefix + type_name -> prefix_type_name 
	Indentation: 
		Code uses four spaces for indentation; tabs are not allowed! 
	Line Limit: 
		The limit of length of line is 80-chars. 
		Long argument list are broken up as if a series of statements in
		a block.

		Ex 1. of breaking line: 
			Func(A, B, C, D) becomes
			Function(
				A,
				B,
				C,
				D
			)

		Ex 2. of breaking line: 
			int Func(int A, int B, int C, int D) becomes 
			int Func(
				int A, 
				int B,
				int C,
				int D
			)
	Line Endings:
		All lines end with UNIX line endings in online repository.
	Attributes:
		Both GCC (__atttribute__) and C2X attributes ([[]]) are to be put on
		the line before the return type. If both are used C2X attributes come
		second. 
	Documentation:
		When written documentation should be written in Linux kernel style 
	C Standard:
		All C2X compatible features are allowed 
Conventions:
	On passing an array as a parameter 
		Static array indices should be used to indicate the size of the array passed 
		Example:
			int Func(int Array[static 4]);
		Previous parameter should be element count if not element count is dynamic 
		Example:
			int Func(size_t Count, int Array[static Count]);
Build:
	Currently, the game build is two unities build, one for the platform layer and
	the other for the platform independent code.
