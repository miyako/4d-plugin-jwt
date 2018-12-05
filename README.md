# 4d-plugin-jwt
Sign and Verify JSON Web Token

based on [josepp](https://github.com/troian/josepp) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp)

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

### Version

<img src="https://cloud.githubusercontent.com/assets/1725068/18940649/21945000-8645-11e6-86ed-4a0f800e5a73.png" width="32" height="32" /> <img src="https://cloud.githubusercontent.com/assets/1725068/18940648/2192ddba-8645-11e6-864d-6d5692d55717.png" width="32" height="32" /> <img src="https://user-images.githubusercontent.com/1725068/41266195-ddf767b2-6e30-11e8-9d6b-2adf6a9f57a5.png" width="32" height="32" />

![preemption xx](https://user-images.githubusercontent.com/1725068/41327179-4e839948-6efd-11e8-982b-a670d511e04f.png)

### Releases 

[1.1](https://github.com/miyako/4d-plugin-jwt/releases/tag/1.1)

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
header|TEXT|JSON
claim|TEXT|JSON
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

## Examples

* EC

```
$private_key:=Document to text(Get 4D folder(Current resources folder)+"ecsdata"+Folder separator+"private.pem";"utf-8";Document unchanged)
$public_key:=Document to text(Get 4D folder(Current resources folder)+"ecsdata"+Folder separator+"public.pem";"utf-8";Document unchanged)

C_OBJECT($header)

$header:=New object(\
JWT Header typ;"JWT";\
JWT Header alg;JWT Digest ES256)

C_OBJECT($claim)
$claim:=New object(\
JWT Claim sub;"1234567890";\
"name";"John Doe";\
"admin";True;\
JWT Claim iat;1516239022)

$assertion:=JWT Sign (JSON Stringify($header);JSON Stringify($claim);$private_key)
SET TEXT TO PASTEBOARD($assertion)

ASSERT(1=JWT Verify ($assertion;$public_key))  //normally verify with public key

ASSERT(1=JWT Verify ($assertion;$private_key))  //this is fallback
```

* RSA

```
$private_key:=Document to text(Get 4D folder(Current resources folder)+"rsa"+Folder separator+"private.pem";"utf-8";Document unchanged)
$public_key:=Document to text(Get 4D folder(Current resources folder)+"rsa"+Folder separator+"public.pem";"utf-8";Document unchanged)

C_OBJECT($header)

$header:=New object(\
JWT Header typ;"JWT";\
JWT Header alg;JWT Digest RS256)

C_OBJECT($claim)
$claim:=New object(\
JWT Claim sub;"1234567890";\
"name";"John Doe";\
"admin";True;\
JWT Claim iat;1516239022)

$assertion:=JWT Sign (JSON Stringify($header);JSON Stringify($claim);$private_key)
SET TEXT TO PASTEBOARD($assertion)

ASSERT(1=JWT Verify ($assertion;$public_key))  //normally verify with public key

ASSERT(1=JWT Verify ($assertion;$private_key))  //this is fallback
```

* HMAC

```
$private_key:="secret"

C_OBJECT($header)

$header:=New object(\
JWT Header typ;"JWT";\
JWT Header alg;JWT Digest HS256)

C_OBJECT($claim)
$claim:=New object(\
JWT Claim sub;"1234567890";\
"name";"John Doe";\
"admin";True;\
JWT Claim iat;1516239022)

$assertion:=JWT Sign (JSON Stringify($header);JSON Stringify($claim);$private_key)
SET TEXT TO PASTEBOARD($assertion)

ASSERT(1=JWT Verify ($assertion;$private_key))
```
