FROM alpine:3.22 AS build
RUN apk add --no-cache build-base gmp-dev
WORKDIR /src
COPY render.c render_gmp.h ./
RUN cc -O3 -Wall -Wextra -Werror -ffp-contract=off -fopenmp render.c -lgmp -lm -o render && strip render
FROM alpine:3.22
RUN apk add --no-cache python3 gmp libgomp && adduser -D -u 10001 app
WORKDIR /app
COPY --from=build /src/render ./render
COPY server.py index.html app.js style.css ./
USER app
EXPOSE 8080
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1 OMP_NUM_THREADS=4 OMP_DYNAMIC=FALSE
HEALTHCHECK --interval=30s --timeout=3s CMD python3 -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8080/health', timeout=2)"
CMD ["python3", "server.py"]
