![build rest wrapper](https://github.com/unaPoloGTIc/journalpeek/workflows/build%20rest%20wrapper/badge.svg)

# journalpeek
C++, rest and web-ui wrappers for sd-journal

## This is a pet project, be advised that systemd-journal-gatewayd, Journalbeat exist.

## Build the backend:
```
cd rest  
make dockerize
```

## Start the backebd:
```
docker run --name rest-backend -p 6666:6666 --rm -td jd-restify
```

## Serve via angular:
(after the usual npm install...)  
```
ng serve
```
