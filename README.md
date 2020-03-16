![build rest wrapper](https://github.com/unaPoloGTIc/journalpeek/workflows/build%20rest%20wrapper/badge.svg)

![ng build](https://github.com/unaPoloGTIc/journalpeek/workflows/ng%20build/badge.svg)

# journalpeek
C++, rest and web-ui wrappers for sd-journal

## This is a pet project, be advised that systemd-journal-gatewayd, Journalbeat exist.

## Build the backend:
```
cd rest  
make dockerize
```

## Start the backend:
```
docker run -p 6666:6666 --rm -td jd-restify
```
Or, to use your own journal files:  
```
docker run -v PATH/TO/JOURNAL_FILES:/testdata -p 6666:6666 --rm -td jd-restify
```

## Serve via angular:
```
cd webui
npm install
ng serve
```
