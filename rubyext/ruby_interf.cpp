
#include <ruby.h>
#include "ruby/ruby.h"
#include "ruby/thread.h"

#include "../stridx.hpp"

extern "C" {

void str_idx_free(void *data) { delete (StringIndex *)data; }

// Wrap StringIndex class inside a ruby variable
static const rb_data_type_t str_idx_type = {
    // .wrap_struct_name: "doesn’t really matter what it is as long as it’s sensible and unique"
    .wrap_struct_name = "StringIndexW9q4We",

    // Used by Carbage Collector:
    .function =
        {
            .dmark = NULL,
            .dfree = str_idx_free,
            .dsize = NULL, // TODO
        },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

VALUE str_idx_alloc(VALUE self) {
  void *data = new StringIndex();
  return TypedData_Wrap_Struct(self, &str_idx_type, data);
}

VALUE StringIndexAddSegments(VALUE self, VALUE str, VALUE fileId) {
  std::string s1 = StringValueCStr(str);
  int fid = NUM2INT(fileId);

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  ((StringIndex *)data)->addStrToIndex(s1, fid, '/');

  return self;
}

VALUE StringIndexFind(VALUE self, VALUE str, VALUE minChars) {
  VALUE ret;
  std::string s1 = StringValueCStr(str);

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  StringIndex *idx = (StringIndex *)data;

  ret = rb_ary_new();
  const std::vector<std::pair<float, int>> &results = idx->findSimilar(s1, NUM2INT(minChars));
  int limit = 15;
  int i = 0;
  for (const auto &res : results) {
    VALUE arr = rb_ary_new();
    rb_ary_push(arr, INT2NUM(res.second));
    rb_ary_push(arr, DBL2NUM(res.first));
    rb_ary_push(ret, arr);
    i++;
    if (i >= limit) {
      break;
    }
  }
  return ret;
}

void Init_stridx(void) {

  VALUE cFoo = rb_define_class("CppStringIndex", rb_cObject);

  rb_define_alloc_func(cFoo, str_idx_alloc);
  rb_define_method(cFoo, "add", StringIndexAddSegments, 2);
  rb_define_method(cFoo, "find", StringIndexFind, 2);
}

} // End extern "C"

