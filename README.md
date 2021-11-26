![version](https://img.shields.io/badge/version-17%2B-3E8B93)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-jwt)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-jwt/total)

**Note**: for v17 and earlier, move `manifest.json` to `Contents`

# 4d-plugin-jwt
Sign and Verify JSON Web Token based on ~~[josepp](https://github.com/troian/josepp)~~ [jwtpp](https://github.com/troian/jwtpp) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp).

### Native solution available

Please note that [CryptoKey](https://developer.4d.com/docs/en/API/CryptoKeyClass.html) can be used to process JWT.

c.f. https://blog.4d.com/cryptokey-encrypt-decrypt-sign-and-verify/

## Included In This Repo
• Source code for the plugin
• Test database (folder `test`)

## Installation Instructions
1. Extract either the **JWT.DMG** or the **JWT.zip** archives
2. Add the **JWT.bundle** to your database's **Plugins** folder.

## Syntax

```
timestamp:=JWT Timestamp ({time1{;time2}})
```

Parameter|Type|Description
------------|------------|----
time1|TEXT|anchor time or ``""`` for current time
time2|TEXT|time to add
timestamp|TEXT|UNIX time in milliseconds

```
bearer:=JWT Sign (header;claim;privateKey)
```

Parameter|Type|Description
------------|------------|----
header|TEXT|
claim|TEXT|
privateKey|TEXT|
bearer|TEXT|

```
valid:=JWT Verify (bearer;privateKey)
```

Parameter|Type|Description
------------|------------|----
bearer|TEXT|
privateKey|TEXT|
valid|LONGINT|``1``:valid
