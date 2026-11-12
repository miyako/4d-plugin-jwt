#include "4DPluginAPI.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"
#include "ARRAY_TEXT.h"
#include "C_BLOB.h"
#include "4DPlugin-JSON.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sys/timeb.h>
#include <CoreGraphics/CoreGraphics.h>

// Only these synthetic engine operations are permitted. No real 4D host is loaded.
static std::map<void *, size_t> strings, handles;
static int refs = 0, pictures = 0;
static int bioCalls = 0;
static bool failGet = false, failCommand = false, failHas = false, failString = false, missing = false;
static PA_Variable property;
static PA_Variable stored;
static PA_Unichar empty[] = {0};
static PA_Unichar fixture[] = {'A', 0x65e5, 0xd83d, 0xde00, 0};
static PA_Unistring makeString(const PA_Unichar *s)
{
    size_t n = 0;
    if(s) while(s[n]) ++n;
    auto p = new PA_Unichar[n + 1];
    if(n) memcpy(p, s, n * sizeof(*p));
    p[n] = 0;
    strings[p] = n;
    PA_Unistring result = {};
    result.fString = p;
    result.fLength = static_cast<PA_long32>(n);
    return result;
}
static void dispose(PA_Unistring s)
{
    if(!s.fString) return;
    assert(strings.erase(s.fString) == 1);
    delete[] s.fString;
}
static PA_Variable copyVariable(PA_Variable v)
{
    switch(v.fType)
    {
        case 0:
        case eVK_Unistring: v.uValue.fString = makeString(v.uValue.fString.fString); break;
        case eVK_Object:
        case eVK_Collection: if(v.uValue.fObject) ++refs; break;
        case eVK_Picture: if(v.uValue.fPicture) ++pictures; break;
        default: break;
    }
    return v;
}
static void clear(PA_Variable *v)
{
    switch(v->fType)
    {
        case 0:
        case eVK_Unistring: dispose(v->uValue.fString); break;
        case eVK_Object:
        case eVK_Collection: if(v->uValue.fObject) assert(--refs >= 0); break;
        case eVK_Picture: if(v->uValue.fPicture) assert(--pictures >= 0); break;
        case eVK_ArrayUnicode:
            if(v->uValue.fArray.fData)
            {
                auto h = v->uValue.fArray.fData;
                for(int i = 0; i <= v->uValue.fArray.fNbElements; ++i)
                    dispose(reinterpret_cast<PA_Unistring *>(*h)[i]);
                assert(handles.erase(h) == 1);
                free(*h); free(h);
            }
            break;
        default: break;
    }
    memset(v, 0, sizeof(*v));
    v->fType = eVK_Undefined;
}
static void host(short selector, EngineBlock *eb)
{
    eb->fError = eER_NoErr;
    switch(selector)
    {
        case EX_CREATE_UNISTRING:
            eb->fUniString1 = makeString(reinterpret_cast<PA_Unichar *>(eb->fHandle)); break;
        case EX_SET_UNISTRING:
        {
            auto next = makeString(reinterpret_cast<PA_Unichar *>(eb->fHandle));
            dispose(eb->fUniString1); eb->fUniString1 = next; break;
        }
        case EX_DISPOSE_UNISTRING: dispose(eb->fUniString1); break;
        case EX_CLEAR_VARIABLE: clear(static_cast<PA_Variable *>(eb->fPtr1)); break;
        case EX_VARIABLE_TO_STRING:
        {
            auto v = reinterpret_cast<PA_Variable *>(eb->fHandle);
            assert(v->fType == eVK_Unistring || v->fType == 0);
            if(failString) {
                memset(&eb->fUniString1, 0xa5, sizeof(eb->fUniString1));
                eb->fError = eER_IncompatibleTypes; break;
            }
            eb->fUniString1 = v->uValue.fString; break;
        }
        case EX_GET_OBJ_VALUE:
            if(failGet)
            {
                // Deliberately invalid: failed results are unspecified, not owners.
                memset(eb->fPtr2, 0xa5, sizeof(PA_Variable));
                eb->fError = eER_IncompatibleTypes;
            }
            else *static_cast<PA_Variable *>(eb->fPtr2) = copyVariable(property);
            break;
        case EX_SET_OBJ_VALUE:
            clear(&stored);
            stored = copyVariable(*static_cast<PA_Variable *>(eb->fPtr2));
            break;
        case EX_EXECUTE_COMMAND_BY_ID:
        {
            auto out = static_cast<PA_Variable *>(eb->fPtr2);
            if(eb->fParam1 == 1231) {
                PA_SetBooleanVariable(out, !missing);
                if(failHas) eb->fError = eER_IncompatibleTypes;
                break;
            }
            if(eb->fParam1 == 1225) {
                PA_SetObjectVariable(out, reinterpret_cast<PA_ObjectRef>(1)); ++refs; break;
            }
            assert(eb->fParam1 == 1217);
            if(failCommand)
            {
                memset(out, 0xa5, sizeof(*out)); eb->fError = eER_IncompatibleTypes;
            }
            else *out = copyVariable(property);
            break;
        }
        case EX_HANDLE_MANAGER:
        {
            auto h = eb->fHandle;
            switch(eb->fParam1)
            {
                case 1:
                    h = static_cast<PA_Handle>(malloc(sizeof(*h)));
                    *h = static_cast<char *>(calloc(1, eb->fParam2 ? eb->fParam2 : 1));
                    handles[h] = eb->fParam2; eb->fHandle = h; break;
                case 2:
                    assert(handles.erase(h) == 1); free(*h); free(h); break;
                case 3:
                    assert(handles.count(h)); *h = static_cast<char *>(realloc(*h, eb->fParam2));
                    handles[h] = eb->fParam2; break;
                case 4: assert(handles.count(h)); eb->fParam2 = handles[h]; break;
                case 5: assert(handles.count(h)); eb->fParam3 = reinterpret_cast<sLONG_PTR>(*h); break;
                case 6: break;
                default: std::abort();
            }
            break;
        }
        case EX_CREATE_PICTURE:
            ++pictures; eb->fPicture = reinterpret_cast<void *>(1); break;
        case EX_DISPOSE_PICTURE: assert(--pictures >= 0); break;
        default:
            std::cerr << "DENIED engine selector " << selector << "\n";
            std::abort();
    }
}

// The dispatcher is real, but key parsing/crypto and wall-clock access are denied.
extern "C" void *ownership_BIO_new_mem_buf(const void *p, int n)
{
    assert(std::string(static_cast<const char *>(p), n) == "synthetic-only");
    ++bioCalls;
    return nullptr;
}
extern "C" int ownership_ftime(struct timeb *p)
{
    p->time = 123; p->millitm = 456; return 0;
}
extern "C" CFURLRef ownership_CFURLCreateWithFileSystemPath(CFAllocatorRef, CFStringRef, CFURLPathStyle, Boolean)
{
    std::abort();
}
extern "C" CFStringRef ownership_CFURLCopyFileSystemPath(CFURLRef, CFURLPathStyle)
{
    std::abort();
}
extern "C" void ownership_CGContextScaleCTM(CGContextRef, CGFloat, CGFloat) { std::abort(); }
extern "C" void ownership_CGContextTranslateCTM(CGContextRef, CGFloat, CGFloat) { std::abort(); }

void convertFromString(std::string &, C_TEXT &);
void convertToString(C_TEXT &, std::string &);

static void expect(bool ok) { if(!ok) throw std::runtime_error("content/lifecycle mismatch"); }
static PA_Variable textProperty(int raw = eVK_Unistring)
{
    PA_Variable v = {};
    v.fType = raw; v.uValue.fString = makeString(fixture); return v;
}
static void getCase(const std::string &name)
{
    auto obj = reinterpret_cast<PA_ObjectRef>(1);
    CUTF8String u8;
    CUTF16String u16;
    if(name == "s") ob_get_s(obj, L"key", &u8);
    if(name == "a") ob_get_a(obj, L"key", &u16);
    if(name == "b") ob_get_b(obj, L"key");
    if(name == "n") ob_get_n(obj, L"key");
    if(name == "o")
    {
        auto result = ob_get_o(obj, L"key");
        if(result) PA_DisposeObject(result);
    }
    if(name == "c")
    {
        auto result = ob_get_c(obj, L"key");
        if(result) PA_DisposeCollection(result);
    }
    if((property.fType == eVK_Unistring || property.fType == 0) && !failGet && !missing)
    {
        if(name == "a") expect(u16 == CUTF16String(property.uValue.fString.fString));
        if(name == "s") expect(u8 == CUTF8String(reinterpret_cast<const uint8_t *>(
            property.uValue.fString.fLength ? "A\xe6\x97\xa5\xf0\x9f\x98\x80" : "")));
    }
}
static void test(const std::string &name)
{
    auto obj = reinterpret_cast<PA_ObjectRef>(1);
    if(name.find("get-") == 0)
    {
        auto kind = name.substr(6);
        if(kind == "text") property = textProperty();
        else if(kind == "raw0") property = textProperty(0);
        else if(kind == "empty") {
            property.fType = eVK_Unistring; property.uValue.fString = makeString(empty);
        }
        else if(kind == "boolean") {
            PA_SetBooleanVariable(&property, true);
        }
        else if(kind == "picture") {
            property.fType = eVK_Picture;
            property.uValue.fPicture = reinterpret_cast<void *>(1); ++pictures;
        }
        else if(kind == "object" || kind == "collection")
        {
            property.fType = kind == "object" ? eVK_Object : eVK_Collection;
            property.uValue.fObject = obj; ++refs;
        }
        else if(kind == "error") { property.fType = eVK_Undefined; failGet = true; }
        else if(kind == "haserror") { property.fType = eVK_Undefined; failHas = true; }
        else if(kind == "missing") { property = textProperty(); missing = true; }
        else if(kind == "stringerror") { property = textProperty(); failString = true; }
        else { property.fType = eVK_Real; property.uValue.fReal = 12.5; }
        bool caught = false;
        try { getCase(name.substr(4, 1)); } catch(const std::runtime_error &) { caught = true; }
        expect(caught == (failGet || failHas || (failString && (name[4] == 's' || name[4] == 'a'))));
        clear(&property);
    }
    else if(name.find("set-") == 0)
    {
        auto which = name.substr(4);
        CUTF16String value(fixture);
        if(which == "ss") ob_set_s(obj, "key", "synthetic");
        if(which == "ws") ob_set_s(obj, L"key", "synthetic");
        if(which == "wa") ob_set_a(obj, L"key", &value);
        if(which == "ww") ob_set_a(obj, L"key", L"synthetic");
        if(which == "empty") ob_set_s(obj, "key", "");
        if(which == "bmp") ob_set_s(obj, "key", "\xe6\x97\xa5");
        if(which == "supplementary") ob_set_s(obj, "key", "\xf0\x9f\x98\x80");
        if(which == "picture")
        {
            ++pictures;
            ob_set_p(obj, L"key", reinterpret_cast<PA_Picture>(1));
        }
        if(which == "number") ob_set_n(obj, L"key", 12.5);
        if(which == "number-char") ob_set_n(obj, "key", 12.5);
        if(which == "bool") ob_set_b(obj, L"key", true);
        if(which == "null") ob_set_0(obj, L"key");
        if(which == "null-char") ob_set_0(obj, "key");
        if(which == "integer") ob_set_i(obj, L"key", 42);
        if(which == "object" || which == "object-char") {
            ++refs;
            if(which == "object") ob_set_o(obj, L"key", obj);
            else ob_set_o(obj, "key", obj);
        }
        if(which == "collection") { ++refs; ob_set_c(obj, L"key", obj); }
        if(stored.fType == eVK_Unistring) {
            CUTF16String expected = which == "wa" ? CUTF16String(fixture) :
                which == "empty" ? CUTF16String() : which == "bmp" ? CUTF16String(fixture + 1, 1) :
                which == "supplementary" ? CUTF16String(fixture + 2, 2) :
                CUTF16String({'s','y','n','t','h','e','t','i','c'});
            expect(CUTF16String(stored.uValue.fString.fString) == expected);
        }
        clear(&stored);
    }
    else if(name.find("stringify-") == 0)
    {
        if(name == "stringify-number") {
            property.fType = eVK_Real; property.uValue.fReal = 12.5;
        } else property = textProperty(name == "stringify-raw0" ? 0 : eVK_Unistring);
        failCommand = name == "stringify-error";
        failString = name == "stringify-stringerror";
        CUTF8String result;
        bool caught = false;
        try { ob_stringify(obj, &result); } catch(const std::runtime_error &) { caught = true; }
        expect(caught == (failCommand || failString || name == "stringify-number"));
        if(!caught) expect(result.size() == 8);
        clear(&property);
    }
    else if(name == "copy-text" || name == "assign-text")
    {
        C_TEXT target;
        {
            C_TEXT source;
            source.setUTF16String(fixture, 4);
            if(name == "assign-text") target = source;
            else { C_TEXT copy(source); copy.setUTF16String(empty, 0); }
            expect(source.getUTF16Length() == 4);
        }
        if(name == "assign-text") expect(target.getUTF16Length() == 4);
    }
    else if(name == "copy-array" || name == "assign-array")
    {
        ARRAY_TEXT target;
        {
            ARRAY_TEXT source;
            source.appendUTF16String(fixture);
            if(name == "assign-array") target = source;
            else { ARRAY_TEXT copy(source); copy.setSize(0); }
            expect(source.getSize() == 1);
        }
        if(name == "assign-array") expect(target.getSize() == 1);
    }
    else if(name == "copy-blob" || name == "assign-blob")
    {
        C_BLOB target;
        const uint8_t bytes[] = {1, 2, 3};
        {
            C_BLOB source;
            source.setBytes(bytes, 3);
            if(name == "assign-blob") target = source;
            else { C_BLOB copy(source); copy.addBytes(bytes, 3); }
            expect(source.getBytesLength() == 3);
        }
        if(name == "assign-blob") expect(target.getBytesLength() == 3);
    }
    else if(name.find("array-") == 0)
    {
        PA_Variable arr = PA_CreateVariable(eVK_ArrayUnicode);
        BytePtr params[] = {reinterpret_cast<BytePtr>(&arr)};
        ARRAY_TEXT values;
        values.appendUTF16String(empty);
        CUTF16String expected = name == "array-ascii" ? CUTF16String(fixture, 1) :
            name == "array-bmp" ? CUTF16String(fixture + 1, 1) :
            name == "array-supplementary" ? CUTF16String(fixture + 2, 2) :
            name == "array-empty" ? CUTF16String() : CUTF16String(fixture);
        values.appendUTF16String(&expected);
        values.toParamAtIndex(params, 1);
        expect(PA_GetStringInArray(arr, 1).fLength == expected.size());
        expect(PA_GetStringInArray(arr, 1).fString[expected.size()] == 0);
        for(int i = 0; i < 32; ++i) dispose(makeString(fixture));
        auto first = PA_GetStringInArray(arr, 1).fString;
        ARRAY_TEXT read;
        read.fromParamAtIndex(params, 1);
        CUTF16String result;
        read.copyUTF16StringAtIndex(&result, 1);
        expect(result == expected);
        if(name == "array-replace")
        {
            values.toParamAtIndex(params, 1);
            expect(PA_GetStringInArray(arr, 1).fString != first);
        }
        if(name == "array-shrink") PA_ResizeArray(&arr, 0);
        if(name == "array-independent") {
            PA_Variable other = PA_CreateVariable(eVK_ArrayUnicode);
            BytePtr output[] = {reinterpret_cast<BytePtr>(&other)};
            values.toParamAtIndex(output, 1);
            expect(PA_GetStringInArray(arr, 1).fString != PA_GetStringInArray(other, 1).fString);
            PA_ClearVariable(&other);
            expect(CUTF16String(PA_GetStringInArray(arr, 1).fString) == expected);
        }
        PA_ClearVariable(&arr);
    }
    else if(name.find("transfer-") == 0)
    {
        auto s = makeString(fixture);
        auto arr = PA_CreateVariable(eVK_ArrayUnicode);
        int index = name == "transfer-index" ? -1 : name == "transfer-upper" ? 1 : 0;
        if(name == "transfer-kind") arr.fType = eVK_ArrayLongint;
        auto originalData = arr.uValue.fArray.fData;
        if(name == "transfer-null") arr.uValue.fArray.fData = nullptr;
        PA_SetStringInArray(arr, index, &s);
        if(name != "transfer-zero") dispose(s);
        arr.uValue.fArray.fData = originalData;
        arr.fType = eVK_ArrayUnicode;
        clear(&arr);
    }
    else if(name.find("text-") == 0 || name.find("consumer-") == 0)
    {
        auto kind = name.substr(name.find('-') + 1);
        const char *input = kind == "ascii" ? "ABC" :
                            kind == "bmp" ? "\xe6\x97\xa5" :
                            kind == "supplementary" ? "\xf0\x9f\x98\x80" : "";
        C_TEXT text;
        std::string source(input);
        if(name.find("consumer-") == 0) convertFromString(source, text);
        else text.setUTF8String(reinterpret_cast<const uint8_t *>(input), strlen(input));
        expect(text.getUTF16Length() == (kind == "ascii" ? 3 : kind == "bmp" ? 1 :
                                      kind == "supplementary" ? 2 : 0));
        expect(text.getUTF16StringPtr()[text.getUTF16Length()] == 0);
        CUTF8String out;
        text.copyUTF8String(&out);
        expect(out == CUTF8String(reinterpret_cast<const uint8_t *>(input)));
        std::string consumerOut;
        convertToString(text, consumerOut);
        expect(consumerOut == input);
        text.setUTF8String(reinterpret_cast<const uint8_t *>(""), 0);
        expect(text.getUTF16Length() == 0);
    }
    else if(name.find("defined-") == 0) {
        missing = name == "defined-missing";
        failHas = name == "defined-error";
        bool caught = false, result = false;
        try { result = ob_is_defined(obj, L"key"); } catch(const std::runtime_error &) { caught = true; }
        expect(caught == failHas);
        if(!caught) expect(result == !missing);
    }
    else if(name == "scalar-synchronous") {
        auto s = makeString(fixture);
        PA_Variable v;
        PA_SetStringVariable(&v, &s);
        expect(CUTF16String(PA_GetStringVariable(v).fString) == CUTF16String(fixture));
        dispose(s); // synchronous borrower has finished; v is not an independent owner
    }
    else if(name.find("dispatch-") == 0)
    {
        auto a = makeString(empty), b = makeString(empty), c = makeString(empty);
        if(name == "dispatch-sign") {
            dispose(a); dispose(b);
            PA_Unichar json[] = {'{','}',0};
            a = makeString(json); b = makeString(json);
        }
        dispose(c);
        PA_Unichar dummy[] = {'s','y','n','t','h','e','t','i','c','-','o','n','l','y',0};
        c = makeString(dummy);
        if(name == "dispatch-verify") { dispose(b); b = makeString(dummy); }
        BytePtr params[] = {reinterpret_cast<BytePtr>(&a), reinterpret_cast<BytePtr>(&b), reinterpret_cast<BytePtr>(&c)};
        auto result = makeString(empty);
        PA_Unistring *resultPtr = &result;
        PA_long32 number = -1;
        void *output = name == "dispatch-verify" ? static_cast<void *>(&number) : &resultPtr;
        FourDPackex(name == "dispatch-sign" ? 2 : name == "dispatch-verify" ? 3 : 1, params, nullptr, output);
        if(name == "dispatch-sign") expect(result.fLength == 0);
        else if(name == "dispatch-verify") expect(number == 0);
        else { PA_Unichar expected[] = {'1','2','3','4','5','6',0}; expect(CUTF16String(result.fString) == CUTF16String(expected)); }
        expect(bioCalls == (name == "dispatch-timestamp" ? 0 : 1));
        dispose(result); dispose(a); dispose(b); dispose(c);
    }
    else std::abort();
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    gCall4D = host;
    stored.fType = eVK_Undefined;
    try { test(argv[1]); }
    catch(const std::exception &e) { std::cerr << e.what() << "\n"; return 1; }
    if(!strings.empty() || !handles.empty() || refs || pictures)
    {
        std::cerr << "unbalanced owners strings=" << strings.size() << " handles=" << handles.size()
                  << " refs=" << refs << " pictures=" << pictures << "\n";
        return 1;
    }
    std::cout << "PASS " << argv[1] << "\n";
}
