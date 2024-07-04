
#include <ruby.h>
#include "ruby/ruby.h"
#include "ruby/thread.h"

#include "../stridx.hpp"

extern "C" {

void str_idx_free(void *data) { delete (StrIdx::StringIndex *)data; }

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
  void *data = new StrIdx::StringIndex();
  return TypedData_Wrap_Struct(self, &str_idx_type, data);
}

VALUE StringIndexAddSegments(VALUE self, VALUE str, VALUE fileId) {
  std::string s1 = StringValueCStr(str);
  int fid = NUM2INT(fileId);

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  // ((StringIndex *)data)->addStrToIndex(s1, fid);
  ((StrIdx::StringIndex *)data)->addStrToIndexThreaded(s1, fid);

  return self;
}

VALUE StringIndexWaitUntilDone(VALUE self) {
  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  ((StrIdx::StringIndex *)data)->waitUntilDone();
  return self;
}
  

VALUE StringIndexFind(VALUE self, VALUE str) {
  VALUE ret;
  std::string s1 = StringValueCStr(str);

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  StrIdx::StringIndex *idx = (StrIdx::StringIndex *)data;

  ret = rb_ary_new();
  const std::vector<std::pair<float, int>> &results = idx->findSimilar(s1);
  int limit = 40;
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

VALUE StringIndexFindFilesAndDirs(VALUE self, VALUE str) {
  VALUE ret;
  std::string s1 = StringValueCStr(str);

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  StrIdx::StringIndex *idx = (StrIdx::StringIndex *)data;

  ret = rb_ary_new();
  const std::vector<std::pair<float, std::string>> &results = idx->findFilesAndDirectories(s1);
  int limit = 40;
  int i = 0;
  for (const auto &res : results) {
    VALUE arr = rb_ary_new();
    rb_ary_push(arr, rb_str_new_cstr(res.second.c_str()));
    rb_ary_push(arr, DBL2NUM(res.first));
    rb_ary_push(ret, arr);
    i++;
    if (i >= limit) {
      break;
    }
  }
  return ret;
}

VALUE StringIndexFindDirs(VALUE self, VALUE str) {
  VALUE ret;
  std::string s1 = StringValueCStr(str);

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  StrIdx::StringIndex *idx = (StrIdx::StringIndex *)data;

  ret = rb_ary_new();
  const std::vector<std::pair<float, std::string>> &results = idx->findFilesAndDirectories(s1,false,true);
  int limit = 40;
  int i = 0;
  for (const auto &res : results) {
    VALUE arr = rb_ary_new();
    rb_ary_push(arr, rb_str_new_cstr(res.second.c_str()));
    rb_ary_push(arr, DBL2NUM(res.first));
    rb_ary_push(ret, arr);
    i++;
    if (i >= limit) {
      break;
    }
  }
  return ret;
}





VALUE StringIndexSetDirSeparator(VALUE self, VALUE str) {
  char c = '/';
  if (TYPE(str) == T_STRING) {
    std::string s = StringValueCStr(str);
    if (s.size() >= 1) {
      c = s[0];
    }
  } else {
    c = '\0'; // No separator
    // if (TYPE(obj) == T_NIL)
  }

  void *data;
  TypedData_Get_Struct(self, int, &str_idx_type, data);
  StrIdx::StringIndex *idx = (StrIdx::StringIndex *)data;
  idx->setDirSeparator(c);

  return self;
}

void Init_stridx(void) {

  VALUE mStrIdx = rb_define_module("StrIdx");
  VALUE classStringIndex = rb_define_class_under(mStrIdx, "StringIndex", rb_cObject);

  rb_define_alloc_func(classStringIndex, str_idx_alloc);
  rb_define_method(classStringIndex, "add", StringIndexAddSegments, 2);
  rb_define_method(classStringIndex, "waitUntilDone", StringIndexWaitUntilDone, 0);
  rb_define_method(classStringIndex, "find", StringIndexFind, 1);
  rb_define_method(classStringIndex, "findFilesAndDirs", StringIndexFindFilesAndDirs, 1);
  rb_define_method(classStringIndex, "findDirs", StringIndexFindDirs, 1);
  
  rb_define_method(classStringIndex, "setDirSeparator", StringIndexSetDirSeparator, 1);
  
  
}

} // End extern "C"
