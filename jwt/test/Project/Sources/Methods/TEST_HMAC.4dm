//%attributes = {}
$secret:="secret"
$header:=New object:C1471("alg";"HS256";"typ";"JWT")
$payload:=New object:C1471("sub";"1234567890";"name";"John Doe";"iat";"1516239022")

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$secret)
ASSERT:C1129(1=JWT Verify ($token;$secret))

$header.alg:="HS384"

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$secret)
ASSERT:C1129(1=JWT Verify ($token;$secret))

$header.alg:="HS512"

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$secret)
ASSERT:C1129(1=JWT Verify ($token;$secret))