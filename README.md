# 4d-plugin-jwt
Sign and Verify JSON Web Token

based on [josepp](https://github.com/troian/josepp) and [jsoncpp](https://github.com/open-source-parsers/jsoncpp)

### Platform

| carbon | cocoa | win32 | win64 |
|:------:|:-----:|:---------:|:---------:|
||<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|<img src="https://cloud.githubusercontent.com/assets/1725068/22371562/1b091f0a-e4db-11e6-8458-8653954a7cce.png" width="24" height="24" />|

### Version

<img width="32" height="32" src="https://user-images.githubusercontent.com/1725068/73986501-15964580-4981-11ea-9ac1-73c5cee50aae.png"> <img src="https://user-images.githubusercontent.com/1725068/73987971-db2ea780-4984-11ea-8ada-e25fb9c3cf4e.png" width="32" height="32" />

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
