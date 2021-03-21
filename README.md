![version](https://img.shields.io/badge/version-18%2B-EB8E5F)
![platform](https://img.shields.io/static/v1?label=platform&message=osx-intel%20|%20osx-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-jwt)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-jwt/total)

# 4d-plugin-jwt
Sign and Verify JSON Web Token based on ~~[josepp](https://github.com/troian/josepp)~~ [jwtpp](https://github.com/troian/jwtpp) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp).

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
