#!/usr/bin/env bash
# Builds jsoncpp 1.9.4 as a Windows x64 static lib matching the vendored
# headers in include/json (verified byte-identical to the 1.9.4 release).
# Output: lib/windows64/jsoncpp.lib  (MT static runtime, same toolset as the plugin)
set -euo pipefail
cd "$(dirname "$0")/.."

TARBALL=jsoncpp-1.9.4.tar.gz
SHA256=e34a628a8142643b976c7233ef381457efad79468c67cb1ae0b83a33d7493999
URL=https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.4.tar.gz
TOOLSET="${PLATFORM_TOOLSET:-v143}"
WORK=.windows-deps/jsoncpp
OUT=lib/windows64/jsoncpp.lib

if [ -f "$OUT" ]; then
  echo "jsoncpp.lib already present - skipping build"
  exit 0
fi

mkdir -p "$WORK"
cd "$WORK"
if [ ! -f "$TARBALL" ]; then
  curl -fsSL -o "$TARBALL" "$URL"
fi
echo "$SHA256  $TARBALL" | sha256sum -c -
tar xzf "$TARBALL"
SRC=jsoncpp-1.9.4

# The vendored headers must match the release we build, or the link fails
# with missing/extra symbols (playbook section on vendored header/lib drift).
diff -r --strip-trailing-cr "$SRC/include/json" "$OLDPWD/include/json"

cmake -S "$SRC" -B build -A x64 -T "$TOOLSET" \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded \
  -DJSONCPP_WITH_TESTS=OFF \
  -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_OBJECT_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX="$OLDPWD/.windows-deps/jsoncpp/install"
cmake --build build --config Release --target jsoncpp_static

LIB=$(find build -name 'jsoncpp*.lib' | head -1)
[ -n "$LIB" ] || { echo "jsoncpp static lib not found"; exit 1; }
mkdir -p "$OLDPWD/lib/windows64"
cp "$LIB" "$OLDPWD/$OUT"

# Sanity: the symbol families the plugin needs and the static MT runtime
# directive (MSVC embeds /FAILIFMISMATCH:RuntimeLibrary=... in every object).
for sym in '??AStreamWriterBuilder@Json@@' 'deref@ValueIteratorBase@Json@@' '??4Value@Json@@QEAAAEAV01@$$'; do
  grep -aqF "$sym" "$OLDPWD/$OUT" || { echo "MISSING $sym in $OUT"; exit 1; }
done
grep -aqF 'RuntimeLibrary=MT_StaticRelease' "$OLDPWD/$OUT" || { echo "RuntimeLibrary mismatch in $OUT"; exit 1; }
echo "built $OUT"
