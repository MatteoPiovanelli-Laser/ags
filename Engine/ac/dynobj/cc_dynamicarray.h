//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#ifndef __CC_DYNAMICARRAY_H
#define __CC_DYNAMICARRAY_H

#include <vector>
#include <unordered_map>
#include "ac/dynobj/cc_agsdynamicobject.h"


#define ARRAY_MANAGED_TYPE_FLAG    0x80000000

struct CCDynamicArray final : AGSCCDynamicObject
{
public:
    static const char *TypeName;

    struct Header
    {
        // Type id of elements, refering the RTTI
        // May contain ARRAY_MANAGED_TYPE_FLAG
        uint32_t TypeID = 0u;
        uint32_t ElemCount = 0u;
        // TODO: refactor and store "elem size" instead
        uint32_t TotalSize = 0u;
    };

    inline static const Header &GetHeader(const char *address)
    {
        return reinterpret_cast<const Header&>(*(address - MemHeaderSz));
    }

    // return the type name of the object
    const char *GetType() override;
    int Dispose(const char *address, bool force) override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz);
    // Create managed array object and return a pointer to the beginning of a buffer
    DynObjectRef CreateOld(uint32_t elem_count, uint32_t elem_size, bool isManagedType)
        { return CreateImpl(0u, isManagedType, elem_count, elem_size); }
    DynObjectRef CreateNew(uint32_t type_id, uint32_t elem_count, uint32_t elem_size)
        { return CreateImpl(type_id, false, elem_count, elem_size); }

    // Remap typeid fields using the provided map
    void RemapTypeids(const char* address,
        const std::unordered_map<uint32_t, uint32_t> &typeid_map) override;
    // Traverse all managed references in this object, and run callback for each of them
    void TraverseRefs(const char *address, PfnTraverseRefOp traverse_op) override;

private:
    // The size of the array's header in memory, prepended to the element data
    static const size_t MemHeaderSz = sizeof(Header);
    // The size of the serialized header
    static const size_t FileHeaderSz = sizeof(uint32_t) * 3;

    DynObjectRef CreateImpl(uint32_t type_id, bool is_managed, uint32_t elem_count, uint32_t elem_size);

    // Savegame serialization
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const char *address) override;
    // Write object data into the provided stream
    void Serialize(const char *address, AGS::Common::Stream *out) override;
};

extern CCDynamicArray globalDynamicArray;

// Helper functions for setting up dynamic arrays.
namespace DynamicArrayHelpers
{
    // Create array of managed strings
    DynObjectRef CreateStringArray(const std::vector<const char*>);
};

#endif