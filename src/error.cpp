#include "error.h"

static void
setup_error_to_error_string()
{
  error_to_error_string[(usize) Error::Success] = "success";
  error_to_error_string[(usize) Error::OutOfMemory] = "out of memory";
  error_to_error_string[(usize) Error::InvalidParameter] = "invalid parameter";
  error_to_error_string[(usize) Error::FileReading] = "file reading";
  error_to_error_string[(usize) Error::NotFound] = "not found";

  error_to_error_string[(usize) Error::ShaderCompilation] = "[SHADER] compilation";
  error_to_error_string[(usize) Error::ShaderLinking] = "[SHADER] linking";

  error_to_error_string[(usize) Error::PngInvalidHeader] = "[PNG] invalid header";
  error_to_error_string[(usize) Error::PngIhdrNotFirst] = "[PNG] ihdr chunk is not first";
  error_to_error_string[(usize) Error::PngInvalidIhdr] = "[PNG] invalid ihdr chunk";
  error_to_error_string[(usize) Error::PngInvalidIdat] = "[PNG] invalid idat chunk";
  error_to_error_string[(usize) Error::PngBadSizes] = "[PNG] bad sizes";
  error_to_error_string[(usize) Error::PngBadCodeLengths] = "[PNG] bad code lengths";
  error_to_error_string[(usize) Error::PngBadHuffmanCode] = "[PNG] bad huffman code";
  error_to_error_string[(usize) Error::PngBadDistance] = "[PNG] bad distance";
  error_to_error_string[(usize) Error::PngUnexpectedEnd] = "[PNG] unexpected end";
  error_to_error_string[(usize) Error::PngCorruptZlib] = "[PNG], corrupt zlib";
  error_to_error_string[(usize) Error::PngReadPastBuffer] = "[PNG] read past buffer";
  error_to_error_string[(usize) Error::PngInvalidFilter] = "[PNG] invalid filter";
  error_to_error_string[(usize) Error::PngIllegalCompresionType] = "[PNG] illegal compression type";

  error_to_error_string[(usize) Error::ObjInvalidData] = "[OBJ] invalid data";
}

