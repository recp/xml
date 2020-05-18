# 🔋 XML parser for `C`

[![Build Status](https://travis-ci.org/recp/xml.svg?branch=master)](https://travis-ci.org/recp/xml)
[![Build status](https://ci.appveyor.com/api/projects/status/ugk4im145es00v3g/branch/master?svg=true)](https://ci.appveyor.com/project/recp/xml/branch/master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/c3439f84b19f40d2a5da3b748b3fe6b4)](https://www.codacy.com/manual/recp/xml?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=recp/xml&amp;utm_campaign=Badge_Grade)

This is very simple and very powerful XML parser. It creates DOM-like data structure and allows to iterate and process XML objects very simple way.
It does not alloc any memory for XML itself, it only allocs memory for tokens. It also does not use recursive way to build data structure which makes it very fast to build DOM-like tree structure.

#### Documentation

Almost all functions (inline versions) and parameters are documented inside related headers. <br />
Complete documentation: TODO.

## Features

- header-only or optional compiled library
- option to store members and arrays as reverse order or normal
- option to separate xml tag prefix 
- doesn't alloc memory for keys and values only for tokens
- creates DOM-like data structure to make it easy to iterate though
- simple api
- provides some util functions to print xml, get int32, int64, float, double...
- very small library
- unique way to parse XML (check the object map section)
- helper to get string nodes, primitive values (int, float, bool) for both attribs and values

## TODOs

- [x] provide header only library and optionally compile version
- [x] provide option to preserve array order (currently array order is reversed, because it is easy to parse it in this way; this may be changed. Please follow new commits or releases)
- [x] provide option to separate tag prefixes
- [x] windows build
- [ ] documentation
- [x] handle or ignore comments? (ignored)
- [x] handle or ignore CDATA? (ignored)
- [x] cmake
- [ ] tests
- [ ] extra optimizations
- [ ] usage in detail

## Build

### Unix (Autotools)

```bash
sh autogen.sh
./configure
make
[sudo] make install
```

you can grap library in .libs folder after build finished

### Windows (MSBuild)

Windows related build files, project files are located in `win` folder,
make sure you are inside `tm/win` folder.
Code Analysis are enabled, it may take awhile to build

```Powershell
cd win
.\build.bat
```

##### Cmake options with Defaults:

```CMake
option(XML_SHARED "Shared build" ON)
option(XML_STATIC "Static build" OFF)
option(XML_USE_C99 "" OFF) # C11 
option(XML_USE_TEST "Enable Tests" OFF) # for make check - make test
```

```bash
mkdir build && cd build
cmake ..
make
[sudo] make install
```

### Header-only or Compiled Library

The functions has the `xmlc_` prefix are compiled version which is called from library. To use this feature you must include `xml/call/xml.h` header.

To use header-only library you must include `xml/xml.h` header. The functions has the `xml_` prefix are forced to be inlined. When you use this, you don't have to compile the library.

todo.

#### Example usage

You can inspect `xml_print()` to view usage in more detail. The example will be updated later to give more detail.

```C
#include <xml/xml.h>
#include <xml/print.h>

int main(int argc, const char * argv[]) {
  xml_doc_t *doc;
  xml_t     *root;
  
  doc  = xml_parse(/* XML string */, true, true);
  root = doc->root;

  xml_print_human(stderr, root);

  xml_free(doc);

  return 0;
}

```

```C

const xml_doc_t *xmlDoc;
const xml_t     *xml;

xmlDoc = xml_parse(/* XML string */, true, true);
xml    = xmlDoc->root->value;

/* already defined in util.h */
XML_INLINE
bool
xml_tag_eq(const xml_t * __restrict obj, const char * __restrict str);

while (xml) {
    if (xml_key_eq(xml, "tag 1")) {
      int aNumber;

     aNumber = xml_int32(xml, 0);

     /* ... */
    } else if (xml_tag_eq(xml, "tag 2")) {
      const char *nonNullTerminatedString;
      const char *nullTerminatedString;

      /* just pointer */
      nonNullTerminatedString = xml_string(xml);

       /* null-terminated string (strdup), needs to be freed */
      nullTerminatedString    = xml_string_dup(xml);

     /* ... */
    } else if (xml_key_eq(xml, "tag 3")) {
      xml_t *aChild;
      
      aChild = xml->value;
      while (aChild) {
          /* handle child node */
          aChild = aChild->next;
      }
    }

    xml = xml->next;
}
```

#### Using Object Map

```C
void
callback_1(xml_t * __restrict xml, void * __restrict obj) {
  printf("entered callback_1\n");
}

xml = xml_parse(/* XML string */, true, true);

xml_objmap_t objmap[] = {
    {
      .key = "key1",
      .foundFunc = {
        .func = callback_1
      }
    },
    {
      .key = "key2",
      .foundFunc = {
        .func = callback_1
      }
    }
};

/* or you can use macro helpers which is more readable if you don't need more details: */
xml_objmap_t objmap[] = {
    XML_OBJMAP_FN("key 1", func1, param1),
    XML_OBJMAP_FN("key 2", func2, param2),
    XML_OBJMAP_FN("key 3", func3, param3),
    /* ... */
};

xml_objmap_call(xml, objmap, ARRAY_LEN(objmap), NULL);
```

In this way you don't have to compare keys in a loop, just map the keys with a function or with userdata. You don't have to use function in this way, you may use to map xml object to userdata which may be a GOTO LABEL (to use compound gotos) or something else. 

## License

MIT. check the LICENSE file
