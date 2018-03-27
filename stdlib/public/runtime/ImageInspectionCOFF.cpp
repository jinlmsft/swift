//===--- ImageInspectionWin32.cpp - Win32 image inspection ----------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#if !defined(__ELF__) && !defined(__MACH__)

#include "ImageInspection.h"
#include "ImageInspectionCOFF.h"

#if defined(__CYGWIN__)
#include <dlfcn.h>
#endif

using namespace swift;

namespace {
static const swift::MetadataSections *registered = nullptr;

void record(const swift::MetadataSections *sections) {
  if (registered == nullptr) {
    registered = sections;
    sections->next = sections->prev = sections;
  } else {
    registered->prev->next = sections;
    sections->next = registered;
    sections->prev = registered->prev;
    registered->prev = sections;
  }
}
}

void swift::initializeProtocolLookup() {
  const swift::MetadataSections *sections = registered;
  while (true) {
    const swift::MetadataSections::Range &protocols =
      sections->swift4_protocols;
    if (protocols.length)
      addImageProtocolsBlockCallback(reinterpret_cast<void *>(protocols.start),
                                     protocols.length);

    if (sections->next == registered)
      break;
    sections = sections->next;
  }
}

void swift::initializeProtocolConformanceLookup() {
  const swift::MetadataSections *sections = registered;
  while (true) {
    const swift::MetadataSections::Range &conformances =
        sections->swift4_protocol_conformances;
    if (conformances.length)
      addImageProtocolConformanceBlockCallback(reinterpret_cast<void *>(conformances.start),
                                               conformances.length);

    if (sections->next == registered)
      break;
    sections = sections->next;
  }
}

void swift::initializeTypeMetadataRecordLookup() {
  const swift::MetadataSections *sections = registered;
  while (true) {
    const swift::MetadataSections::Range &type_metadata =
        sections->swift4_type_metadata;
    if (type_metadata.length)
      addImageTypeMetadataRecordBlockCallback(reinterpret_cast<void *>(type_metadata.start),
                                              type_metadata.length);

    if (sections->next == registered)
      break;
    sections = sections->next;
  }
}

void swift::initializeTypeFieldLookup() {
  const swift::MetadataSections *sections = registered;
  while (true) {
    const swift::MetadataSections::Range &fields = sections->swift4_fieldmd;
    if (fields.length)
      addImageTypeFieldDescriptorBlockCallback(
          reinterpret_cast<void *>(fields.start), fields.length);

    if (sections->next == registered)
      break;
    sections = sections->next;
  }
}

SWIFT_RUNTIME_EXPORT
void swift_addNewDSOImage(const void *addr) {
  const swift::MetadataSections *sections =
      static_cast<const swift::MetadataSections *>(addr);

  record(sections);

  const auto &protocols_section = sections->swift4_protocols;
  const void *protocols =
      reinterpret_cast<void *>(protocols_section.start);
  if (protocols_section.length)
    addImageProtocolsBlockCallback(protocols, protocols_section.length);

  const auto &protocol_conformances = sections->swift4_protocol_conformances;
  const void *conformances =
      reinterpret_cast<void *>(protocol_conformances.start);
  if (protocol_conformances.length)
    addImageProtocolConformanceBlockCallback(conformances,
                                             protocol_conformances.length);

  const auto &type_metadata = sections->swift4_type_metadata;
  const void *metadata = reinterpret_cast<void *>(type_metadata.start);
  if (type_metadata.length)
    addImageTypeMetadataRecordBlockCallback(metadata, type_metadata.length);
}

int swift::lookupSymbol(const void *address, SymbolInfo *info) {
#if defined(__CYGWIN__)
  Dl_info dlinfo;
  if (dladdr(address, &dlinfo) == 0) {
    return 0;
  }

  info->fileName = dlinfo.dli_fname;
  info->baseAddress = dlinfo.dli_fbase;
  info->symbolName = dli_info.dli_sname;
  info->symbolAddress = dli_saddr;
  return 1;
#else
  return 0;
#endif // __CYGWIN__
}

// This is only used for backward deployment hooks, which we currently only support for
// MachO. Add a stub here to make sure it still compiles.
void *lookupSection(const char *segment, const char *section, size_t *outSize) {
  return nullptr;
}

#endif // !defined(__ELF__) && !defined(__MACH__)
