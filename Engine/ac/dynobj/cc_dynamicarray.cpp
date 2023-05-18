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
#include "cc_dynamicarray.h"
#include <string.h>
#include "ac/dynobj/managedobjectpool.h"
#include "ac/dynobj/dynobj_manager.h"
#include "script/cc_instance.h"
#include "script/cc_script.h" // RTTI
#include "util/memorystream.h"

using namespace AGS::Common;

const char *CCDynamicArray::TypeName = "CCDynamicArr2";

// return the type name of the object
const char *CCDynamicArray::GetType()
{
    return TypeName;
}

int CCDynamicArray::Dispose(const char *address, bool force)
{
    // If it's an array of managed objects, release their ref counts;
    // except if this array is forcefully removed from the managed pool,
    // in which case just ignore these.
    if (!force)
    {
        TraverseRefs(address, [](int handle) { pool.SubRefNoCheck(handle); });
    }

    delete[] (address - MemHeaderSz);
    return 1;
}

size_t CCDynamicArray::CalcSerializeSize(const char *address)
{
    const Header &hdr = GetHeader(address);
    return hdr.TotalSize + FileHeaderSz;
}

void CCDynamicArray::Serialize(const char *address, AGS::Common::Stream *out)
{
    const Header &hdr = GetHeader(address);
    out->WriteInt32(hdr.TypeID);
    out->WriteInt32(hdr.ElemCount);
    out->WriteInt32(hdr.TotalSize / hdr.ElemCount); // elem size
    out->Write(address, hdr.TotalSize); // elements
}

void CCDynamicArray::Unserialize(int index, Stream *in, size_t data_sz)
{
    char *new_arr = new char[(data_sz - FileHeaderSz) + MemHeaderSz];
    Header &hdr = reinterpret_cast<Header&>(*new_arr);
    hdr.TypeID = in->ReadInt32();
    hdr.ElemCount = in->ReadInt32();
    hdr.TotalSize = hdr.ElemCount * in->ReadInt32(); // elem size
    in->Read(new_arr + MemHeaderSz, data_sz - FileHeaderSz);
    ccRegisterUnserializedObject(index, &new_arr[MemHeaderSz], this);
}

DynObjectRef CCDynamicArray::CreateImpl(uint32_t type_id, bool is_managed, uint32_t elem_count, uint32_t elem_size)
{
    char *new_arr = new char[elem_count * elem_size + MemHeaderSz];
    memset(new_arr, 0, elem_count * elem_size + MemHeaderSz);
    Header &hdr = reinterpret_cast<Header&>(*new_arr);
    if (type_id > 0)
    {
        assert(ccInstance::GetRTTI()->GetTypes().size() > type_id);
        is_managed = (ccInstance::GetRTTI()->GetTypes()[type_id].flags & RTTI::kType_Managed) != 0;
    }
    hdr.TypeID = type_id | (ARRAY_MANAGED_TYPE_FLAG * is_managed);
    hdr.ElemCount = elem_count;
    hdr.TotalSize = elem_count * elem_size;
    void *obj_ptr = &new_arr[MemHeaderSz];
    // TODO: investigate if it's possible to register real object ptr directly
    int32_t handle = ccRegisterManagedObject(obj_ptr, this);
    if (handle == 0)
    {
        delete[] new_arr;
        return DynObjectRef(0, nullptr);
    }
    return DynObjectRef(handle, obj_ptr);
}

void CCDynamicArray::RemapTypeids(const char *address,
    const std::unordered_map<uint32_t, uint32_t> &typeid_map)
{
    Header &hdr = (Header&)GetHeader(address);
    const auto it = typeid_map.find(hdr.TypeID);
    assert(hdr.TypeID == 0u || it != typeid_map.end());
    hdr.TypeID = (it != typeid_map.end()) ? it->second : 0u;
}

void CCDynamicArray::TraverseRefs(const char *address, PfnTraverseRefOp traverse_op)
{
    // TODO: may be a bit faster if we make a "runtime type"
    // struct, merging joint type info and auxiliary helper data,
    // and store a pointer in the arr data.
    // might also have a dummy "type" for "unknown type" arrays.
    const Header &hdr = GetHeader(address);
    const uint32_t type_id = hdr.TypeID & (~ARRAY_MANAGED_TYPE_FLAG);
    bool is_managed = (hdr.TypeID & ARRAY_MANAGED_TYPE_FLAG) != 0;
    const RTTI::Type *ti = nullptr;
    if (type_id > 0)
    {
        assert(ccInstance::GetRTTI()->GetTypes().size() > type_id);
        ti = &ccInstance::GetRTTI()->GetTypes()[type_id];
    }

    // Dynamic array of managed pointers: subref them directly
    if (is_managed)
    {
        const uint32_t *handles = reinterpret_cast<const uint32_t*>(address);
        for (uint32_t i = 0; i < hdr.ElemCount; ++i)
        {
            traverse_op(handles[i]);
        }
    }
    // Dynamic array of regular structs that *may* contain managed pointers
    else if (ti && (ti->flags & RTTI::kType_Struct))
    {
        const auto fref = ccInstance::GetRTTIHelper()->GetManagedOffsetsForType(type_id);
        if (fref.second > fref.first)
        { // there are managed pointers inside!
            const char *elem_ptr = address;
            // For each array element...
            const uint32_t el_size = hdr.TotalSize / hdr.ElemCount;
            for (uint32_t i = 0; i < hdr.ElemCount; ++i, elem_ptr += el_size)
            {
                // ..subref each managed pointer found inside
                for (auto it = fref.first; it < fref.second; ++it)
                {
                    traverse_op(*(int32_t*)(elem_ptr + *it));
                }
            }
        }
    }
}


CCDynamicArray globalDynamicArray;


DynObjectRef DynamicArrayHelpers::CreateStringArray(const std::vector<const char*> items)
{
    // FIXME: create using CreateNew, but need to pass String's type id somehow! (just lookup for "String" in rtti?)
    DynObjectRef arr = globalDynamicArray.CreateOld(items.size(), sizeof(int32_t), true);
    if (!arr.second)
        return arr;
    // Create script strings and put handles into array
    int32_t *slots = static_cast<int32_t*>(arr.second);
    for (auto s : items)
    {
        DynObjectRef str = stringClassImpl->CreateString(s);
        // We must add reference count, because the string is going to be saved
        // within another object (array), not returned to script directly
        ccAddObjectReference(str.first);
        *(slots++) = str.first;
    }
    return arr;
}


#include "script/script_api.h"
#include "script/script_runtime.h"

int32_t DynamicArray_Length(void *untyped_dynarray)
{
    const CCDynamicArray::Header &hdr = CCDynamicArray::GetHeader((const char*)untyped_dynarray);
    return hdr.ElemCount;
}

RuntimeScriptValue Sc_DynamicArray_Length(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(DynamicArray_Length, void);
}

void RegisterDynamicArrayAPI()
{
    ccAddExternalStaticFunction("__Builtin_DynamicArrayLength^1", Sc_DynamicArray_Length);
}

