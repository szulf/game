#include "base/base.cpp"

typedef bool (*TestFN)();

struct Test
{
  const char* name;
  TestFN fn;
};

class TestRunner
{
public:
  void add(const Test& test)
  {
    tests[count++] = test;
  }

  void run()
  {
    usize success_count = 0;
    for (usize test_idx = 0; test_idx < count; ++test_idx)
    {
      auto& test = tests[test_idx];
      print("Running test: '%s'\n", test.name);
      bool success = test.fn();
      if (success)
      {
        ++success_count;
      }
      print("Outcome     : %s\033[0m\n", success ? "\033[32mSucceeded" : "\033[31mFailed");
    }
    print("\n%zu/%zu tests passed.\n", success_count, count);
  }

private:
  static constexpr usize MAX_TESTS = 100;
  Test tests[MAX_TESTS];
  usize count = 0;
};

bool test_array_sort()
{
  i32 test_items[] = {-1, 5, 2, 12, 49, 123, -129, -14, 123, -89, 89, 1, 32};
  i32 sorted_items[] = {-129, -89, -14, -1, 1, 2, 5, 12, 32, 49, 89, 123, 123};

  auto arr = Array<i32>::from(test_items, array_size(test_items));
  arr.sort(
    [](const i32& a, const i32& b)
    {
      return a > b;
    }
  );
  for (usize i = 0; i < arr.size; ++i)
  {
    if (arr[i] != sorted_items[i])
    {
      return false;
    }
  }
  return true;
}

bool test_map_set_get()
{
  constexpr i32 N = 5;
  String keys[N] = {
    String::make("key 1"),
    String::make("key 2"),
    String::make("key 3"),
    String::make("key 4"),
    String::make("key 5")
  };
  i32 values[N] = {-1, 69, -420, 42, 123};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto map = Map<String, i32>::make(100, scratch_arena.allocator);
  for (usize i = 0; i < N; ++i)
  {
    map.set(keys[i], values[i]);
  }

  for (usize i = 0; i < N; ++i)
  {
    auto* v = map[keys[i]];
    if (v == nullptr || *v != values[i])
    {
      return false;
    }
  }
  return true;
}

bool test_map_entry()
{
  constexpr i32 N = 5;
  String keys[N] = {
    String::make("key 1"),
    String::make("key 2"),
    String::make("key 3"),
    String::make("key 4"),
    String::make("key 5")
  };
  i32 values[N] = {-1, 69, -420, 42, 123};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto map = Map<String, i32>::make(100, scratch_arena.allocator);
  for (usize i = 0; i < N; ++i)
  {
    map.set(keys[i], values[i]);
  }

  for (usize i = 0; i < N; ++i)
  {
    auto* e = map.entry(keys[i]);
    if (e == nullptr || e->key != keys[i] || e->value != values[i] || !e->set)
    {
      return false;
    }
  }
  return true;
}

bool test_map_contains_succ()
{
  constexpr i32 N = 5;
  String keys[N] = {
    String::make("key 1"),
    String::make("key 2"),
    String::make("key 3"),
    String::make("key 4"),
    String::make("key 5")
  };
  i32 values[N] = {-1, 69, -420, 42, 123};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto map = Map<String, i32>::make(100, scratch_arena.allocator);
  for (usize i = 0; i < N; ++i)
  {
    map.set(keys[i], values[i]);
  }

  return map.contains(keys[3]);
}

bool test_map_contains_fail()
{
  constexpr i32 N = 5;
  String keys[N] = {
    String::make("key 1"),
    String::make("key 2"),
    String::make("key 3"),
    String::make("key 4"),
    String::make("key 5")
  };
  i32 values[N] = {-1, 69, -420, 42, 123};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto map = Map<String, i32>::make(100, scratch_arena.allocator);
  for (usize i = 0; i < N; ++i)
  {
    map.set(keys[i], values[i]);
  }

  return !map.contains(String::make("key 6"));
}

bool test_string_count_char()
{
  auto str = String::make("hello world, this is a pretty long string, it is also a test string");
  auto n = str.count('h');
  return n == 2;
}

bool test_string_count_cstr()
{
  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  auto n = str.count("test");
  return n == 3;
}

bool test_string_find_char_succ()
{
  auto str =
    String::make("test string, this is made for [x] a test case, that is run with the test batch");
  auto n = str.find('x');
  return n == 31;
}

bool test_string_find_char_fail()
{
  auto str =
    String::make("test string, this is made for [] a test case, that is run with the test batch");
  auto n = str.find('x');
  return n == (usize) -1;
}

bool test_string_find_char_start_idx()
{
  auto str =
    String::make("test string, this is made for [x] a test case, that is run with the test batch");
  auto n = str.find('x', 45);
  return n == (usize) -1;
}

bool test_string_find_last_char()
{
  auto str = String::make("some/file/path/");
  auto n = str.find_last('/');
  return n == 14;
}

bool test_string_starts_with_succ()
{
  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  return str.starts_with("test string,");
}

bool test_string_starts_with_fail()
{
  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  return !str.starts_with("test string!");
}

bool test_string_ends_with_succ()
{
  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  return str.ends_with("test batch");
}

bool test_string_ends_with_fail()
{
  auto str = String::make("test string, this is made for a test case, that is run with the test");
  return !str.ends_with("test batch");
}

bool test_string_append_cstr()
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  auto new_str = str.append(", another test", scratch_arena.allocator);
  return new_str ==
         "test string, this is made for a test case, that is run with the test batch, another test";
}

bool test_string_append_str()
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  auto new_str = str.append(String::make(", another test"), scratch_arena.allocator);
  return new_str ==
         "test string, this is made for a test case, that is run with the test batch, another test";
}

bool test_string_prepend_cstr()
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  auto new_str = str.prepend("another test, ", scratch_arena.allocator);
  return new_str ==
         "another test, test string, this is made for a test case, that is run with the test batch";
}

bool test_string_substr()
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto str =
    String::make("test string, this is made for a test case, that is run with the test batch");
  auto new_str = str.substr(str.find(','), 5, scratch_arena.allocator);
  return new_str == ", thi";
}

bool test_string_split()
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  const char* strs[] =
    {"test string", " this is made for a test case", " ", " that is run with the test batch"};
  auto str =
    String::make("test string, this is made for a test case,, , that is run with the test batch");
  auto splits = str.split(',', scratch_arena.allocator);
  if (splits.size != 4)
  {
    return false;
  }
  for (usize i = 0; i < splits.size; ++i)
  {
    if (splits[i] != strs[i])
    {
      return false;
    }
  }
  return true;
}

bool test_string_trim_whitespace()
{
  auto str = String::make("   \n\tthis is a test string  \t\r");
  auto new_str = str.trim_whitespace();
  return new_str == "this is a test string";
}

bool test_string_get_filename()
{
  auto str = String::make("some/file/path/file.txt");
  auto name = str.get_filename();
  return name == "file";
}

i32 main()
{
  TestRunner runner{};

  runner.add({"Array::sort()", test_array_sort});

  runner.add({"Map::set() Map::operator[]()", test_map_set_get});
  runner.add({"Map::entry()", test_map_entry});
  runner.add({"Map::contains() success", test_map_contains_succ});
  runner.add({"Map::contains() fail", test_map_contains_fail});

  runner.add({"String::count(char c)", test_string_count_char});
  runner.add({"String::count(const char* c)", test_string_count_cstr});
  runner.add({"String::find(char c) success", test_string_find_char_succ});
  runner.add({"String::find(char c) fail", test_string_find_char_fail});
  runner.add({"String::find(char c, usize start_idx)", test_string_find_char_start_idx});
  runner.add({"String::find_last(char c)", test_string_find_last_char});
  runner.add({"String::starts_with(const char* cstr) success", test_string_starts_with_succ});
  runner.add({"String::starts_with(const char* cstr) fail", test_string_starts_with_fail});
  runner.add({"String::ends_with(const char* cstr) success", test_string_ends_with_succ});
  runner.add({"String::ends_with(const char* cstr) fail", test_string_ends_with_fail});
  runner.add({"String::append(const char* cstr)", test_string_append_cstr});
  runner.add({"String::append(const String& str)", test_string_append_str});
  runner.add({"String::prepend(const char* cstr)", test_string_prepend_cstr});
  runner.add({"String::substr()", test_string_substr});
  runner.add({"String::split()", test_string_split});
  runner.add({"String::trim_whitespace()", test_string_trim_whitespace});
  runner.add({"String::get_filename()", test_string_get_filename});

  runner.run();

  return 0;
}
