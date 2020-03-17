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
docker run -v PATH/TO/JOURNAL_FILES/:/testdata -p 6666:6666 -v /PATH/TO/CERTS/:/certs --rm -td jd-restify
```

`PATH/TO/JOURNAL_FILES/` must contain only *.journal files.  
`/PATH/TO/CERTS/` must contain:  
`/live/trex-security.com/fullchain.pem`, `/live/trex-security.com/privkey.pem`  
LetsEncrypt certs work just fine.  
The hardcoded domain helps me with key rotation on my demo machine (LE keys are symlinked and Docker doesn't like that.).  
You can ignore my stupidity or change the hardcoded value (or hate mail if that's legal where you live).

## Serve via angular:
```
cd webui
npm install
ng serve
```
