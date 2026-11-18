#!/usr/bin/env bash
# Builds josepp/jwtpp at the exact upstream commit matching the vendored
# headers in include/jwtpp (verified identical to troian/jwtpp@4ce1b9bd59).
# Output: lib/windows64/jwtpp.lib  (MT static runtime, same toolset as the plugin)
set -euo pipefail
cd "$(dirname "$0")/.."

REPO=troian/jwtpp
SHA=4ce1b9bd5939b71fbabc4fc914815a3cf44718b3
TOOLSET="${PLATFORM_TOOLSET:-v143}"
WORK=.windows-deps/jwtpp
OUT=lib/windows64/jwtpp.lib

if [ -f "$OUT" ]; then
  echo "jwtpp.lib already present - skipping build"
  exit 0
fi

ROOT=$(pwd)
W=$(cygpath -m "$ROOT")
mkdir -p "$WORK/src" "$WORK/build" "$WORK/openssl/include" "$WORK/openssl/lib"

# OpenSSL shim so FindOpenSSL resolves against the repo's vendored headers/libs.
cp -r "$ROOT/include/openssl" "$WORK/openssl/include/"
cp "$ROOT/lib/windows64/libcrypto.lib" "$ROOT/lib/windows64/libssl.lib" "$WORK/openssl/lib/"

cd "$WORK/src"
if [ ! -d .git ]; then
  git init -q
  git remote add origin "https://github.com/$REPO.git"
  git fetch -q --depth 1 origin "$SHA"
  git checkout -q FETCH_HEAD
fi

cmake -S . -B "../build" -A x64 -T "$TOOLSET" \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded \
  -DOPENSSL_ROOT_DIR="$W/.windows-deps/jwtpp/openssl" \
  -DOPENSSL_INCLUDE_DIR="$W/include" \
  -DJsonCPP_INCLUDE_DIRS="$W/include" \
  -DJsonCPP_LIBRARIES="$W/lib/windows64/jsoncpp.lib" \
  -DJsonCPP_LIBRARY_DIRS="$W/lib/windows64" \
  -DJWTPP_WITH_TESTS=OFF \
  -DJWTPP_WITH_COVERAGE=OFF \
  -DJWTPP_WITH_INSTALL=OFF \
  -DJWTPP_WITH_SHARED_LIBS=OFF
cmake --build ../build --config Release --target jwtpp-static

LIB=$(find ../build -name 'jwtpp.lib' | head -1)
[ -n "$LIB" ] || { echo "jwtpp.lib not found"; exit 1; }
mkdir -p "$ROOT/lib/windows64"
cp "$LIB" "$ROOT/$OUT"

# Sanity: the claims ctor used by the plugin and the static MT runtime
# directive (MSVC embeds /FAILIFMISMATCH:RuntimeLibrary=... in every object).
grep -aqF '??0claims@jwtpp@@QEAA@AEBV' "$ROOT/$OUT" \
  || { echo "claims ctor missing in $OUT"; exit 1; }
grep -aqF 'RuntimeLibrary=MT_StaticRelease' "$ROOT/$OUT" \
  || { echo "RuntimeLibrary mismatch in $OUT"; exit 1; }
echo "built $ROOT/$OUT"
