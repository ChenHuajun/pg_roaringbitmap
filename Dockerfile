ARG BASE_IMAGE=postgres:17-alpine

## Custom Alpine Postgres docker file with custom extensions
FROM ${BASE_IMAGE} AS builder

# Install required dependencies
RUN apk --no-cache add \
    python3 \
    py3-pip \
    cmake \
    make \
    gcc \
    g++ \
    clang19 \
    llvm19 \
    postgresql-dev

COPY . /workdir

WORKDIR /workdir

RUN make -f Makefile_native && make install

## Cleanup to reduce image size
RUN apk del \
    python3 \
    py3-pip \
    cmake \
    make \
    gcc \
    g++ \
    clang19 \
    llvm19 \
    postgresql-dev \
    && rm -rf /var/cache/apk/* \
    && rm -rf /root/.cache \
    && rm -rf /root/.pgxn \
    && rm -fr /pyenv \
    && rm -fr /workdir

## Custom Alpine Postgres docker file with our extensions
FROM ${BASE_IMAGE}

## Copy all extensions from the builder stage
COPY --from=builder /usr/local/lib/postgresql/* /usr/local/lib/postgresql/
COPY --from=builder /usr/local/share/postgresql/extension/* /usr/local/share/postgresql/extension/
