FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl zip unzip tar build-essential cmake ninja-build pkg-config \
    ca-certificates && rm -rf /var/lib/apt/lists/*
# Instalar vcpkg, fijado a una versión: las versiones de las dependencias ya las fija el builtin-baseline de
# vcpkg.json, esto fija la herramienta y sus scripts. Además forma parte de la caché de capas del CI, así que con
# un clone sin fijar la versión quedaría congelada en la caché y cambiaría sola al expirar. Actualizarlo a propósito.
ARG VCPKG_VERSION=2026.10.16
WORKDIR /opt
RUN git clone --branch ${VCPKG_VERSION} https://github.com/microsoft/vcpkg.git && cd vcpkg && ./bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"
# Instalar dependencias del proyecto
WORKDIR /usr/src/forgottenserver-downgrade
COPY vcpkg.json ./
# Reintentar: un fallo de red puntual (p. ej. DNS al bajar un port de GitHub) no debe tumbar el build.
# Los paquetes ya compilados quedan en la caché binaria local de vcpkg, así que cada reintento continúa donde quedó.
# Al terminar se borran las fuentes y temporales de cada port (--clean-after-build) y esa caché binaria: la capa
# solo guarda lo instalado, que es lo que reutiliza la caché de capas del CI.
RUN for attempt in 1 2 3; do \
        /opt/vcpkg/vcpkg install --triplet x64-linux --clean-after-build && break; \
        if [ "${attempt}" -eq 3 ]; then exit 1; fi; \
        echo "vcpkg install falló (intento ${attempt}/3), reintentando en 30s..."; \
        sleep 30; \
    done \
    && rm -rf /root/.cache/vcpkg
# Copiar el resto del código
COPY cmake /usr/src/forgottenserver-downgrade/cmake/
COPY src /usr/src/forgottenserver-downgrade/src/
COPY CMakeLists.txt /usr/src/forgottenserver-downgrade/
WORKDIR /usr/src/forgottenserver-downgrade
# CMake con el toolchain de vcpkg. VCPKG_INSTALLED_DIR apunta a las dependencias del paso anterior; sin él, el
# toolchain las vuelve a instalar en build/vcpkg_installed. Ninja compila en paralelo (make sin -j usa un solo núcleo).
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_INSTALLED_DIR=/usr/src/forgottenserver-downgrade/vcpkg_installed \
    && cmake --build build

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=build /usr/src/forgottenserver-downgrade/build/tfs /bin/tfs
COPY data /srv/data/
COPY LICENSE README.md *.dist *.sql key.pem /srv/
EXPOSE 7171 7172
WORKDIR /srv
VOLUME /srv
ENTRYPOINT ["/bin/tfs"]
