#ifndef SERIALIZER_H
#define SERIALIZER_H

struct Serializer
{
  enum class SourceType
  {
    STRING,
    FILE,
  };

  template <typename T>
  static T read(
    const String& str,
    Error& out_error,
    Allocator* allocator = nullptr,
    SourceType source_type = SourceType::STRING
  );
  // TODO: what arguments does this take?
  template <typename T>
  static void write();

private:
  static String
  take_source(const String& str, SourceType source_type, Allocator& allocator, Error& out_error);
};

#endif
