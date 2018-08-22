# 4d-plugin-jwt
Sign and Verify JSON Web Token

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

## Steps

[Manage API client access](https://admin.google.com/AdminHome?chromeless=1#OGX:ManageOauthClients)

[https://support.google.com/](https://support.google.com/a/answer/162106)

[OAuth 2.0 Scopes for Google APIs](https://developers.google.com/identity/protocols/googlescopes)

<img width="300" src="https://user-images.githubusercontent.com/1725068/44127804-005eb000-a07a-11e8-9826-4ea44c62fc4f.png" />

* Client: 

```
service@my-project-27018.iam.gserviceaccount.com
```

or

```
100484781807410278113
```

* Scope: 

```
https://www.googleapis.com/auth/admin.directory.user,https://www.googleapis.com/auth/admin.directory.group
```

full scope for [Admin Directory API](https://developers.google.com/admin-sdk/directory/) ``directory_v1``

```
https://www.googleapis.com/auth/admin.directory.group,https://www.googleapis.com/auth/admin.directory.user,https://www.googleapis.com/auth/admin.directory.userschema,https://www.googleapis.com/auth/admin.directory.user.security,https://www.googleapis.com/auth/admin.directory.user.alias,https://www.googleapis.com/auth/admin.directory.orgunit,https://www.googleapis.com/auth/admin.directory.notifications,https://www.googleapis.com/auth/admin.directory.group.member,https://www.googleapis.com/auth/admin.directory.device.mobile.action,https://www.googleapis.com/auth/admin.directory.device.mobile,https://www.googleapis.com/auth/admin.directory.device.chromeos
```

[Enable G Suite Domain-wide Delegation](https://console.developers.google.com/iam-admin/serviceaccounts/details/100484781807410278113;edit=true?organizationId=772231897481&project=my-project-27018&supportedpurview=project)

<img width="300" src="https://user-images.githubusercontent.com/1725068/44128105-e9cde5b6-a07b-11e8-8ba4-8475072cdfbe.png" />

### Trouble shooting

When an address other than the service account itself is specified in ``sub`` (either an admin account or a regular account), the auth server returns an error.

```
{
 "error": "unauthorized_client",
 "error_description": "Client is unauthorized to retrieve access tokens using this method."
}
```

Evidentally, passing standard audience to ``aud`` results in error.

BAD: https://www.googleapis.com/oauth2/v4/token

```
{
 "error": "invalid_grant",
 "error_description": "Invalid JWT: Failed audience check. The right audience is https://www.googleapis.com/oauth2/v4/token"
}
```

GOOD: https://oauth2.googleapis.com/token

```
{
  "access_token": "ya29.c.Elz6Be4keP1I5udHB1-iJb6LiDpfTcpgBl_SNe6U6-LIVwN3pUBkunCDvCbbJjFAb0N90siKJKQB3xprTQZl5F4-h1LQ6FuUlgPc398I4fYB49Osn6hn64NiCgv6rQ",
  "expires_in": 3600,
  "token_type": "Bearer"
}
```

Including scope in request parameter results in error if ``sub`` is not the service account. No problems if ``sub`` is the service account. 

```
{
  "error": "invalid_scope",
  "error_description": "Invalid downscoping, scopes should not be specified as a request parameter."
}
```

HTTP Get can return error 61 connection refused, depending on the LAN confuration on v17

c.f. ACI0098213, ACI0098503
