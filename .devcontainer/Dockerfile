FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.21.5"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# El contenido que VS Code ya generó estará aquí arriba...

# --- AÑADIR ESTAS LÍNEAS AL FINAL DEL DOCKERFILE ---

# Actualizamos e instalamos git, que es necesario para clonar el SDK
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends git

# Clonamos, instalamos y activamos el SDK de Emscripten
# Lo instalamos en una carpeta estándar dentro del contenedor
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk
RUN cd /opt/emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

# Añadimos el entorno de emsdk al PATH permanentemente para todas las terminales
RUN echo 'source /opt/emsdk/emsdk_env.sh > /dev/null 2>&1' >> /etc/bash.bashrc
