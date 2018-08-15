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
https://support.google.com/a/answer/162106

<img width="300" src="https://user-images.githubusercontent.com/1725068/44127804-005eb000-a07a-11e8-9826-4ea44c62fc4f.png" />

* Client Name: ``service@my-project-27018.iam.gserviceaccount.com`` or ``100484781807410278113``
* Scopes: ``https://www.googleapis.com/auth/admin.directory.user,https://www.googleapis.com/auth/admin.directory.group``

[Enable G Suite Domain-wide Delegation](https://console.developers.google.com/iam-admin/serviceaccounts/details/100484781807410278113;edit=true?organizationId=772231897481&project=my-project-27018&supportedpurview=project)

<img width="300" src="https://user-images.githubusercontent.com/1725068/44128105-e9cde5b6-a07b-11e8-8ba4-8475072cdfbe.png" />
