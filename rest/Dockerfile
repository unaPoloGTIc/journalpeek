FROM debian:bullseye AS build

RUN apt-get update && apt-get install -y --force-yes --fix-missing \
    g++ libcpprest-dev make libsystemd-dev pkg-config libstdc++6 valgrind

#TODO: just copy dir?
COPY rest.cpp .
COPY rest-internal.cpp .
COPY rest-internal.h .
COPY test.cpp .
COPY Makefile .

RUN make build

FROM build AS test

RUN apt-get update && apt-get install -y --force-yes --fix-missing \
    libcpprest2.10 libcurl4-g* libgmock-dev libsystemd0

COPY testdata/testlog.journal testdata/testlog.journal

RUN make test
RUN ./test

FROM test

ENTRYPOINT ["./rest"]
