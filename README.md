![version](https://img.shields.io/badge/version-17%2B-3E8B93)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-jwt)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-jwt/total)

# 4d-plugin-jwt
Sign and Verify JSON Web Token based on ~~[josepp](https://github.com/troian/josepp)~~ [jwtpp](https://github.com/troian/jwtpp) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp).

### Native solution available

Please note that [CryptoKey](https://developer.4d.com/docs/API/CryptoKeyClass/#4dcryptokeynew) can be used to process JWT.

c.f. https://blog.4d.com/cryptokey-encrypt-decrypt-sign-and-verify/

## Included In This Repo
• Source code for the plugin
• Test database (folder `test`)

## Native ownership regression

The [ownership audit](docs/ownership-audit.md) records bundled SDK contracts,
compiled and dormant helper coverage, paired baseline/fixed evidence, and
platform limits. The safe synthetic regression runs without project signing
or packaging phases:

```sh
python3 tests/run_ownership.py --output /tmp/jwt-ownership-results
```

## Installation Instructions
1. Extract either the **JWT.DMG** or the **JWT.zip** archives
2. Add the **JWT.bundle** to your database's **Plugins** folder.

## Command Reference

| Command | Syntax | Returns |
|---------|--------|---------|
| JWT Timestamp | `JWT Timestamp ({time1{;time2}})` | TEXT |
| JWT Sign | `JWT Sign (header; claim; privateKey)` | TEXT |
| JWT Verify | `JWT Verify (bearer; privateKey)` | LONGINT |

Platforms: macOS (universal arm64 + x86_64, macOS 11.5+) and Windows x64.

### JWT Timestamp

Returns the current UNIX time in **milliseconds** as text.

Parameter|Type|Description
------------|------------|----
time1|TEXT|anchor time in milliseconds, or `""` for the current time
time2|TEXT|offset in milliseconds added to `time1`
timestamp|TEXT|UNIX time in milliseconds

If `time1` is empty or not numeric, the current system time is used.
`time2` defaults to `0`.

### JWT Sign

Creates a signed JSON Web Token.

Parameter|Type|Description
------------|------------|----
header|TEXT|JSON object serialized as text; its `"alg"` member selects the algorithm
claim|TEXT|JSON object serialized as text (the JWT payload)
privateKey|TEXT|signing key: shared secret for HS*, PEM private key for RS*/ES*
bearer|TEXT|`header.payload.signature` token, or `""` on failure

Supported `alg` values:

| Family | Algorithms | Key format |
|--------|-----------|------------|
| HMAC | HS256, HS384, HS512 | shared secret text |
| RSA | RS256, RS384, RS512 | PEM-encoded RSA private key |
| ECDSA | ES256, ES384, ES512 | PEM-encoded EC private key |

Behavior notes:

- String values of the `exp`, `iat`, and `nbf` claim members are converted to
  numbers before signing.
- The returned token has no `Bearer ` prefix.
- On any failure (unparsable JSON, unknown `alg`, unreadable key, signing
  error) the command returns `""`.

### JWT Verify

Verifies a token and returns `1` when the signature is valid, `0` otherwise.

Parameter|Type|Description
------------|------------|----
bearer|TEXT|token produced by `JWT Sign`; an optional `Bearer ` prefix is accepted
privateKey|TEXT|verification key: shared secret for HS*, PEM public key for RS*/ES*
valid|LONGINT|`1` if valid, `0` if invalid or malformed

### Examples

From `test/Project/Sources/Methods/TEST_HMAC.4dm` — HMAC with a shared secret:

```4d
$secret:="secret"
$header:=New object:C1471("alg";"HS256";"typ";"JWT")
$payload:=New object:C1471("sub";"1234567890";"name";"John Doe";"iat";"1516239022")

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$secret)
ASSERT:C1129(1=JWT Verify ($token;$secret))
```

`TEST_RSA.4dm` and `TEST_ES.4dm` do the same with PEM keys loaded from
`test/Resources/` (`RS256-private.pem`/`RS256-public.pem`, `ES256-*`, etc.) and
cover RS256/384/512 and ES256/384/512 respectively — sign with the private key,
verify with the public key:

```4d
$privatekey:=File:C1566("/RESOURCES/RS256-private.pem").getText("ascii";Document unchanged:K24:18)
$publickey:=File:C1566("/RESOURCES/RS256-public.pem").getText("ascii";Document unchanged:K24:18)

$header:=New object:C1471("alg";"RS256";"typ";"JWT")
$payload:=New object:C1471("sub";"1234567890";"name";"John Doe";"admin";True:C214;"iat";"1516239022")

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)
ASSERT:C1129(1=JWT Verify ($token;$publickey))
```

### Quick Reference

- Sign: JSON header + JSON payload + key → compact JWS token (`""` on failure)
- Verify: token + key → `1` valid / `0` invalid
- `alg` lives in the header JSON; HS* use the secret directly, RS*/ES* use PEM
- No expiration check: `exp`/`iat`/`nbf` are signed as numbers but `JWT Verify`
  only checks the signature
