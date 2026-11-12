# Native ownership audit

## Provenance and scope

Baseline: `378125fbd269c834554fae1f1e36acfbd410fa63`. The immutable and
current `miyako/skills/4d-skills/skills/4dplugin/SKILL.md` both resolved to
blob `aabdd402ca8bd87be6c84a1a61f2118ad4d4cb13`; the immutable skills
revision was `e6b492fcb81e5e1a7f7db723594b420e63bf6eb9`.

SDK labels (`v14` implementation, `v13` headers) are not upstream release
identities. The actual bundled files below are unchanged:

| File under `jwt/4D Plugin API/` | Git blob |
|---|---|
| `4DPluginAPI.c` | `62f745e91d81ef2c3c901c656ea8e175d3b173e8` |
| `4DPluginAPI.h` | `528bf1d2552df08d6a8b1bc95011a9b72934ad5c` |
| `PrivateTypes.h` | `a3098cb662cdbc43ba574fdef72a059c4c6d1cd4` |
| `PublicTypes.h` | `9ae20b001bff9128f690fff3e6c3f873b6238333` |
| `EntryPoints.h` | `da428d5f6ca23b0a30371c7288a01d29aa3c466a` |
| `Flags.h` | `7a5a35539f27430a3fd9cb711d6b789051316471` |
| `4DPluginAPI.def` | `5bbdb82fb724b6f8e307feb01d9f158b802de89d` |

| Native surface | Membership and consumers |
|---|---|
| `4DPluginAPI.c`, `C_TEXT.cpp`, `C_LONGINT.cpp`, `4DPlugin-JWT.cpp` | Current Xcode and Visual Studio source memberships. `FourDPackex` calls `PluginMain`; selectors 1/2/3 call Timestamp/Sign/Verify. |
| `ARRAY_TEXT.cpp`, `C_BLOB.cpp` and their headers | Dormant wrappers, not current project build members. Full implementations are compiled by the regression harness. |
| `support/4DPlugin-JSON.cpp/.h` | Dormant property setters/getters and stringify helper; no dispatcher consumers. Compiled in full by the harness. |
| `templates/main.cpp/.h` and template projects | Wizard templates, not native build members; no additional implemented command or ownership consumer. |
| `include/json`, `include/jwtpp`, `include/josepp`, `include/openssl` and bundled libraries | Third-party declarations/inline helpers and prebuilt implementations. No additional 4D ownership consumers. Legacy josepp includes remain, but the current dispatcher uses jwtpp/jsoncpp/OpenSSL. |
| Packaged test plugin and archives | Prebuilt artifacts, not rebuilt or changed. Both manifest copies have blob `b23ddc1fea770017e7e14f2f425e248bb098b671`. |

Both manifests declare only `JWT Timestamp(&T;&T):T`,
`JWT Sign(&T;&T;&T):T`, and `JWT Verify(&T;&T):L`. There are no exported
array, picture, object, collection, or callback arguments. Actual `C_TEXT`
consumers pass references or pointers; none copies an owner by value.
The copy defects below are therefore dormant capabilities, not demonstrated
command failures. No independently retained asynchronous callback exists.

## Contracts and changes

The bundled SDK's string-array setter (lines 4933-4947) accepts Unicode
arrays with non-null data and indices `0 <= i <= fNbElements`. It disposes
the old element and shallow-assigns the incoming descriptor. Array getters
(4738-4755) borrow their element. `ARRAY_TEXT` already allocates independent
outgoing strings and does not dispose transferred descriptors; that valid
behavior is preserved. Its incoming values are copied into C++ strings.
Rejected low/high/type/null destinations retain the caller's allocation.

`PA_SetStringVariable` (4254-4259) and the picture/object/collection scalar
setters shallow-assign; they neither release an old owner nor create an
independent one. `PA_CreateVariable` creates an allocated empty text/picture.
Four JSON string setter overloads and the picture setter overwrote these
initial owners. They now initialize the descriptor through the setter
instead. Existing synchronous cleanup of the supplied descriptor is
preserved. In particular, this change does **not** invent a new borrowed
picture contract for external callers: the helper's existing supplied-picture
cleanup remains unchanged. Object/collection setter handoffs likewise keep
their previous ownership behavior.

Successful `PA_GetObjectProperty` results are explicitly documented by this
SDK as requiring `PA_ClearVariable`. Scalar getters and mismatched reference
getters now clear these owners through a scoped result. Matching object and
collection getters continue to hand their result reference to the caller,
who must dispose it. Raw type zero is tested as text because
`PA_GetVariableKind` normalizes it to Unicode.

Errors are captured immediately after property existence/read, string access,
or command execution, before cleanup can overwrite the error. Failed results
are not adopted, classified, or cleared. Engine failures throw
`std::runtime_error`; missing properties and successful type mismatches keep
their previous return/default behavior. Stringify borrows its object only
for the synchronous read-only command, checks the returned kind, and clears
successful non-text results as well as successful text results.

`C_TEXT`, `ARRAY_TEXT`, and `C_BLOB` previously shallow-copied their owning
heap pointer. Named copy-constructor and copy-assignment consumers each
reproduce an ASan use-after-free when one owner is destroyed and the other
is used. Explicit deep copies preserve object layout and existing copyable
interfaces. `C_LONGINT` and `CBytes` already contain independently copyable
value storage; the `C_BLOB` copy includes its byte cursor.

Unicode conversion paths use CoreFoundation UTF-16 code-unit lengths and
UTF-8 byte lengths; there is no SDK byte-to-unit conversion call in these
wrappers. Scalar output uses the engine-backed content setter, not the
array's descriptor transfer. ASCII, BMP, supplementary, and empty fixtures
assert exact lengths, contents, termination, round trips, replacement,
reuse, churn, independent output owners, and final cleanup.

## Reproducible regression

Run on macOS, without invoking any project configuration:

```sh
python3 tests/run_ownership.py --output /tmp/jwt-ownership-results
```

`--source /path/to/archived/native/source` runs the **same harness** on
another immutable source tree. The baseline was extracted by `git archive`
with only native source/SDK/include/library paths, excluding test databases,
user preferences, and key fixtures. Full unchanged SDK implementation, all
four wrappers, JSON helpers, and the actual JWT dispatcher are compiled.
Results include every named case, per-case log, all source/SDK/library hashes,
compiler commands/version, platform, and linked imports.

The local final run used Apple clang 21.0.0, arm64, macOS 26.7:

| Case family | Cases | Baseline pass/fail | Fixed pass/fail |
|---|---:|---:|---:|
| Property getters | 72 | 20 / 52 | 72 / 0 |
| Property setters | 17 | 9 / 8 | 17 / 0 |
| Wrapper copy and assignment | 6 | 0 / 6 | 6 / 0 |
| Stringify | 5 | 2 / 3 | 5 / 0 |
| Array lifecycle and independent output | 8 | 8 / 0 | 8 / 0 |
| Transfer acceptance/rejection | 5 | 5 / 0 | 5 / 0 |
| Wrapper and actual consumer Unicode | 8 | 8 / 0 | 8 / 0 |
| Property existence | 3 | 2 / 1 | 3 / 0 |
| Synchronous scalar borrower | 1 | 1 / 0 | 1 / 0 |
| Exported dispatcher | 3 | 3 / 0 | 3 / 0 |
| **Total** | **128** | **58 / 70** | **128 / 0** |

Exact case names are generated by `tests/run_ownership.py` and retained in
`results.json`; no cases are skipped. The baseline failures are:

- `get-{s,a,b,n}-{text,raw0,empty,picture,object,collection,error,haserror,stringerror}`
- `get-o-{text,raw0,empty,picture,collection,error,haserror,stringerror}`
- `get-c-{text,raw0,empty,picture,object,error,haserror,stringerror}`
- `set-{ss,ws,wa,ww,empty,bmp,supplementary,picture}`
- `{copy,assign}-{text,array,blob}`
- `stringify-{number,error,stringerror}` and `defined-error`

Every other named case is one of the 58 baseline passing controls and also
passes fixed. In particular the valid transfer, borrowed getter,
synchronous scalar cleanup, matching reference handoff, Unicode unit, and
exported-command fixtures were already passing, not newly invented failures.

| Evidence | SHA-256 |
|---|---|
| Baseline native archive | `ce40344dd3ec47369efc492a4272953f9636145ffdc6fe713789cf839c67fadf` |
| `tests/ownership.cpp` | `cc61349b0300e9f548860912904e83274c6f50ef12b3d88f528efc008371e08f` |
| `tests/run_ownership.py` | `51738d180fba9aacf8aac106ca0ef5abca7a992d97f63c789b62ca7c5bdb1b78` |
| `tests/crypto_boundary.cpp` | `b8f5b090418305a1abf46ca3ff94d9aec9f5d4c68c4e9cc7f477c1d1483e8957` |
| Baseline `results.json` | `381a890ef42b2f21536c483bdbd39e264c41dbbb04c07494a2f41f23466d5419` |
| Fixed `results.json` | `a1158b305afa31786a47cf181d7f2c249c3a84469ff47f1f5c16eb5ce9df2a4c` |

Immutable local evidence, full logs, source archive, and a copy of the final
harness are retained outside the worktree in this audit session's
audit artifacts. CI runs only the fixed regression on `macos-15`.

## Safety and limits

No Xcode/MSBuild project build is run. Current Deployment phases sign,
package, and notarize; the legacy Xcode/template phases also sign manifest
files and are unsafe even without assuming a Deployment guard. No signing,
notarization, packaging, releases, tags, publication, workflow dispatch,
reruns, or real 4D test databases are part of validation. Local Git hooks
are bypassed for publishing this audit instead of running shared-checkout
hooks. No existing Actions workflow was present before adding the regression.

The new workflow has only `contents: read`, credential persistence disabled,
a pinned checkout, and one compiler/test step. Its event matrix is:
PR **base** `master` -> run; push `master` -> run; feature push -> no run;
tags/manual dispatch -> no run. It does not call any project build phase,
read secrets, or upload artifacts.

The host shim permits only synthetic memory/string/reference/array operations
and known read-only command IDs; unexpected selectors abort. Failed-result
fixtures poison output descriptors. The linked binary's imports are
allowlisted before execution. Crypto implementations are not linked:
all crypto entry points abort, except the explicit dummy BIO rejection
boundary. Wall clock, CFURL filesystem conversion, and graphics transforms
are stubbed. Foundation/CoreFoundation conversion and C++ runtime imports
remain; dormant Objective-C URL convenience methods are not executed.
Only in-memory synthetic data is used. Timestamp checks a fixed clock;
Sign uses `{}` and `synthetic-only` with rejected BIO; Verify uses an empty
bearer and the same dummy input. These exercise the real exported
`FourDPackex`/dispatcher and wrapper lifetimes, **not** real signing or key
parsing. Actual cryptographic correctness is outside this ownership test.

ASan and UBSan instrument the complete native source set. UBSan alignment is
explicitly disabled because this SDK ABI deliberately uses two-byte packing;
this is not evidence that native packed accesses are portable. LSan is
disabled/unsupported in this macOS run; independent explicit allocation,
handle, picture, and reference balances detect the modeled leaks.
Bundled jsoncpp/jwtpp libraries are prebuilt and not sanitizer-instrumented.
No native Windows, real 4D host, allocator/OOM fault-injection, asynchronous
host callback, actual picture payload, filesystem convenience-method, or
third-party crypto test was performed. Engine behavior beyond the documented
SDK ownership and forwarding contracts is not claimed. No native-host crash
reproduction or blanket SDK correctness claim is made.
