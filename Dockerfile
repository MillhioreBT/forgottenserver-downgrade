FROM alpine:3.17.3 AS build
# crypto++-dev is in edge/community
RUN apk add --no-cache --repository http://dl-cdn.alpinelinux.org/alpine/edge/community/ \
  build-base \
  boost-dev \
  cmake \
  crypto++-dev \
  fmt-dev \
  lua5.4 \
  mariadb-connector-c-dev \
  pugixml-dev \
  samurai

COPY cmake /usr/src/forgottenserver-downgrade/cmake/
COPY src /usr/src/forgottenserver-downgrade/src/
COPY CMakeLists.txt CMakePresets.json /usr/src/forgottenserver-downgrade/
WORKDIR /usr/src/forgottenserver-downgrade
RUN cmake --preset default && cmake --build --config RelWithDebInfo --preset default

FROM alpine:3.17.3
# crypto++ is in edge/community
RUN apk add --no-cache --repository http://dl-cdn.alpinelinux.org/alpine/edge/community/ \
  boost-iostreams \
  boost-system \
  crypto++ \
  fmt \
  lua5.4 \
  mariadb-connector-c \
  pugixml

COPY --from=build /usr/src/forgottenserver-downgrade/build/tfs /bin/tfs
COPY data /srv/data/
COPY LICENSE README.md *.dist *.sql key.pem /srv/

EXPOSE 7171 7172
WORKDIR /srv
VOLUME /srv
ENTRYPOINT ["/bin/tfs"]
