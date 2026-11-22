FROM ubuntu:24.04

COPY dist/trrispo25_*.deb /tmp/app.deb
RUN apt-get update \
    && apt-get install -y --no-install-recommends /tmp/app.deb curl \
    && rm -rf /var/lib/apt/lists/* /tmp/app.deb

USER 10001

ENV APP_API_TOKEN=""

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -fs http://localhost:8080/health || exit 1

CMD ["trrispo25", "serve"]
