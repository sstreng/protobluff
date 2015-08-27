/*
 * Copyright (c) 2013-2015 Martin Donath <martin.donath@squidfunk.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef PB_PROTOBLUFF_FILE_HH
#define PB_PROTOBLUFF_FILE_HH

#include <map>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/common.h>

#include "bin/enum.hh"
#include "bin/extension.hh"
#include "bin/message.hh"

/* ----------------------------------------------------------------------------
 * Interface
 * ------------------------------------------------------------------------- */

namespace protobluff {

  using ::std::map;
  using ::std::string;
  using ::std::vector;

  using ::google::protobuf::FileDescriptor;
  using ::google::protobuf::io::Printer;
  using ::google::protobuf::scoped_array;
  using ::google::protobuf::scoped_ptr;

  class File {

  public:
    explicit
    File(
      const FileDescriptor
        *descriptor);                  /* File descriptor */

    bool
    HasEnums()
    const;

    bool
    HasExtensions()
    const;

    bool
    HasDefaults()
    const;

    void
    GenerateHeader(
      Printer *printer)                /* Printer */
    const;

    void
    GenerateSource(
      Printer *printer)                /* Printer */
    const;

  private:
    const FileDescriptor *descriptor_; /* File descriptor */
    scoped_array<
      scoped_ptr<Message>
    > messages_;                       /* Message generators */
    scoped_array<
      scoped_ptr<Enum>
    > enums_;                          /* Enum generators */
    vector<Extension *> extensions_;   /* Extension generators */
    map<string, string> variables_;    /* Variables */

    void
    PrintDisclaimer(
      Printer *printer)                /* Printer */
    const;

    void
    PrintBanner(
      Printer *printer,                /* Printer */
      const char title[])              /* Title */
    const;
  };
}

#endif /* PB_PROTOBLUFF_FILE_HH */