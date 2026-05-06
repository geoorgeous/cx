# cx code conventions

 - [naming identifiers](#naming-identifiers)
 - [booleans](#booleans)
 - [pointers](#pointers)
 - [typedef usage](#typedef)
 - [enumeration constants](#enumeration-constants)
 - [function output](#function-output)
 - [read-only members](#read-only-members)
 - [line length](#line-length)

## naming identifiers

| type                    | convention       | example                     |
| ----------------------- | ---------------- | --------------------------- |
| file, directory names   | snake-case       | src/module_foo_bar.c        |
| include guard           | file name, uppser-snake-case | `#ifndef _H__FOO_BAR` (*src/foo_bar.h*) |
| preprocessor definition | upper-snake-case | `#define FOO_BAR 42`        |
| preprocessor macro      | upper-snake-case | `#define FOO_BAR() ...`     |
| function                | snake-case       | `void foo_bar_func(void);`  |
| struct                  | snake-case       | `struct foo_bar { ... };`   |
| enum                    | snake-case       | `enum foo_bar { ... };`     |
| variable                | snake-case       | `int foo_bar;`              |
 
## booleans

There is no reliance on `<stdbool.h>` or any kind of `typedef` or `#define` for boolean types.

 - Signed integers
 - Identifier prefixed with `b_`

```C
int b_foo;
```

## pointers

- Identifier prefixed with `p_`
- Double pointers etc. prefixed with one `p` per level of indirection
- Pointers to null-character (`\0`) terminated strings of characters prefixed with `s_`
- Function pointers prefixed with `f_`

```C
int*         p_foo;       // pointer
int**        pp_foo_bar;  // pointer to a pointer
const char*  s_name;      // null-terminated string
const char** p_s_name;    // pointer to a null-terminated string
int*         p_b_bool;    // pointer to a bool

int(*f_callback)(const int, int, int*); // pointer to a function. param names are optional
```

## typedef usage

Generally `typedef` should be avoided in favour of explicitly specifying types by their full identifiers.
One exception to this rule is with function types. Since some function types can be quite long, it can be
convenient to `typedef` the function type to a shorter identifer. **These identifiers should be postfixed with `_fn`**:

```C
typedef void(*foo_bar_fn)(const int, int, float*);

foo_bar_fn f_foo_bar_callback;
```

## enumeration constants

 - Snake-case
 - Prefixed with enum identifier in upper-snake-case
 - Optional last constant postfixed with `_MAX_`

```
enum foo_bar {
    FOO_BAR_first_item,
    FOO_BAR_second,
    FOO_BAR_a_third_item,
    FOO_BAR_MAX_
};
```

## function output

Many times functions may have a parameter that is a pointer to some type that is to be overwritten with some result.
These parameters should be at the end of a function's parameter list and should be prefixed with `p_out_`:

```C
void add(const int a, const int b, int* p_out);
```
```C
// multiple p_out params
void get_size(const struct rect* p_rect, float* p_out_width, float* p_out_height);
```
```C
// boolean p_out param
enum error validate(int* p_out_b_success);
```

## read-only members

Some structs may have members that should only be modified by the implimentation, and not the user.
It is often useful to expose a struct's definition to make stack-allocation of said structs more convenient,
but this will then expose members that shouldn't be modified by the user.
In these cases, read-only members should be prefixed with a single underscore (`_`):

```C
struct foo_bar {
  int read_write_me;
  int _read_only;
};
```

## line length

Lines of code that exceed 120 characters should be split in to multiple lines. To assist with this, most editors have
a way of visualising document columns. For example in Neovim: `vim.o.colorcolumn = "120"
`

```C
// bad:
int ray_plane_intersection(const float* p_ray_origin, const float* p_ray_dir, const float* p_plane_norm, float plane_offset, float* p_result) {
  // ...
}

// good:
int ray_plane_intersection(
  const float* p_ray_origin, const float* p_ray_dir,
  const float* p_plane_norm,
  float plane_offset,
  float* p_out_intersection) {

  // recommended to leave a space at the top of the function implementation for clarity
  // ...
}
```

```C
// bad:
const char* s_text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";

// good:
const char* s_text =
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
  "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
  "Ut enim ad minim veniam, quis nostrud exercitation ullamco "
  "laboris nisi ut aliquip ex ea commodo consequat. ";
```
