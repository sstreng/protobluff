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

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/common.h>

#include "bin/enum.hh"
#include "bin/field.hh"
#include "bin/message.hh"
#include "bin/strutil.hh"

/* ----------------------------------------------------------------------------
 * Interface
 * ------------------------------------------------------------------------- */

namespace protobluff {

  using ::std::map;
  using ::std::set;
  using ::std::sort;
  using ::std::string;
  using ::std::vector;

  using ::google::protobuf::Descriptor;
  using ::google::protobuf::FieldDescriptor;
  using ::google::protobuf::io::Printer;
  using ::google::protobuf::scoped_ptr;

  using ::google::protobuf::LowerString;
  using ::google::protobuf::SimpleItoa;
  using ::google::protobuf::StringReplace;

  /*!
   * Create a message generator.
   *
   * \param[in] descriptor descriptor
   */
  Message::
  Message(const Descriptor *descriptor) :
      descriptor_(descriptor),
      fields_(new scoped_ptr<Field>[descriptor_->field_count()]),
      nested_(new scoped_ptr<Message>[descriptor_->nested_type_count()]),
      enums_(new scoped_ptr<Enum>[descriptor_->enum_type_count()]) {

    /* Sort field generators by tag */
    vector<Field *> sorted;
    for (size_t f = 0; f < descriptor_->field_count(); f++)
      sorted.push_back(new Field(descriptor_->field(f)));
    sort(sorted.begin(), sorted.end(), FieldComparator);

    /* Initialize field generators */
    for (size_t f = 0; f < descriptor_->field_count(); f++)
      fields_[f].reset(sorted[f]);

    /* Initialize nested message generators */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      nested_[n].reset(new Message(descriptor_->nested_type(n)));

    /* Initialize enum generators */
    for (size_t e = 0; e < descriptor_->enum_type_count(); e++)
    enums_[e].reset(new Enum(descriptor_->enum_type(e)));

    /* Build set of unique extended message descriptors */
    set<const Descriptor *> unique;
    for (size_t e = 0; e < descriptor_->extension_count(); e++)
      unique.insert(descriptor_->extension(e)->containing_type());

    /* Initialize extension generators */
    for (set<const Descriptor *>::iterator it  = unique.begin();
                                           it != unique.end(); ++it) {
      Extension *extension = new Extension(*it, descriptor_);
      for (size_t e = 0; e < descriptor_->extension_count(); e++)
        if (descriptor_->extension(e)->containing_type() == *it)
          extension->AddField(descriptor_->extension(e));

      /* Add to list of extension generators */
      extensions_.push_back(extension);
    }

    /* Extract full name for signature */
    variables_["signature"] = descriptor_->full_name();

    /* Prepare descriptor symbol */
    variables_["descriptor.symbol"] = StringReplace(
      variables_["signature"], ".", "_", true);
    LowerString(&(variables_["descriptor.symbol"]));
  }

  /*!
   * Check whether a message or its nested messages have enums.
   *
   * \return Test result
   */
  bool Message::
  HasEnums() const {
    if (descriptor_->enum_type_count())
      return true;

    /* Check enums for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      if (nested_[n]->HasEnums())
        return true;

    /* No enums */
    return false;
  }

  /*!
   * Retrieve nested enum generators.
   *
   * \return Nested enum generators
   */
  const vector<const Enum *> Message::
  GetEnums() const {
    vector<const Enum *> enums;
    for (size_t e = 0; e < descriptor_->enum_type_count(); e++)
      enums.push_back(enums_[e].get());

    /* Retrieve extensions for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++) {
      vector<const Enum *> nested = nested_[n]->GetEnums();
      enums.insert(enums.end(), nested.begin(), nested.end());
    }
    return enums;
  }

  /*!
   * Check whether a message or its nested messages have extensions.
   *
   * \return Test result
   */
  bool Message::
  HasExtensions() const {
    if (descriptor_->extension_count())
      return true;

    /* Check extensions for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      if (nested_[n]->HasExtensions())
        return true;

    /* No extensions */
    return false;
  }

  /*!
   * Retrieve nested extension generators.
   *
   * \return Nested extension generators
   */
  const vector<const Extension *> Message::
  GetExtensions() const {
    vector<const Extension *> extensions;
    extensions.insert(extensions.end(),
      extensions_.begin(), extensions_.end());

    /* Retrieve extensions for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++) {
      vector<const Extension *> nested = nested_[n]->GetExtensions();
      extensions.insert(extensions.end(), nested.begin(), nested.end());
    }
    return extensions;
  }

  /*!
   * Check whether a message or its nested messages have default values.
   *
   * \return Test result
   */
  bool Message::
  HasDefaults() const {
    for (size_t f = 0; f < descriptor_->field_count(); f++)
      if (fields_[f]->HasDefault())
        return true;

    /* Check default values for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      if (nested_[n]->HasDefaults())
        return true;

    /* No default values */
    return false;
  }

  /*!
   * Generate declaration.
   *
   * \param[in,out] printer Printer
   */
  void Message::
  GenerateDeclaration(Printer *printer) const {
    assert(printer);

    /* Generate forward declaration */
    printer->Print(variables_,
      "/* `signature` : descriptor */\n"
      "extern pb_message_descriptor_t\n"
      "`descriptor.symbol`_descriptor;\n"
      "\n");

    /* Generate forward declarations for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      nested_[n]->GenerateDeclaration(printer);
  }

  /*!
   * Generate default values.
   *
   * \param[in,out] printer Printer
   */
  void Message::
  GenerateDefaults(Printer *printer) const {
    assert(printer);
    for (size_t f = 0; f < descriptor_->field_count(); f++)
      if (fields_[f]->HasDefault())
        fields_[f]->GenerateDefault(printer);

    /* Generate default values for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      nested_[n]->GenerateDefaults(printer);
  }

  /*!
   * Generate descriptor.
   *
   * \param[in,out] printer Printer
   */
  void Message::
  GenerateDescriptor(Printer *printer) const {
    assert(printer);

    /* Generate descriptor header */
    if (descriptor_->field_count()) {
      printer->Print(variables_,
        "/* `signature` : descriptor */\n"
        "pb_message_descriptor_t\n"
        "`descriptor.symbol`_descriptor = { {\n"
        "  (const pb_field_descriptor_t []){\n");

      /* Generate field descriptors */
      for (size_t i = 0; i < 2; i++)
        printer->Indent();
      for (size_t f = 0; f < descriptor_->field_count(); f++) {
        fields_[f]->GenerateDescriptor(printer);
        if (f < descriptor_->field_count() - 1)
          printer->Print(",");
        printer->Print("\n");
      }
      for (size_t i = 0; i < 2; i++)
        printer->Outdent();

      /* Generate descriptor footer */
      printer->Print(
        "\n"
        "  }, `fields` } };\n"
        "\n", "fields", SimpleItoa(descriptor_->field_count()));

    /* Print empty descriptor, if message contains no fields */
    } else {
      printer->Print(variables_,
        "/* `signature` : descriptor */\n"
        "pb_message_descriptor_t\n"
        "`descriptor.symbol`_descriptor = {};\n"
        "\n");
    }

    /* Generate descriptors for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      nested_[n]->GenerateDescriptor(printer);
  }

  /*!
   * Generate descriptor assertion.
   *
   * \param[in,out] printer Printer
   */
  void Message::
  GenerateDescriptorAssertion(Printer *printer) const {
    assert(printer);

    /* Generate descriptor assertion */
    printer->Print(variables_,
      "/* `signature` : descriptor assertion */\n"
      "#define `descriptor.symbol`_descriptor_assert(descriptor) \\\n"
      "  (pb_message_descriptor(descriptor) == \\\n"
      "    &`descriptor.symbol`_descriptor)\n"
      "\n");

    /* Generate descriptor assertions for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      nested_[n]->GenerateDescriptorAssertion(printer);
  }

  /*!
   * Generate definitions.
   *
   * \param[in,out] printer Printer
   */
  void Message::
  GenerateDefinitions(Printer *printer) const {
    assert(printer);

    /* Generate constructor */
    printer->Print(variables_,
      "/* `signature` : create */\n"
      "#define `descriptor.symbol`_create(binary) \\\n"
      "  (pb_message_create( \\\n"
      "    &`descriptor.symbol`_descriptor, (binary)))\n"
      "\n");

    /* Generate constructor for byte fields */
    printer->Print(variables_,
      "/* `signature` : create from field */\n"
      "#define `descriptor.symbol`_create_from_field(field) \\\n"
      "  (pb_message_create_from_field( \\\n"
      "    &`descriptor.symbol`_descriptor, (field)))\n"
      "\n");

    /* Generate destructor */
    printer->Print(variables_,
      "/* `signature` : destroy */\n"
      "#define `descriptor.symbol`_destroy(message) \\\n"
      "  (`descriptor.symbol`_descriptor_assert(message), \\\n"
      "    (pb_message_destroy(message)))\n"
      "\n");

    /* Generate definitions for fields */
    for (size_t f = 0; f < descriptor_->field_count(); f++)
      fields_[f]->GenerateDefinitions(printer);

    /* Generate definitions for nested messages */
    for (size_t n = 0; n < descriptor_->nested_type_count(); n++)
      nested_[n]->GenerateDefinitions(printer);
  }

  /*!
   * Generate nested definitions.
   *
   * The trace is used to keep track of the fields that are involved from the
   * uppermost level to the definition of the underlying message.
   *
   * \param[in,out] printer Printer
   * \param[in,out] trace   Trace
   */
  void Message::
  GenerateDefinitions(
      Printer *printer, vector<const FieldDescriptor *> &trace) const {
    assert(printer);
    for (size_t f = 0; f < descriptor_->field_count(); f++)
      fields_[f]->GenerateDefinitions(printer, trace);
  }
}