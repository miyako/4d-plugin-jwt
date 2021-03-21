//%attributes = {}
$privatekey:=File:C1566("/RESOURCES/ES256-private.pem").getText("ascii";Document unchanged:K24:18)
$publickey:=File:C1566("/RESOURCES/ES256-public.pem").getText("ascii";Document unchanged:K24:18)

$header:=New object:C1471("alg";"ES256";"typ";"JWT")
$payload:=New object:C1471("sub";"1234567890";"name";"John Doe";"iat";"1516239022")

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)
ASSERT:C1129(1=JWT Verify ($token;$publickey))

$privatekey:=File:C1566("/RESOURCES/ES384-private.pem").getText("ascii";Document unchanged:K24:18)
$publickey:=File:C1566("/RESOURCES/ES384-public.pem").getText("ascii";Document unchanged:K24:18)

$header.alg:="ES384"

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)
ASSERT:C1129(1=JWT Verify ($token;$publickey))

$privatekey:=File:C1566("/RESOURCES/ES512-private.pem").getText("ascii";Document unchanged:K24:18)
$publickey:=File:C1566("/RESOURCES/ES512-public.pem").getText("ascii";Document unchanged:K24:18)

$header.alg:="ES512"

$token:=JWT Sign (JSON Stringify:C1217($header);JSON Stringify:C1217($payload);$privatekey)

SET TEXT TO PASTEBOARD:C523($token)

ASSERT:C1129(1=JWT Verify ($token;$publickey))
