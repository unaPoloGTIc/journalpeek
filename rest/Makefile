name = jd-restify
containername = $(name)
imagename = $(name)

build: rest-internal.cpp rest-internal.h rest.cpp
	g++ rest-internal.cpp rest.cpp -std=c++17 -o rest \
	-lcrypto -lssl -lcpprest -lpthread -lboost_system \
	`pkg-config --cflags --libs libsystemd`

test: rest test.cpp rest-internal.cpp rest-internal.h
	g++ test.cpp rest-internal.cpp -std=c++17 -o test \
	-lgtest -lgmock -lboost_system -lcrypto -lssl -lcpprest \
	-lpthread `curl-config --libs`

dockerize: Dockerfile #builds and runs tests in the container.
	docker build -t $(imagename) .
