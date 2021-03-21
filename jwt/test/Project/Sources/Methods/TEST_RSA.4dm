//%attributes = {}
$privatekey:=File:C1566("/RESOURCES/RS256-private.pem").getText("ascii";Document unchanged:K24:18)
$publickey:=File:C1566("/RESOURCES/RS256-public.pem").getText("ascii";Document unchanged:K24:18)

$header:=New object:C1471("alg";"RS256";"typ";"JWT")
$payload:=New object:C1471("sub";"1234567890";"name";"John Doe";"admin";True:C214;"iat";"1516239022")

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)
ASSERT:C1129(1=JWT Verify ($token;$publickey))

$header.alg:="RS384"

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)
ASSERT:C1129(1=JWT Verify ($token;$publickey))

$header.alg:="RS512"

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)
ASSERT:C1129(1=JWT Verify ($token;$publickey))


