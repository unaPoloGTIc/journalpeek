name = jd-restify
containername = $(name)
imagename = $(name)

build: rest-internal.cpp rest-internal.h rest.cpp
	g++ rest-internal.cpp rest.cpp -Wpedantic -Wall -Wextra -std=c++17 -o rest \
	-lcrypto -lssl -lcpprest -lpthread -lboost_system \
	`pkg-config --cflags --libs libsystemd`

test: build test.cpp rest-internal.cpp rest-internal.h
	g++ -g test.cpp rest-internal.cpp -Wpedantic -Wall -Wextra -std=c++17 -o test \
	-lgtest -lgmock -lboost_system -lcrypto -lssl -lcpprest \
	-lpthread `curl-config --libs` `pkg-config --cflags --libs libsystemd`
	./test
	valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --track-origins=yes ./test

dockerize: Dockerfile #builds and runs tests in the container.
	docker build -t $(imagename) .
