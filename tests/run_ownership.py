#!/usr/bin/env python3
"""Compile actual sources, never project build phases; use only synthetic fixtures."""
import argparse
import hashlib
import json
import os
import platform
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1])
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
root = args.source.resolve()
args.output.mkdir(parents=True, exist_ok=True)
api = root / "jwt/4D Plugin API"
classes = api / "Classes"
tests = Path(__file__).resolve().parent
sources = [api / "4DPluginAPI.c", *[classes / f"{n}.cpp" for n in
           ("C_TEXT", "C_LONGINT", "C_BLOB", "ARRAY_TEXT")],
           root / "jwt/support/4DPlugin-JSON.cpp", root / "jwt/4DPlugin-JWT.cpp"]
common = ["-g", "-O1", "-fsanitize=address,undefined", "-fno-sanitize=alignment",
          "-Wno-deprecated-declarations", "-Wno-multichar", "-Wno-int-conversion",
          "-I" + str(root / "jwt"), "-I" + str(api), "-I" + str(classes), "-I" + str(root / "jwt/support"),
          "-I" + str(root / "jwt/include")]
with tempfile.TemporaryDirectory(prefix="jwt-ownership-") as work:
    work = Path(work)
    objects = []
    commands = []
    for i, source in enumerate(sources):
        obj = work / f"{i}.o"
        cmd = (["clang", "-c", str(source), "-o", str(obj)] if source.suffix == ".c" else
               ["clang++", "-std=c++14", "-x", "objective-c++", "-c", str(source), "-o", str(obj)])
        cmd += common
        cmd += ["-DCFURLCreateWithFileSystemPath=ownership_CFURLCreateWithFileSystemPath",
                "-DCFURLCopyFileSystemPath=ownership_CFURLCopyFileSystemPath",
                "-DCGContextScaleCTM=ownership_CGContextScaleCTM",
                "-DCGContextTranslateCTM=ownership_CGContextTranslateCTM"]
        if source.name == "4DPlugin-JWT.cpp":
            cmd += ["-DBIO_new_mem_buf=ownership_BIO_new_mem_buf", "-Dftime=ownership_ftime"]
        commands.append(cmd)
        subprocess.run(cmd, check=True)
        objects.append(str(obj))
    exe = work / "ownership"
    cmd = ["clang++", "-std=c++14", "-x", "objective-c++", str(tests / "ownership.cpp"),
           str(tests / "crypto_boundary.cpp"),
           *common, "-x", "none", *objects, str(root / "jwt/a/libjwtpp.a"),
           str(root / "jwt/a/libjsoncpp_static.a"),
           "-framework", "Cocoa", "-framework", "CoreGraphics", "-o", str(exe)]
    commands.append(cmd)
    subprocess.run(cmd, check=True)
    # Retain exact linked imports for review; BIO and wall-clock boundaries are local stubs.
    imports = subprocess.check_output(["nm", "-u", str(exe)], text=True)
    (args.output / "imports.txt").write_text(imports)
    allowed = {
        "_CFRelease", "_CFStringCreateWithBytes", "_CFStringCreateWithCharacters",
        "_CFStringGetBytes", "_CFStringGetCharacters", "_CFStringGetLength",
        "_CFStringGetMaximumSizeForEncoding", "_kCFAllocatorDefault",
        "_OBJC_CLASS_$_NSMutableString", "_OBJC_CLASS_$_NSString", "_OBJC_CLASS_$_NSURL",
        "_objc_alloc", "_objc_msgSend", "_objc_release", "___CFConstantStringClassReference",
        "__DefaultRuneLocale", "___maskrune", "___fpclassifyd", "__Unwind_Resume",
        "___gxx_personality_v0", "___assert_rtn", "___stack_chk_fail", "___stack_chk_guard",
        "__tlv_bootstrap", "_abort", "_bzero", "_calloc", "_free", "_malloc", "_memchr",
        "_memcmp", "_memcpy", "_memmove", "_memset", "_modf", "_realloc", "_snprintf",
        "_sprintf", "_strlen", "_strncmp", "_strtoll", "_wcslen",
    }
    prefixes = ("__Z", "___asan_", "___ubsan_", "___cxa_", "___sanitizer_")
    unexpected = [line.strip() for line in imports.splitlines()
                  if line.strip() not in allowed and not line.strip().startswith(prefixes)]
    if unexpected:
        raise RuntimeError(f"Denied unexpected imports before execution: {unexpected}")
    cases = [f"get-{getter}-{kind}" for getter in "sabnoc" for kind in
             ("text", "raw0", "empty", "boolean", "number", "picture", "object", "collection",
              "error", "haserror", "missing", "stringerror")]
    cases += [f"set-{kind}" for kind in ("ss", "ws", "wa", "ww", "empty", "bmp", "supplementary",
              "picture", "number", "number-char", "bool", "null", "null-char", "integer",
              "object", "object-char", "collection")]
    cases += [f"{op}-{kind}" for op in ("copy", "assign") for kind in ("text", "array", "blob")]
    cases += ["stringify-text", "stringify-raw0", "stringify-number", "stringify-error", "stringify-stringerror",
              "array-roundtrip", "array-replace", "array-shrink", "array-independent", "array-ascii", "array-bmp",
              "array-supplementary", "array-empty",
              "transfer-zero", "transfer-index", "transfer-kind", "transfer-upper", "transfer-null",
              "text-ascii", "text-bmp", "text-supplementary", "text-empty",
              "consumer-ascii", "consumer-bmp", "consumer-supplementary", "consumer-empty",
              "defined-present", "defined-missing", "defined-error", "scalar-synchronous",
              "dispatch-timestamp", "dispatch-sign", "dispatch-verify"]
    results = []
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1")
    for case in cases:
        result = subprocess.run([str(exe), case], env=env, text=True, capture_output=True)
        (args.output / f"{case}.log").write_text(result.stdout + result.stderr)
        results.append({"case": case, "passed": result.returncode == 0, "returncode": result.returncode})
        print(("PASS " if result.returncode == 0 else "FAIL ") + case, flush=True)
        if result.returncode:
            print((result.stdout + result.stderr)[:10000], flush=True)
    manifest = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    manifest["harness"] = hashlib.sha256((tests / "ownership.cpp").read_bytes()).hexdigest()
    manifest["runner"] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    manifest["crypto_boundary"] = hashlib.sha256((tests / "crypto_boundary.cpp").read_bytes()).hexdigest()
    for path in [*api.glob("*.h"), *api.glob("*.def"), *classes.glob("*.h"),
                 root / "jwt/support/4DPlugin-JSON.h", root / "jwt/4DPlugin-JWT.h",
                 root / "jwt/a/libjwtpp.a", root / "jwt/a/libjsoncpp_static.a"]:
        manifest[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    report = {"sources": manifest, "commands": commands, "cases": results,
              "platform": {"machine": platform.machine(), "macOS": platform.mac_ver()[0]},
              "compiler": subprocess.check_output(["clang", "--version"], text=True),
              "passed": sum(r["passed"] for r in results), "total": len(results)}
    (args.output / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("passed", "total")}))
    raise SystemExit(0 if report["passed"] == report["total"] else 1)
