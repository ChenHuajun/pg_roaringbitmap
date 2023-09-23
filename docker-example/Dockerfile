FROM postgres:16.0-bookworm AS builder
ENV VERSION_TAG 0.5.4

RUN apt-get update && apt-get install -y curl unzip make gcc postgresql-server-dev-16
RUN curl -LO "https://github.com/ChenHuajun/pg_roaringbitmap/archive/refs/tags/v$VERSION_TAG.zip"
RUN unzip "v$VERSION_TAG.zip"
WORKDIR "pg_roaringbitmap-$VERSION_TAG"
RUN make -f Makefile_native && make install

FROM postgres:16.0-bookworm
COPY --from=builder /usr/share/postgresql/16/extension/roaringbitmap* /usr/share/postgresql/16/extension
COPY --from=builder /usr/lib/postgresql/16/lib/roaringbitmap.so /usr/lib/postgresql/16/lib
COPY --from=builder /usr/lib/postgresql/16/lib/bitcode/roaringbitmap /usr/lib/postgresql/16/lib/bitcode/roaringbitmap
COPY --from=builder /usr/lib/postgresql/16/lib/bitcode/roaringbitmap.index.bc /usr/lib/postgresql/16/lib/bitcode/roaringbitmap.index.bc

COPY load-extension.sql /docker-entrypoint-initdb.d
